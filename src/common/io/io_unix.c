#include "io.h"
#include "fastfetch.h"
#include "util/stringUtils.h"
#include "common/time.h"

#include <fcntl.h>
#include <termios.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <TargetConditionals.h>

#ifndef __APPLE__
#include <poll.h>
#else
#include <sys/select.h>
#endif

#if !TARGET_OS_IOS && FF_HAVE_WORDEXP
    #include <wordexp.h>
#else
    #include <glob.h>
#endif

static void createSubfolders(const char* fileName)
{
    FF_STRBUF_AUTO_DESTROY path = ffStrbufCreate();
    char* token = NULL;

    while ((token = strchr(fileName, '/')) != NULL)
    {
        ffStrbufAppendNS(&path, (uint32_t)(token - fileName + 1), fileName);
        mkdir(path.chars, S_IRWXU | S_IRGRP | S_IROTH);
        fileName = token + 1;
    }
}

bool ffWriteFileData(const char* fileName, size_t dataSize, const void* data)
{
    int openFlagsModes = O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC;
    mode_t openFlagsRights = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    int FF_AUTO_CLOSE_FD fd = open(fileName, openFlagsModes, openFlagsRights);
    if (fd == -1)
    {
        if (errno == ENOENT)
        {
            createSubfolders(fileName);
            fd = open(fileName, openFlagsModes, openFlagsRights);
            if (fd == -1)
                return false;
        }
        else
            return false;
    }

    return write(fd, data, dataSize) > 0;
}

static inline void readWithLength(int fd, FFstrbuf* buffer, uint32_t length)
{
    ffStrbufEnsureFixedLengthFree(buffer, length);
    ssize_t bytesRead = 0;
    while (length > 0 && (bytesRead = read(fd, buffer->chars + buffer->length, length)) > 0)
    {
        buffer->length += (uint32_t)bytesRead;
        length -= (uint32_t)bytesRead;
    }
}

static inline void readUntilEOF(int fd, FFstrbuf* buffer)
{
    ffStrbufEnsureFree(buffer, 31);
    uint32_t available = ffStrbufGetFree(buffer);
    ssize_t bytesRead = 0;
    while ((bytesRead = read(fd, buffer->chars + buffer->length, available)) > 0)
    {
        buffer->length += (uint32_t)bytesRead;
        if ((uint32_t)bytesRead == available)
            ffStrbufEnsureFree(buffer, buffer->allocated - 1);
        available = ffStrbufGetFree(buffer);
    }
}

bool ffAppendFDBuffer(int fd, FFstrbuf* buffer)
{
    struct stat fileInfo;
    if (fstat(fd, &fileInfo) != 0)
        return false;

    if (fileInfo.st_size > 0)
        readWithLength(fd, buffer, (uint32_t)fileInfo.st_size);
    else
        readUntilEOF(fd, buffer);

    buffer->chars[buffer->length] = '\0';
    return buffer->length > 0;
}

ssize_t ffReadFileData(const char* fileName, size_t dataSize, void* data)
{
    int FF_AUTO_CLOSE_FD fd = open(fileName, O_RDONLY | O_CLOEXEC);
    if (fd == -1)
        return -1;

    return ffReadFDData(fd, dataSize, data);
}

ssize_t ffReadFileDataRelative(int dfd, const char* fileName, size_t dataSize, void* data)
{
    int FF_AUTO_CLOSE_FD fd = openat(dfd, fileName, O_RDONLY | O_CLOEXEC);
    if (fd == -1)
        return -1;

    return ffReadFDData(fd, dataSize, data);
}

bool ffAppendFileBuffer(const char* fileName, FFstrbuf* buffer)
{
    int FF_AUTO_CLOSE_FD fd = open(fileName, O_RDONLY | O_CLOEXEC);
    if (fd == -1)
        return false;

    return ffAppendFDBuffer(fd, buffer);
}

bool ffAppendFileBufferRelative(int dfd, const char* fileName, FFstrbuf* buffer)
{
    int FF_AUTO_CLOSE_FD fd = openat(dfd, fileName, O_RDONLY | O_CLOEXEC);
    if (fd == -1)
        return false;

    return ffAppendFDBuffer(fd, buffer);
}

bool ffPathExpandEnv(const char* in, FFstrbuf* out)
{
#if !TARGET_OS_IOS && FF_HAVE_WORDEXP
    wordexp_t exp;
    if (wordexp(in, &exp, 0) != 0)
        return false;

    if (exp.we_wordc >= 1)
    {
        ffStrbufSetS(out, exp.we_wordv[exp.we_wordc > 1 ? ffTimeGetNow() % exp.we_wordc : 0]);
        wordfree(&exp);
        return true;
    }
    wordfree(&exp);
    return false;
#else
    glob_t gb;
    if (glob(in, GLOB_NOSORT
#ifdef GLOB_TILDE
             | GLOB_TILDE
#endif
#ifdef GLOB_BRACE
             | GLOB_BRACE
#endif
             , NULL, &gb) != 0)
        return false;

    if (gb.gl_pathc >= 1)
    {
        ffStrbufSetS(out, gb.gl_pathv[gb.gl_pathc > 1 ? ffTimeGetNow() % (unsigned)gb.gl_pathc : 0]);
        globfree(&gb);
        return true;
    }

    globfree(&gb);
    return false;
#endif
}

static int ftty = -1;
static struct termios oldTerm;

void restoreTerm(void)
{
    tcsetattr(ftty, TCSAFLUSH, &oldTerm);
}

const char* ffGetTerminalResponse(const char* request, int nParams, const char* format, ...)
{
    if (ftty < 0)
    {
        ftty = open("/dev/tty", O_RDWR | O_NOCTTY | O_CLOEXEC);
        if (ftty < 0)
            return "open(/dev/tty) failed";

        if (tcgetattr(ftty, &oldTerm) == -1)
            return "tcgetattr failed";

        struct termios newTerm = oldTerm;
        newTerm.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
        if (tcsetattr(ftty, TCSAFLUSH, &newTerm) == -1)
            return "tcsetattr failed";

        atexit(restoreTerm);
    }

    ffWriteFDData(ftty, strlen(request), request);

#ifndef __APPLE__
    if (poll(&(struct pollfd){ .fd = ftty, .events = POLLIN }, 1, FF_IO_TERM_RESP_WAIT_MS) <= 0)
        return "poll timeout or failed";
#else
    {
        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(ftty, &rd);
        if (select(ftty + 1, &rd, NULL, NULL,
                   &(struct timeval){ .tv_sec = FF_IO_TERM_RESP_WAIT_MS / 1000,
                                      .tv_usec = (FF_IO_TERM_RESP_WAIT_MS % 1000) * 1000 }) <= 0)
            return "select timeout or failed";
    }
#endif

    char buffer[1024];
    size_t bytesRead = 0;

    va_list args;
    va_start(args, format);

    while (true)
    {
        ssize_t nRead = read(ftty, buffer + bytesRead, sizeof(buffer) - bytesRead - 1);
        if (nRead <= 0)
        {
            va_end(args);
            return "read failed";
        }

        bytesRead += (size_t)nRead;
        buffer[bytesRead] = '\0';

        va_list cargs;
        va_copy(cargs, args);
        int ret = vsscanf(buffer, format, cargs);
        va_end(cargs);

        if (ret > 0 && ret >= nParams)
            break;
    }

    va_end(args);
    return NULL;
}

bool ffSuppressIO(bool suppress)
{
#ifndef NDEBUG
    if (instance.config.display.debugMode)
        return false;
#endif

    static bool init = false;
    static int origOut = -1;
    static int origErr = -1;
    static int nullFile = -1;

    if (!init)
    {
        if (!suppress)
            return true;

        origOut = dup(STDOUT_FILENO);
        origErr = dup(STDERR_FILENO);
        nullFile = open("/dev/null", O_WRONLY | O_CLOEXEC);
        init = true;
    }

    if (nullFile == -1)
        return false;

    fflush(stdout);
    fflush(stderr);

    dup2(suppress ? nullFile : origOut, STDOUT_FILENO);
    dup2(suppress ? nullFile : origErr, STDERR_FILENO);
    return true;
}

static void listFilesRecursivelyInternal(FFstrbuf* basePath, void (*cb)(const char*, void*), void* cbData)
{
    DIR* dir = opendir(basePath->chars);
    if (!dir)
        return;

    struct dirent* entry;
    FF_STRBUF_AUTO_DESTROY path = ffStrbufCreate();
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.' &&
            (entry->d_name[1] == '\0' ||
             (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
            continue;

        ffStrbufSetS(&path, basePath->chars);
        ffStrbufAppendS(&path, "/");
        ffStrbufAppendS(&path, entry->d_name);

        if (entry->d_type == DT_DIR)
        {
            listFilesRecursivelyInternal(&path, cb, cbData);
        }
        else
            cb(path.chars, cbData);
    }

    closedir(dir);
}

void ffListFilesRecursively(const char* path, void (*cb)(const char*, void*), void* cbData)
{
    FF_STRBUF_AUTO_DESTROY basePath = ffStrbufCreateS(path);
    listFilesRecursivelyInternal(&basePath, cb, cbData);
}

