#include "clay.h"
#include "layout_model.h"
#include "raylib.h"
#include "nfd.h"
#include "cJSON.h"
#include <stdio.h>
#include <stddef.h>
#include <ctype.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windef.h>
#include <winbase.h>
#include <processthreadsapi.h>
#include <errhandlingapi.h>
#include <namedpipeapi.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#endif

#define TEXTBOX_BUFFER_SIZE 256
#define FLOAT_MAX 1e5f

#define TEXT_CONFIG_DEFAULT CLAY_TEXT_CONFIG({ \
    .fontId = FONT_ID_BODY,                    \
    .fontSize = fontData.fontSize,             \
    .textColor = COLOR_WHITE,                  \
})
#define TEXT_CONFIG_FAINT CLAY_TEXT_CONFIG({ \
    .fontId = FONT_ID_BODY,                  \
    .fontSize = fontData.fontSize,           \
    .textColor = COLOR_LIGHTGRAY,            \
})
#define TEXT_CONFIG_BOLD CLAY_TEXT_CONFIG({ \
    .fontId = FONT_ID_BOLD,                 \
    .fontSize = fontData.fontSize,          \
    .textColor = COLOR_WHITE,               \
})
#define TEXT_CONFIG_SYMBOL CLAY_TEXT_CONFIG({ \
    .fontId = FONT_ID_SYMBOL,                 \
    .fontSize = fontData.fontSize,            \
    .textColor = COLOR_WHITE,                 \
})

#define DROPDOWN_OPTION_NULL {"", NULL}
#define DROPDOWN_OPTION_UNSELECTED {"---", ""}
#define DROPDOWN_OPTION_OUTPUT_TYPE_VIDEO {"Video", "v"}
#define DROPDOWN_OPTION_OUTPUT_TYPE_AUDIO {"Audio", "a"}
#define DROPDOWN_OPTION_OUTPUT_TYPE_IMAGE {"Image", "i"}

#define LOG(format, ...) printf("\x1b[33mLOG: " format "\x1b[0m\n", ##__VA_ARGS__)
#ifndef PRODUCTION
#define ERROR(format, ...) printf("\x1b[31mERROR: " format "\x1b[0m\n", ##__VA_ARGS__)
#else
#define ERROR(format, ...) ((void)0)
#endif

#define CLAMP(val, min, max) (val < min) ? min : (val > max) ? max \
                                                             : val

typedef enum
{
    FONT_ID_BODY,
    FONT_ID_BOLD,
    // FONT_ID_SYMBOL,
    FONT_ID_DUMMY_LAST
} FontID;

typedef enum
{
    TEXTBOX_ID_INPUT_PATH,
    TEXTBOX_ID_FPS,
    TEXTBOX_ID_DURATION_START_VIDEO,
    TEXTBOX_ID_DURATION_END_VIDEO,
    TEXTBOX_ID_SPEED_VIDEO,
    TEXTBOX_ID_CROP_W,
    TEXTBOX_ID_CROP_H,
    TEXTBOX_ID_CROP_X,
    TEXTBOX_ID_CROP_Y,
    TEXTBOX_ID_SCALE_W,
    TEXTBOX_ID_SCALE_H,
    TEXTBOX_ID_VOLUME,
    TEXTBOX_ID_DURATION_START_AUDIO,
    TEXTBOX_ID_DURATION_END_AUDIO,
    TEXTBOX_ID_SPEED_AUDIO,
    TEXTBOX_ID_DELAY,
    TEXTBOX_ID_SUBTITLES_SOURCE,
    TEXTBOX_ID_OUTPUT_PATH,
    TEXTBOX_ID_DUMMY_LAST
} TextboxID;

typedef enum
{
    DROPDOWN_ID_OUTPUT_TYPE,
    DROPDOWN_ID_LOUDNORM_ENABLE,
    DROPDOWN_ID_CHANNEL_LAYOUT,
    DROPDOWN_ID_SUBTITLES,
    DROPDOWN_ID_DUMMY_LAST
} DropdownID;

typedef enum
{
    BUTTON_ID_BROWSE_INPUT,
    BUTTON_ID_LOAD_INPUT,
    BUTTON_ID_LOAD_PREVIEW,
    BUTTON_ID_PREVIEW_SIZE_DOWN,
    BUTTON_ID_PREVIEW_SIZE_UP,
    BUTTON_ID_BROWSE_SUBTITLES_SOURCE,
    BUTTON_ID_BROWSE_OUTPUT,
    BUTTON_ID_CONVERT,
    BUTTON_ID_CONVERT_CANCEL,
    BUTTON_ID_DUMMY_LAST
} ButtonID;

typedef enum
{
    STREAM_ID_VIDEO,
    STREAM_ID_AUDIO,
    STREAM_ID_SUBTITLES,
    STREAM_ID_DUMMY_LAST,
} StreamID;

typedef enum
{
    TAB_ID_DIMENSIONS,
    TAB_ID_VIDEO,
    TAB_ID_AUDIO,
    TAB_ID_SUBTITLES,
    TAB_ID_DUMMY_LAST,
} TabID;

typedef enum
{
    SCROLL_ID_WINDOW,
    SCROLL_ID_DUMMY_LAST,
} ScrollID;

typedef enum
{
    DIRECTION_ID_NONE,
    DIRECTION_ID_VERTICAL,
    DIRECTION_ID_HORIZONTAL,
} DirectionID;

typedef enum
{
    OUTPUT_TYPE_ID_VIDEO,
    OUTPUT_TYPE_ID_AUDIO,
    OUTPUT_TYPE_ID_IMAGE,
    OUTPUT_TYPE_ID_DUMMY_LAST,
} OutputTypeID;

typedef enum
{
    FORMAT_VIDEO_ID_GIF,
    FORMAT_VIDEO_ID_MKV,
    FORMAT_VIDEO_ID_MOV,
    FORMAT_VIDEO_ID_MP4,
    FORMAT_VIDEO_ID_WEBM,
    FORMAT_VIDEO_ID_DUMMY_LAST,
} FormatVideoID;

typedef enum
{
    FORMAT_AUDIO_ID_FLAC,
    FORMAT_AUDIO_ID_M4A,
    FORMAT_AUDIO_ID_MP3,
    FORMAT_AUDIO_ID_OGG,
    FORMAT_AUDIO_ID_OPUS,
    FORMAT_AUDIO_ID_WAV,
    FORMAT_AUDIO_ID_DUMMY_LAST,
} FormatAudioID;

typedef enum
{
    FORMAT_IMAGE_ID_JPEG,
    FORMAT_IMAGE_ID_PNG,
    FORMAT_IMAGE_ID_TIFF,
    FORMAT_IMAGE_ID_WEBP,
    FORMAT_IMAGE_ID_DUMMY_LAST,
} FormatImageID;

const Clay_Color COLOR_BG_MAIN = {50, 50, 50, 255};
const Clay_Color COLOR_BG_SECTION = {70, 70, 70, 255};
const Clay_Color COLOR_BG_TEXTBOX = {80, 80, 80, 255};
const Clay_Color COLOR_BG_TEXTBOX_DISABLED = {110, 110, 110, 255};
const Clay_Color COLOR_BG_BUTTON = {80, 80, 80, 255};
const Clay_Color COLOR_BG_BUTTON_HOVERED = {100, 100, 100, 255};
const Clay_Color COLOR_BG_BUTTON_DISABLED = {110, 110, 110, 255};
const Clay_Color COLOR_BG_DROPDOWN = {80, 80, 80, 255};
const Clay_Color COLOR_BG_DROPDOWN_HOVERED = {100, 100, 100, 255};
const Clay_Color COLOR_BG_TAB = {50, 50, 50, 255};
const Clay_Color COLOR_BG_TAB_HOVERED = {100, 100, 100, 255};
const Clay_Color COLOR_BG_TAB_SELECTED = {80, 80, 80, 255};
const Clay_Color COLOR_BG_TAB_DISABLED = {110, 110, 110, 255};
const Clay_Color COLOR_BG_SCROLLBAR = {200, 200, 200, 255};
const Clay_Color COLOR_BG_THUMB = {140, 140, 140, 255};
const Clay_Color COLOR_BG_THUMB_HOVERED = {120, 120, 120, 255};

const Clay_Color COLOR_BORDER_TEXTBOX = {150, 150, 150, 255};
const Clay_Color COLOR_BORDER_TEXTBOX_FOCUSED = {200, 200, 200, 255};
const Clay_Color COLOR_BORDER_BUTTON = {150, 150, 150, 255};
const Clay_Color COLOR_BORDER_DROPDOWN = {150, 150, 150, 255};
const Clay_Color COLOR_BORDER_TAB = {150, 150, 150, 255};

const Clay_Color COLOR_TRANSPARENT = {0, 0, 0, 0};
const Clay_Color COLOR_WHITE = {255, 255, 255, 255};
const Clay_Color COLOR_BLACK = {0, 0, 0, 255};
const Clay_Color COLOR_DARKGRAY = {80, 80, 80, 255};
const Clay_Color COLOR_GRAY = {140, 140, 140, 255};
const Clay_Color COLOR_LIGHTGRAY = {200, 200, 200, 255};

typedef struct
{
    size_t fontSize;
    size_t fontSizeMin;
    size_t fontSizeMax;
    size_t charWidth;
    size_t charHeight;
    size_t charHeightOverFour;
} FontData;

typedef struct
{
    bool focusRegistered;
    int focusIndex;
    double focusStartTime;
} FocusData;

typedef struct
{
    bool isNumberbox;
    bool isInt;
    float min;
    float max;
} NumberboxConfig;

typedef struct
{
    char chars[TEXTBOX_BUFFER_SIZE];
    const char *charsDefault;
    size_t length;
    size_t cursorPosition;
    NumberboxConfig numberboxConfig;
} TextboxBuffer;

typedef struct
{
    TextboxBuffer textboxBuffers[TEXTBOX_ID_DUMMY_LAST];
    bool isInit[TEXTBOX_ID_DUMMY_LAST];
    bool isEnabled[TEXTBOX_ID_DUMMY_LAST];
    int hoveredTextbox;
    FocusData focusData;
} TextboxData;

typedef struct
{
    const char *label;
    const char *value;
} DropdownOption;

typedef struct
{
    size_t selectedOptions[DROPDOWN_ID_DUMMY_LAST];
    const char *selectedValues[DROPDOWN_ID_DUMMY_LAST];
    bool isInit[DROPDOWN_ID_DUMMY_LAST];
    size_t hoveredOption;
    const char *hoveredValue;
} DropdownData;

typedef struct
{
    size_t selectedTab;
    bool isDisabled[TAB_ID_DUMMY_LAST];
} TabData;

typedef struct
{
    Clay_ScrollContainerData data[SCROLL_ID_DUMMY_LAST];
    int scrolling;
    DirectionID directionLock;
    Vector2 middleClickPosition;
} ScrollData;

typedef struct
{
#ifdef _WIN32
    HANDLE process;
    HANDLE stdoutRead;
#else
    pid_t pid;
    int stdoutfd;
#endif
    bool valid;
} ChildProcessData;

typedef struct
{
    char inputPath[TEXTBOX_BUFFER_SIZE];
    size_t streamCounts[STREAM_ID_DUMMY_LAST];
    double inputDurationSeconds;
    ChildProcessData convertProcess;
    char convertOutput[8192];
    size_t convertOutputLength;
    LayoutConvertProgress progress;
    char progressPartialLine[512];
    size_t progressPartialLineLength;
} StreamData;

typedef struct
{
    char message[256];
    double createdAt;
} ErrorPopup;

typedef struct
{
    ErrorPopup popups[4];
    size_t count;
} ErrorPopupData;

typedef struct
{
    Texture2D imageTexture;
    size_t imageSize;
    size_t imageSizeMin;
    size_t imageSizeMax;
} ImageData;

typedef struct
{
    char *buffer;
    size_t len;
    size_t cap;
} StringBuilder;

typedef struct
{
    char **v;
    size_t len;
    size_t cap;
} ArgvBuilder;

static FontData fontData;
static TextboxData textboxData;
static DropdownData dropdownData;
static TabData tabData;
static ScrollData scrollData;
static StreamData streamData;
static ImageData previewImageData;
static ErrorPopupData errorPopupData;

static Clay_Padding defaultBoxPadding;
static Clay_Padding buttonPadding;
static Clay_Padding dropdownPadding;

static Clay_ElementDeclaration styleFilters;
static Clay_ElementDeclaration styleFilterGroup;
static Clay_ElementDeclaration styleFilterItemGroup;
static Clay_ElementDeclaration styleFilterItem;
static Clay_ElementDeclaration styleFilterItemLabel;
static Clay_ElementDeclaration styleFilterItemField;

static const nfdu8filteritem_t videoFilters[] = {
    {"GIF", "gif"},
    {"MKV", "mkv"},
    {"MOV", "mov"},
    {"MP4", "mp4"},
    {"WEBM", "webm"},
};
static const nfdu8filteritem_t audioFilters[] = {
    {"WAV", "wav,wave"},
    {"MP3", "mp3"},
    {"M4A", "m4a"},
    {"FLAC", "flac"},
    {"OGG", "ogg,oga"},
    {"OPUS", "opus"},
};
static const nfdu8filteritem_t imageFilters[] = {
    {"JPEG", "jpg,jpeg,jpe,jfif"},
    {"PNG", "png"},
    {"TIFF", "tiff,tif"},
    {"WEBP", "webp"},
};

static const char *defaultExtVideo = ".gif";
static const char *defaultExtAudio = ".wav";
static const char *defaultExtImage = ".jpg";

Clay_RenderCommandArray LayoutCreator_CreateLayout();

bool charMatchesAny(char c, const char *matchString)
{
    for (size_t i = 0; matchString[i] != '\0'; i++)
    {
        if (c == matchString[i])
        {
            return true;
        }
    }
    return false;
}

int clampFloatAsString(char *floatAsString, float min, float max)
{
    float clampedVal = atof(floatAsString);
    clampedVal = CLAMP(clampedVal, min, max);

    int charsWritten = snprintf(floatAsString, TEXTBOX_BUFFER_SIZE, "%g", clampedVal);

    return charsWritten;
}

const char *trim(const char *str)
{
    size_t i;
    for (i = 0; str[i] != '\0'; i++)
    {
        if (str[i] != ' ')
        {
            break;
        }
    }
    return str + i;
}

bool extensionEqualsIgnoreCase(const char *a, const char *b)
{
    if (a == NULL || b == NULL)
    {
        return false;
    }

    while (*a && *b)
    {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
        {
            return false;
        }
        a++;
        b++;
    }

    return *a == *b;
}

char *ReadFileToString(FILE *fp, size_t *outSize)
{
    if (fp == NULL || outSize == NULL)
    {
        return NULL;
    }

    size_t cap = 4096;
    size_t len = 0;
    char *buffer = malloc(cap);
    if (buffer == NULL)
    {
        return NULL;
    }

    for (;;)
    {
        if (len + 4096 + 1 > cap)
        {
            size_t newCap = cap * 2;
            while (len + 4096 + 1 > newCap)
            {
                newCap *= 2;
            }
            char *tmp = realloc(buffer, newCap);
            if (tmp == NULL)
            {
                free(buffer);
                return NULL;
            }
            buffer = tmp;
            cap = newCap;
        }

        size_t n = fread(buffer + len, 1, 4096, fp);
        if (n > 0)
        {
            len += n;
        }

        if (n < 4096)
        {
            break;
        }
    }

    buffer[len] = '\0';
    *outSize = len;
    return buffer;
}

const char *getTextboxValue(TextboxID textboxId)
{
    if (textboxData.textboxBuffers[textboxId].length > 0)
    {
        return textboxData.textboxBuffers[textboxId].chars;
    }
    return textboxData.textboxBuffers[textboxId].charsDefault;
}

static const char *sanitizeErrorMessage(const char *message)
{
    if (message == NULL || message[0] == '\0')
    {
        return "An unexpected error occurred.";
    }

    static char buffer[256];
    size_t out = 0;
    const char *p = message;

    while (*p != '\0' && out + 1 < sizeof(buffer))
    {
        if (*p == '\r')
        {
            p++;
            continue;
        }
        if (*p == '\n')
        {
            break;
        }
        if (*p == '\x1b')
        {
            while (*p != '\0' && *p != 'm')
            {
                p++;
            }
            if (*p == 'm')
            {
                p++;
            }
            continue;
        }
        buffer[out++] = *p;
        p++;
    }
    buffer[out] = '\0';

    char *trimmed = buffer;
    while (*trimmed == ' ' || *trimmed == '\t')
    {
        trimmed++;
    }

    if (strstr(trimmed, "height") != NULL && strstr(trimmed, "divisible by 2") != NULL)
    {
        return "The selected output height must be divisible by 2 for this conversion.";
    }
    if (strstr(trimmed, "Invalid argument") != NULL)
    {
        return "The requested conversion settings could not be applied.";
    }
    if (strstr(trimmed, "Error") != NULL || strstr(trimmed, "error") != NULL)
    {
        return trimmed;
    }

    return trimmed;
}

static void removeErrorPopup(size_t index)
{
    if (index >= errorPopupData.count)
    {
        return;
    }

    for (size_t i = index; i + 1 < errorPopupData.count; i++)
    {
        errorPopupData.popups[i] = errorPopupData.popups[i + 1];
    }
    errorPopupData.count--;
}

void HandleErrorPopupCloseButtonInteraction(
    Clay_ElementId elementId,
    Clay_PointerData pointerData,
    intptr_t userData)
{
    if (pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        removeErrorPopup((size_t)userData);
    }
}

void reportErrorf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char message[512];
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    const char *displayMessage = sanitizeErrorMessage(message);
    if (errorPopupData.count < sizeof(errorPopupData.popups) / sizeof(errorPopupData.popups[0]))
    {
        errorPopupData.popups[errorPopupData.count++] = (ErrorPopup){0};
        snprintf(errorPopupData.popups[errorPopupData.count - 1].message, sizeof(errorPopupData.popups[errorPopupData.count - 1].message), "%s", displayMessage);
        errorPopupData.popups[errorPopupData.count - 1].createdAt = GetTime();
    }
    else
    {
        for (size_t i = 1; i < errorPopupData.count; i++)
        {
            errorPopupData.popups[i - 1] = errorPopupData.popups[i];
        }
        snprintf(errorPopupData.popups[errorPopupData.count - 1].message, sizeof(errorPopupData.popups[errorPopupData.count - 1].message), "%s", displayMessage);
        errorPopupData.popups[errorPopupData.count - 1].createdAt = GetTime();
    }
}

static void updateErrorPopups(void)
{
    double now = GetTime();
    size_t activeCount = 0;

    for (size_t i = 0; i < errorPopupData.count; i++)
    {
        if (now - errorPopupData.popups[i].createdAt < 10.0)
        {
            errorPopupData.popups[activeCount++] = errorPopupData.popups[i];
        }
    }

    errorPopupData.count = activeCount;
}

const char *getDropdownValue(DropdownID dropdownId)
{
    return dropdownData.selectedValues[dropdownId];
}

const char *GetFilePathWithoutExt(const char *filePath)
{
    static char buffer[TEXTBOX_BUFFER_SIZE] = {0};
    strncpy(buffer, filePath, sizeof(buffer) - 1);

    // Find last '.' and strip it (but only if it’s after the last slash)
    char *dot = strrchr(buffer, '.');
    char *slash = strrchr(buffer, '/');
#if defined(_WIN32)
    char *backslash = strrchr(buffer, '\\');
    if (!slash || (backslash && backslash > slash))
    {
        slash = backslash;
    }
#endif

    if (dot && (!slash || dot > slash))
    {
        *dot = '\0';
    }

    return buffer;
}

void sbInit(StringBuilder *sb)
{
    sb->buffer = NULL;
    sb->len = 0;
    sb->cap = 0;
}

void sbReserve(StringBuilder *sb, size_t extra)
{
    if (sb->len + extra + 1 <= sb->cap)
    {
        return;
    }

    size_t new_cap = sb->cap ? sb->cap * 2 : 128;
    while (new_cap < sb->len + extra + 1)
    {
        new_cap *= 2;
    }

    sb->buffer = realloc(sb->buffer, new_cap);
    sb->cap = new_cap;
}

void sbAppend(StringBuilder *sb, const char *s)
{
    size_t n = strlen(s);
    sbReserve(sb, n);
    memcpy(sb->buffer + sb->len, s, n);
    sb->len += n;
    sb->buffer[sb->len] = '\0';
}

void sbAppendf(StringBuilder *sb, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    sbReserve(sb, n);

    va_start(ap, fmt);
    vsnprintf(sb->buffer + sb->len, n + 1, fmt, ap);
    va_end(ap);

    sb->len += n;
}

void sbFree(StringBuilder *sb)
{
    free(sb->buffer);
}

void argvInit(ArgvBuilder *a)
{
    a->v = NULL;
    a->len = 0;
    a->cap = 0;
}

void argvPush(ArgvBuilder *a, char *arg)
{
    if (a->len + 1 >= a->cap)
    {
        a->cap = a->cap ? a->cap * 2 : 8;
        a->v = realloc(a->v, a->cap * sizeof(char *));
    }
    a->v[a->len++] = arg;
    a->v[a->len] = NULL;
}

char *argprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    char *buffer = malloc(n + 1);

    va_start(ap, fmt);
    vsnprintf(buffer, n + 1, fmt, ap);
    va_end(ap);

    return buffer;
}

void argvFree(ArgvBuilder *a)
{
    for (size_t i = 0; i < a->len; i++)
    {
        free(a->v[i]);
    }
    free(a->v);
}

static char *buildCmdline(char *const argv[])
{
    size_t cap = 256;
    size_t len = 0;
    char *cmd = malloc(cap);

    for (size_t i = 0; argv[i] != NULL; i++)
    {
        const char *arg = argv[i];
        int need_quotes = strpbrk(arg, " \t\"") != NULL;

        if (len + strlen(arg) + 4 >= cap)
        {
            cap *= 2;
            cmd = realloc(cmd, cap);
        }

        if (i > 0)
        {
            cmd[len++] = ' ';
        }
        if (!need_quotes)
        {
            len += sprintf(cmd + len, "%s", arg);
        }
        else
        {
            cmd[len++] = '"';
            for (const char *p = arg; *p; p++)
            {
                if (*p == '"')
                    cmd[len++] = '\\';
                cmd[len++] = *p;
            }
            cmd[len++] = '"';
        }
    }

    cmd[len] = '\0';
    return cmd;
}

int childCreate(ChildProcessData *c, char *argv[])
{
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE outRead, outWrite;
    if (!CreatePipe(&outRead, &outWrite, &sa, 0))
    {
        ERROR("CreatePipe failed (%d).", GetLastError());
        return GetLastError();
    }

    SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = outWrite;
    si.hStdError = outWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    char *cmdline = buildCmdline(argv);
    LOG("cmd = \"%s\"", cmdline);

    BOOL ok = CreateProcess(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

    free(cmdline);
    CloseHandle(outWrite);

    if (!ok)
    {
        ERROR("CreateProcess failed (%d).", GetLastError());
        CloseHandle(outRead);
        return GetLastError();
    }

    CloseHandle(pi.hThread);

    c->process = pi.hProcess;
    c->stdoutRead = outRead;
    c->valid = true;

    return 0;
#else
    int pipefd[2];
    if (pipe(pipefd) < 0)
    {
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid == 0)
    {
        /* child */
        close(pipefd[0]);

        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        execvp(argv[0], argv);
        _exit(127);
    }

    /* parent */
    close(pipefd[1]);

    /* make read end non-blocking */
    int flags = fcntl(pipefd[0], F_GETFL, 0);
    fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

    c->pid = pid;
    c->stdoutfd = pipefd[0];
    c->valid = true;

    return 0;
#endif
}

void childReset(ChildProcessData *c)
{
    if (c == NULL)
    {
        return;
    }
#ifdef _WIN32
    if (c->process != NULL)
    {
        CloseHandle(c->process);
    }
    if (c->stdoutRead != NULL)
    {
        CloseHandle(c->stdoutRead);
    }
#else
    if (c->stdoutfd >= 0)
    {
        close(c->stdoutfd);
    }
#endif
    memset(c, 0, sizeof(*c));
}

int childKill(ChildProcessData *c)
{
#ifdef _WIN32
    if (c == NULL || c->process == NULL)
    {
        return -1;
    }

    /* Check if already exited */
    DWORD r = WaitForSingleObject(c->process, 0);
    if (r == WAIT_OBJECT_0)
    {
        return 0;
    }

    /* Force termination */
    if (!TerminateProcess(c->process, 1))
    {
        return -1;
    }

    childReset(c);

    return 0;
#else
    if (c == NULL || c->pid <= 0)
    {
        return -1;
    }

    /* SIGKILL cannot be ignored */
    if (kill(c->pid, SIGKILL) < 0)
    {
        if (errno == ESRCH)
        {
            return 0; /* already dead */
        }
        return -1;
    }

    /* Reap to avoid zombie (non-blocking) */
    waitpid(c->pid, NULL, WNOHANG);

    childReset(c);

    return 0;
#endif
}

int childPoll(ChildProcessData *c)
{
#ifdef _WIN32
    DWORD avail = 0;
    if (!PeekNamedPipe(c->stdoutRead, NULL, 0, NULL, &avail, NULL))
    {
        if (GetLastError() == ERROR_BROKEN_PIPE)
        {
            return 0;
        }
        ERROR("PeekNamedPipe failed (%d).", GetLastError());
        return GetLastError();
    }
    return avail > 0 ? 1 : 0;
#else
    fd_set rfds;
    struct timeval tv = {0, 0};

    FD_ZERO(&rfds);
    FD_SET(c->stdoutfd, &rfds);

    // LOG("%d", c->stdoutfd);
    // int r = select(c->stdoutfd + 1, &rfds, NULL, NULL, NULL);
    int r = select(c->stdoutfd + 1, &rfds, NULL, NULL, &tv);
    if (r < 0)
    {
        return -1;
    }

    return FD_ISSET(c->stdoutfd, &rfds) ? 1 : 0;
#endif
}

int childRead(ChildProcessData *c, char *buffer, size_t len)
{
#ifdef _WIN32
    DWORD read = 0;
    if (!ReadFile(c->stdoutRead, buffer, len, &read, NULL))
    {
        if (GetLastError() == ERROR_BROKEN_PIPE)
        {
            return 0; /* EOF */
        }
        ERROR("ReadFile failed (%d).", GetLastError());
        return GetLastError();
    }
    return read;
#else
    ssize_t r = read(c->stdoutfd, buffer, len);
    if (r < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return 0;
        }
        return -1;
    }
    return r;
#endif
}

char *childReadAll(ChildProcessData *c, size_t *outSize)
{
    if (!c || !outSize)
    {
        ERROR("childReadAll(): NULL c or outSize");
        return NULL;
    }

    size_t cap = 4096;
    size_t len = 0;

    char *out = malloc(cap + 1);
    if (!out)
    {
        ERROR("childReadAll(): failed init malloc");
        return NULL;
    }

#ifndef _WIN32
    int flags = fcntl(c->stdoutfd, F_GETFL, 0);
    if (flags < 0)
    {
        free(out);
        return NULL;
    }

    int origFlags = flags;
    if (fcntl(c->stdoutfd, F_SETFL, flags & ~O_NONBLOCK) < 0)
    {
        free(out);
        return NULL;
    }
#endif

    for (;;)
    {
        char buf[4096];

#ifdef _WIN32
        DWORD nread = 0;

        BOOL ok = ReadFile(
            c->stdoutRead,
            buf,
            sizeof(buf),
            &nread,
            NULL);

        if (!ok)
        {
            DWORD err = GetLastError();

            if (err == ERROR_BROKEN_PIPE)
                break; /* EOF */

            free(out);
            return NULL;
        }

        if (nread == 0)
            break;

        size_t n = (size_t)nread;

#else
        ssize_t nread = read(c->stdoutfd, buf, sizeof(buf));

        if (nread < 0)
        {
            if (errno == EINTR)
                continue;

            free(out);
            ERROR("childReadAll(): read() error");
            perror("ffmpeg");
#ifndef _WIN32
            if (fcntl(c->stdoutfd, F_SETFL, origFlags) < 0)
            {
                ERROR("childReadAll(): failed to restore fd flags");
            }
#endif
            return NULL;
        }

        if (nread == 0)
            break; /* EOF */

        size_t n = (size_t)nread;

#endif

        /* grow output buffer if needed */
        if (len + n + 1 > cap)
        {
            while (len + n + 1 > cap)
                cap *= 2;

            char *tmp = realloc(out, cap + 1);
            if (!tmp)
            {
                free(out);
                ERROR("childReadAll(): failed out resize");
#ifndef _WIN32
                if (fcntl(c->stdoutfd, F_SETFL, origFlags) < 0)
                {
                    ERROR("childReadAll(): failed to restore fd flags");
                }
#endif
                return NULL;
            }

            out = tmp;
        }

        memcpy(out + len, buf, n);
        len += n;
    }

#ifndef _WIN32
    if (fcntl(c->stdoutfd, F_SETFL, origFlags) < 0)
    {
        ERROR("childReadAll(): failed to restore fd flags");
    }
#endif

    out[len] = '\0';
    *outSize = len;

    return out;
}

int childExited(ChildProcessData *c)
{
#ifdef _WIN32
    DWORD r = WaitForSingleObject(c->process, 0);
    if (r == WAIT_OBJECT_0)
    {
        return 1;
    }
    if (r == WAIT_TIMEOUT)
    {
        return 0;
    }
    return -1;
#else
    int status;
    pid_t r = waitpid(c->pid, &status, WNOHANG);
    if (r < 0)
    {
        return -1;
    }
    return r > 0 ? 1 : 0;
#endif
}

static int ValidateSubtitleFile(const char *subtitlePath)
{
    if (subtitlePath == NULL || trim(subtitlePath)[0] == '\0')
    {
        ERROR("ValidateSubtitleFile(): subtitle path is empty.");
        reportErrorf("Please select a subtitle file.");
        return 1;
    }

    if (!FileExists(subtitlePath))
    {
        ERROR("ValidateSubtitleFile(): subtitle file does not exist (\"%s\").", subtitlePath);
        reportErrorf("The selected subtitle file does not exist.");
        return 2;
    }

    char buffer[4096];
    int ret = snprintf(
        buffer,
        sizeof(buffer),
        "ffprobe -v error -show_streams -of json \"%s\"",
        subtitlePath);

    if (ret < 0 || ret >= sizeof(buffer))
    {
        ERROR("ValidateSubtitleFile(): failed to format ffprobe command.");
        reportErrorf("Unable to prepare subtitle validation.");
        return 3;
    }

    FILE *fp = popen(buffer, "r");
    if (fp == NULL)
    {
        ERROR("ValidateSubtitleFile(): failed to execute ffprobe for subtitle validation.");
        reportErrorf("Unable to validate the subtitle file.");
        return 4;
    }

    size_t jsonSize = 0;
    char *jsonText = ReadFileToString(fp, &jsonSize);
    int pcloseRet = pclose(fp);
    if (jsonText == NULL)
    {
        ERROR("ValidateSubtitleFile(): failed to read ffprobe output for subtitle validation.");
        reportErrorf("Unable to read subtitle metadata.");
        return 5;
    }
    if (pcloseRet != 0)
    {
        ERROR("ValidateSubtitleFile(): ffprobe exited with non-zero status for subtitle validation (%d).", pcloseRet);
        reportErrorf("The subtitle file could not be validated.");
        free(jsonText);
        return 6;
    }

    cJSON *json = cJSON_ParseWithLength(jsonText, jsonSize);
    free(jsonText);
    if (json == NULL)
    {
        ERROR("ValidateSubtitleFile(): failed to parse subtitle metadata JSON.");
        reportErrorf("The subtitle file metadata could not be parsed.");
        return 7;
    }

    cJSON *streams = cJSON_GetObjectItemCaseSensitive(json, "streams");
    if (streams == NULL)
    {
        ERROR("ValidateSubtitleFile(): subtitle metadata JSON is missing the streams array.");
        reportErrorf("The subtitle file metadata could not be read.");
        cJSON_Delete(json);
        return 8;
    }

    bool hasSubtitleStream = false;
    cJSON *stream = NULL;
    cJSON_ArrayForEach(stream, streams)
    {
        cJSON *codecType = cJSON_GetObjectItemCaseSensitive(stream, "codec_type");
        if (cJSON_IsString(codecType) && codecType->valuestring != NULL && strcmp(codecType->valuestring, "subtitle") == 0)
        {
            hasSubtitleStream = true;
            break;
        }
    }

    cJSON_Delete(json);
    if (!hasSubtitleStream)
    {
        ERROR("ValidateSubtitleFile(): subtitle file contains no subtitle stream (\"%s\").", subtitlePath);
        reportErrorf("The selected subtitle file contains no subtitle stream.");
        return 9;
    }

    return 0;
}

static void copyGuiStateToModel(LayoutModel *model)
{
    LayoutModel_Init(model);

    for (size_t i = 0; i < TEXTBOX_ID_DUMMY_LAST; i++)
    {
        LayoutTextboxBuffer *dst = &model->textboxData.textboxBuffers[i];
        TextboxBuffer *src = &textboxData.textboxBuffers[i];
        memcpy(dst->chars, src->chars, sizeof(dst->chars));
        dst->charsDefault = src->charsDefault;
        dst->length = src->length;
        dst->cursorPosition = src->cursorPosition;
        dst->numberboxConfig = (LayoutNumberboxConfig){
            .isNumberbox = src->numberboxConfig.isNumberbox,
            .isInt = src->numberboxConfig.isInt,
            .min = src->numberboxConfig.min,
            .max = src->numberboxConfig.max,
        };
        model->textboxData.isInit[i] = textboxData.isInit[i];
        model->textboxData.isEnabled[i] = textboxData.isEnabled[i];
    }

    model->textboxData.hoveredTextbox = textboxData.hoveredTextbox;
    model->textboxData.focusData.focusRegistered = textboxData.focusData.focusRegistered;
    model->textboxData.focusData.focusIndex = textboxData.focusData.focusIndex;
    model->textboxData.focusData.focusStartTime = textboxData.focusData.focusStartTime;

    for (size_t i = 0; i < DROPDOWN_ID_DUMMY_LAST; i++)
    {
        model->dropdownData.selectedOptions[i] = dropdownData.selectedOptions[i];
        model->dropdownData.selectedValues[i] = dropdownData.selectedValues[i];
        model->dropdownData.isInit[i] = dropdownData.isInit[i];
    }
    model->dropdownData.hoveredOption = dropdownData.hoveredOption;
    model->dropdownData.hoveredValue = dropdownData.hoveredValue;

    model->tabData.selectedTab = tabData.selectedTab;
    for (size_t i = 0; i < TAB_ID_DUMMY_LAST; i++)
    {
        model->tabData.isDisabled[i] = tabData.isDisabled[i];
    }

    memcpy(model->streamData.inputPath, streamData.inputPath, sizeof(model->streamData.inputPath));
    for (size_t i = 0; i < STREAM_ID_DUMMY_LAST; i++)
    {
        model->streamData.streamCounts[i] = streamData.streamCounts[i];
    }
    model->streamData.inputDurationSeconds = streamData.inputDurationSeconds;
    memcpy(model->streamData.convertOutput, streamData.convertOutput, sizeof(model->streamData.convertOutput));
    model->streamData.convertOutputLength = streamData.convertOutputLength;
    model->streamData.progress = streamData.progress;
    memcpy(model->streamData.progressPartialLine, streamData.progressPartialLine, sizeof(model->streamData.progressPartialLine));
    model->streamData.progressPartialLineLength = streamData.progressPartialLineLength;

    model->previewImageData.imageSize = previewImageData.imageSize;
    model->previewImageData.imageSizeMin = previewImageData.imageSizeMin;
    model->previewImageData.imageSizeMax = previewImageData.imageSizeMax;

    model->errorPopupData.count = errorPopupData.count;
    for (size_t i = 0; i < errorPopupData.count; i++)
    {
        memcpy(model->errorPopupData.popups[i].message, errorPopupData.popups[i].message, sizeof(model->errorPopupData.popups[i].message));
        model->errorPopupData.popups[i].createdAt = errorPopupData.popups[i].createdAt;
    }
}

static void copyModelStreamToGui(const LayoutModel *model)
{
    memcpy(streamData.inputPath, model->streamData.inputPath, sizeof(streamData.inputPath));
    for (size_t i = 0; i < STREAM_ID_DUMMY_LAST; i++)
    {
        streamData.streamCounts[i] = model->streamData.streamCounts[i];
    }
    streamData.inputDurationSeconds = model->streamData.inputDurationSeconds;
    memcpy(streamData.convertOutput, model->streamData.convertOutput, sizeof(streamData.convertOutput));
    streamData.convertOutputLength = model->streamData.convertOutputLength;
    streamData.progress = model->streamData.progress;
    memcpy(streamData.progressPartialLine, model->streamData.progressPartialLine, sizeof(streamData.progressPartialLine));
    streamData.progressPartialLineLength = model->streamData.progressPartialLineLength;
}

static void copyModelTextboxesToGui(const LayoutModel *model)
{
    for (size_t i = 0; i < TEXTBOX_ID_DUMMY_LAST; i++)
    {
        const LayoutTextboxBuffer *src = &model->textboxData.textboxBuffers[i];
        TextboxBuffer *dst = &textboxData.textboxBuffers[i];
        memcpy(dst->chars, src->chars, sizeof(dst->chars));
        dst->charsDefault = src->charsDefault;
        dst->length = src->length;
        dst->cursorPosition = src->cursorPosition;
        dst->numberboxConfig = (NumberboxConfig){
            .isNumberbox = src->numberboxConfig.isNumberbox,
            .isInt = src->numberboxConfig.isInt,
            .min = src->numberboxConfig.min,
            .max = src->numberboxConfig.max,
        };
        textboxData.isInit[i] = model->textboxData.isInit[i];
        textboxData.isEnabled[i] = model->textboxData.isEnabled[i];
    }
    textboxData.focusData.focusRegistered = model->textboxData.focusData.focusRegistered;
    textboxData.focusData.focusIndex = model->textboxData.focusData.focusIndex;
    textboxData.focusData.focusStartTime = model->textboxData.focusData.focusStartTime;
}

static bool convertFileExists(const char *path, void *userData)
{
    (void)userData;
    return FileExists(path);
}

static bool convertDirectoryExists(const char *path, void *userData)
{
    (void)userData;
    return DirectoryExists(path);
}

static bool convertIsFileNameValid(const char *name, void *userData)
{
    (void)userData;
    return IsFileNameValid(name);
}

static int convertValidateSubtitleFile(const char *path, void *userData)
{
    (void)userData;
    return ValidateSubtitleFile(path);
}

int convert()
{
    streamData.convertOutputLength = 0;
    streamData.convertOutput[0] = '\0';

    LayoutModel model;
    ConvertPlan plan;
    char error[256];
    GuiPlatform platform = {
        .fileExists = convertFileExists,
        .directoryExists = convertDirectoryExists,
        .isFileNameValid = convertIsFileNameValid,
        .validateSubtitleFile = convertValidateSubtitleFile,
        .userData = NULL,
    };
    copyGuiStateToModel(&model);

    int ret = LayoutModel_BuildConvertPlan(&model, &platform, &plan, error, sizeof(error));
    if (ret)
    {
        ERROR("Convert validation failed: %s", error);
        reportErrorf("%s", error);
        return ret;
    }

    char *cmdline = buildCmdline(plan.argv);
    LOG("cmd = \"%s\"", cmdline);
    free(cmdline);

    LayoutModel_BeginConvertProgress(&model);
    copyModelStreamToGui(&model);

    ret = childCreate(&streamData.convertProcess, plan.argv);
    LayoutModel_FreeConvertPlan(&plan);
    if (ret)
    {
        ERROR("Failed to create child process (%d)", ret);
        LayoutModel_ResetConvertProgress(&model);
        copyModelStreamToGui(&model);
        return ret;
    }

    return 0;
}

void HandleTextboxInteraction(Clay_ElementId elementId,
                              Clay_PointerData pointerData,
                              intptr_t userData)
{
    size_t index = (size_t)userData;
    if (/*!textboxData.isDisabled[index] &&*/ pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        textboxData.textboxBuffers[index].cursorPosition = textboxData.textboxBuffers[index].length;

        textboxData.focusData.focusRegistered = true;
        textboxData.focusData.focusIndex = index;
        textboxData.focusData.focusStartTime = GetTime();
    }
}

void HandleLoadInputButtonInteraction(
    Clay_ElementId elementId,
    Clay_PointerData pointerData,
    intptr_t userData)
{
    if (pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        char buffer[4096];

        int ret = snprintf(
            buffer,
            sizeof(buffer),
            "ffprobe -v error -show_streams -show_format -of json \"%s\"",
            getTextboxValue(TEXTBOX_ID_INPUT_PATH));

        if (ret < 0 || ret >= sizeof(buffer))
        {
            ERROR("Failed to write command into buffer.");
            return;
        }

        FILE *fp = popen(buffer, "r");
        if (fp == NULL)
        {
            ERROR("Failed to execute command and establish pipe.");
            reportErrorf("Unable to probe the selected input file.");
            return;
        }

        size_t jsonSize = 0;
        char *jsonText = ReadFileToString(fp, &jsonSize);
        int pcloseRet = pclose(fp);
        if (jsonText == NULL)
        {
            ERROR("Failed to read ffprobe output.");
            reportErrorf("Unable to read metadata from the input file.");
            return;
        }
        if (pcloseRet != 0)
        {
            ERROR("ffprobe exited with non-zero status (%d).", pcloseRet);
            reportErrorf("Unable to read metadata from the input file.");
        }

        LayoutModel model;
        char parseError[256];
        copyGuiStateToModel(&model);
        int parseRet = LayoutModel_ParseStreamsJson(&model, jsonText, jsonSize, parseError, sizeof(parseError));
        free(jsonText);
        if (parseRet != 0)
        {
            ERROR("Failed to parse input metadata: %s", parseError);
            reportErrorf("An unexpected error occurred while reading input metadata.");
            return;
        }

        if (streamData.convertProcess.valid)
        {
            childKill(&streamData.convertProcess);
        }
        if (previewImageData.imageTexture.id != 0)
        {
            UnloadTexture(previewImageData.imageTexture);
            previewImageData.imageTexture.id = 0;
        }

        memset(&streamData, 0, sizeof(streamData));
        for (size_t i = 0; i < STREAM_ID_DUMMY_LAST; i++)
        {
            streamData.streamCounts[i] = model.streamData.streamCounts[i];
        }
        streamData.inputDurationSeconds = model.streamData.inputDurationSeconds;
        LOG("v: %zu, a: %zu, s: %zu", streamData.streamCounts[STREAM_ID_VIDEO], streamData.streamCounts[STREAM_ID_AUDIO], streamData.streamCounts[STREAM_ID_SUBTITLES]);

        snprintf(streamData.inputPath, sizeof(streamData.inputPath), "%s", trim(getTextboxValue(TEXTBOX_ID_INPUT_PATH)));
        dropdownData.isInit[DROPDOWN_ID_OUTPUT_TYPE] = false;
    }
}

void HandleLoadPreviewButtonInteraction(
    Clay_ElementId elementId,
    Clay_PointerData pointerData,
    intptr_t userData)
{
    if (pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        int logCounter = 0;

        char buffer[4096];

        int ret = snprintf(
            buffer,
            sizeof(buffer),
            "ffprobe -v error -select_streams v:0 -show_entries stream=width,height -of csv=p=0 \"%s\"",
            streamData.inputPath);

        if (ret < 0 || ret >= sizeof(buffer))
        {
            ERROR("Failed to write command into buffer.");
            return;
        }
        // LOG("%s", buffer);

        FILE *fp = popen(buffer, "r");
        if (fp == NULL)
        {
            ERROR("Failed to execute command and establish pipe.");
            reportErrorf("Unable to probe preview image dimensions.");
            return;
        }

        int width;
        int height;
        ret = fscanf(fp, "%d,%d", &width, &height);
        pclose(fp);
        if (ret != 2)
        {
            ERROR("Failed to read command output.");
            reportErrorf("Unable to read preview metadata from the input file.");
            return;
        }

        int cropWidth = atoi(getTextboxValue(TEXTBOX_ID_CROP_W));
        int cropHeight = atoi(getTextboxValue(TEXTBOX_ID_CROP_H));
        if (cropWidth == 0)
        {
            cropWidth = width;
        }
        if (cropHeight == 0)
        {
            cropHeight = height;
        }

        int cropOffsetX = atoi(getTextboxValue(TEXTBOX_ID_CROP_X));
        int cropOffsetY = atoi(getTextboxValue(TEXTBOX_ID_CROP_Y));

        int scaleWidth = atoi(getTextboxValue(TEXTBOX_ID_SCALE_W));
        int scaleHeight = atoi(getTextboxValue(TEXTBOX_ID_SCALE_H));
        if (scaleWidth == -1 && scaleHeight == -1)
        {
            ERROR("Both scale dimensions are -1.");
            reportErrorf("Please enter at least one valid preview scale dimension.");
            return;
        }
        if (scaleWidth == -1)
        {
            scaleWidth = cropWidth * scaleHeight / cropHeight;
        }
        else if (scaleWidth == 0)
        {
            scaleWidth = cropWidth;
        }
        if (scaleHeight == -1)
        {
            scaleHeight = cropHeight * scaleWidth / cropWidth;
        }
        else if (scaleHeight == 0)
        {
            scaleHeight = cropHeight;
        }

        ArgvBuilder a;
        argvInit(&a);
        argvPush(&a, strdup("ffmpeg"));
        argvPush(&a, strdup("-v"));
        argvPush(&a, strdup("error"));
        argvPush(&a, strdup("-i"));
        argvPush(&a, strdup(streamData.inputPath));
        argvPush(&a, strdup("-vf"));
        argvPush(&a, argprintf("crop=%d:%d:%d:%d,scale=%d:%d", cropWidth, cropHeight, cropOffsetX, cropOffsetY, scaleWidth, scaleHeight));
        argvPush(&a, strdup("-vframes"));
        argvPush(&a, strdup("1"));
        argvPush(&a, strdup("-f"));
        argvPush(&a, strdup("rawvideo"));
        argvPush(&a, strdup("-pix_fmt"));
        argvPush(&a, strdup("rgba"));
        argvPush(&a, strdup("-"));

        char *cmd = buildCmdline(a.v);
        // LOG("%s %s", a.v[0], cmd);
        free(cmd);

        // LOG("1");
        ChildProcessData imageProcess = {0};
        ret = childCreate(&imageProcess, a.v);
        // LOG("2");
        argvFree(&a);
        if (ret)
        {
            ERROR("Failed to create child process (%d)", ret);
            reportErrorf("Unable to generate preview image.");
            return;
        }

        // LOG("3");
        if (scaleWidth <= 0 || scaleHeight <= 0)
        {
            ERROR("Invalid preview dimensions (%d x %d)", scaleWidth, scaleHeight);
            childReset(&imageProcess);
            reportErrorf("Invalid preview dimensions.");
            return;
        }

        uint64_t pixelCount = (uint64_t)scaleWidth * (uint64_t)scaleHeight;
        if (pixelCount > 10000ULL * 10000ULL)
        {
            ERROR("Preview dimensions too large (%d x %d)", scaleWidth, scaleHeight);
            childReset(&imageProcess);
            reportErrorf("Preview size is too large.");
            return;
        }

        size_t imageBufferSize = (size_t)pixelCount * 4;
        size_t totalBytesRead = 0;

        // LOG("4");
        unsigned char *imageBuffer = childReadAll(&imageProcess, &totalBytesRead);
        childReset(&imageProcess);

        if (imageBuffer == NULL)
        {
            ERROR("NULL imageBuffer");
            reportErrorf("Unable to allocate memory for preview image.");
            return;
        }
        if (totalBytesRead != imageBufferSize)
        {
            ERROR("Total bytes read (%zu) less than image buffer size (%zu)", totalBytesRead, imageBufferSize);
            reportErrorf("Unable to read the full preview image data.");
            LOG("%s", imageBuffer);
            free(imageBuffer);
            return;
        }

        // LOG("5");
        Image image = {
            .data = imageBuffer,
            .width = scaleWidth,
            .height = scaleHeight,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            .mipmaps = 1,
        };

        Texture2D newTexture = LoadTextureFromImage(image);
        if (newTexture.id == 0)
        {
            ERROR("Failed to load texture from image.");
            reportErrorf("Unable to create preview texture.");
            UnloadImage(image);
            return;
        }

        if (previewImageData.imageTexture.id != 0)
        {
            UnloadTexture(previewImageData.imageTexture);
        }
        previewImageData.imageTexture = newTexture;
        // LOG("6");
        UnloadImage(image);
    }
}

void HandlePreviewSizeDownButtonInteraction(
    Clay_ElementId elementId,
    Clay_PointerData pointerData,
    intptr_t userData)
{
    if (pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        previewImageData.imageSize = CLAMP(previewImageData.imageSize - 100, previewImageData.imageSizeMin, previewImageData.imageSizeMax);
    }
}

void HandlePreviewSizeUpButtonInteraction(
    Clay_ElementId elementId,
    Clay_PointerData pointerData,
    intptr_t userData)
{
    if (pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        previewImageData.imageSize = CLAMP(previewImageData.imageSize + 100, previewImageData.imageSizeMin, previewImageData.imageSizeMax);
    }
}

void HandleBrowseButtonInteraction(
    Clay_ElementId elementId,
    Clay_PointerData pointerData,
    intptr_t userData)
{
    if (pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        size_t index = (size_t)userData;

        nfdu8char_t *outPath;
        nfdresult_t result;

        switch (index)
        {
        case TEXTBOX_ID_INPUT_PATH:
            nfdu8filteritem_t inputFilters[] = {
                {"Video", "mp4,mov,mkv,webm,flv,mpeg,gif"},
                {"GIF", "gif"},
            };
            nfdopendialogu8args_t inputOpenDialogArgs = {
                .filterList = inputFilters,
                .filterCount = 2,
            };
            result = NFD_OpenDialogU8_With(&outPath, &inputOpenDialogArgs);
            break;

        case TEXTBOX_ID_SUBTITLES_SOURCE:
            nfdu8filteritem_t subtitleFilters[] = {
                {"Text Subtitle", "srt,vtt,ssa,ass,sub,smi,stl,rt,mpsub,aqt,pts,jss"},
                {"Video Container", "mp4,mkv,mov,m4v"},
            };
            nfdopendialogu8args_t subititleOpenDialogArgs = {
                .filterList = subtitleFilters,
                .filterCount = 2,
            };
            result = NFD_OpenDialogU8_With(&outPath, &subititleOpenDialogArgs);
            break;

        case TEXTBOX_ID_OUTPUT_PATH:
            char outputType = getDropdownValue(DROPDOWN_ID_OUTPUT_TYPE)[0];
            const nfdu8filteritem_t *outputFilters;
            nfdfiltersize_t filterCount;
            switch (outputType)
            {
            case 'v':
                outputFilters = videoFilters;
                filterCount = sizeof(videoFilters) / sizeof(videoFilters[0]);
                break;

            case 'a':
                outputFilters = audioFilters;
                filterCount = sizeof(audioFilters) / sizeof(audioFilters[0]);
                break;

            case 'i':
                outputFilters = imageFilters;
                filterCount = sizeof(imageFilters) / sizeof(imageFilters[0]);
                break;

            default:
                ERROR("Invalid output type selected (%c)", outputType);
                reportErrorf("Unable to open save dialog for the selected output type.");
                return;
                break;
            }

            nfdsavedialogu8args_t saveDialogArgs = {
                .filterList = outputFilters,
                .filterCount = filterCount,
                .defaultName = "Untitled",
            };
            result = NFD_SaveDialogU8_With(&outPath, &saveDialogArgs);
            break;

        default:
            ERROR("Invalid index = %zu (HandleBrowseButtonInteraction)", index);
            return;
            break;
        }

        if (result == NFD_OKAY)
        {
            TextboxBuffer *buffer = &textboxData.textboxBuffers[index];
            strncpy(buffer->chars, outPath, TEXTBOX_BUFFER_SIZE - 1);
            buffer->chars[TEXTBOX_BUFFER_SIZE - 1] = '\0';
            buffer->length = strlen(buffer->chars);
            buffer->cursorPosition = buffer->length;
            NFD_FreePathU8(outPath);
        }
        else if (result == NFD_CANCEL)
        {
            LOG("Browse canceled (index = %zu)", index);
        }
        else
        {
            ERROR("%s", NFD_GetError());
            reportErrorf("A file dialog error occurred. Please try again.");
        }
    }
}

void HandleConvertButtonInteraction(
    Clay_ElementId elementId,
    Clay_PointerData pointerData,
    intptr_t userData)
{
    if (pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        int ret = convert();
        if (ret)
        {
            ERROR("Convert failed (%d)", ret);
        }
    }
}

void HandleConvertCancelButtonInteraction(
    Clay_ElementId elementId,
    Clay_PointerData pointerData,
    intptr_t userData)
{
    if (pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        int ret = childKill(&streamData.convertProcess);
        if (ret)
        {
            ERROR("Convert cancel failed");
            reportErrorf("Unable to cancel conversion.");
        }
    }
}

void HandleDropdownOptionInteraction(
    Clay_ElementId elementId,
    Clay_PointerData pointerData,
    intptr_t userData)
{
    if (pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        size_t index = (size_t)userData;

        dropdownData.selectedOptions[index] = dropdownData.hoveredOption;
        dropdownData.selectedValues[index] = dropdownData.hoveredValue;
    }
}

void HandleTabInteraction(
    Clay_ElementId elementId,
    Clay_PointerData pointerData,
    intptr_t userData)
{
    if (pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        size_t index = (size_t)userData;

        if (!tabData.isDisabled[index])
        {
            tabData.selectedTab = index;
        }
    }
}

void HandleThumbVerticalInteraction(
    Clay_ElementId elementId,
    Clay_PointerData pointerData,
    intptr_t userData)
{
    size_t index = (size_t)userData;

    if (index == SCROLL_ID_DUMMY_LAST)
    {
        scrollData.data[SCROLL_ID_WINDOW].scrollPosition->y -= (GetMouseY() - scrollData.middleClickPosition.y) * 0.5;
    }
    else if (pointerData.state == CLAY_POINTER_DATA_PRESSED)
    {
        Vector2 scrollDelta = GetMouseDelta();
        scrollData.data[index].scrollPosition->y -= scrollDelta.y * scrollData.data[index].contentDimensions.height / scrollData.data[index].scrollContainerDimensions.height;
        scrollData.scrolling = index;
        scrollData.directionLock = DIRECTION_ID_VERTICAL;
    }
    else if (pointerData.state == CLAY_POINTER_DATA_RELEASED)
    {
        scrollData.scrolling = -1;
        scrollData.directionLock = DIRECTION_ID_NONE;
    }
}

void HandleThumbHorizontalInteraction(
    Clay_ElementId elementId,
    Clay_PointerData pointerData,
    intptr_t userData)
{
    size_t index = (size_t)userData;
    if (index == SCROLL_ID_DUMMY_LAST)
    {
        scrollData.data[SCROLL_ID_WINDOW].scrollPosition->x -= (GetMouseX() - scrollData.middleClickPosition.x) * 0.5;
    }
    else if (pointerData.state == CLAY_POINTER_DATA_PRESSED)
    {
        Vector2 scrollDelta = GetMouseDelta();
        scrollData.data[index].scrollPosition->x -= scrollDelta.x * scrollData.data[index].contentDimensions.width / scrollData.data[index].scrollContainerDimensions.width;
        scrollData.scrolling = index;
        scrollData.directionLock = DIRECTION_ID_HORIZONTAL;
    }
    else if (pointerData.state == CLAY_POINTER_DATA_RELEASED)
    {
        scrollData.scrolling = -1;
        scrollData.directionLock = DIRECTION_ID_NONE;
    }
}

void RenderTextbox(
    Clay_String label,
    TextboxID textboxId,
    NumberboxConfig numberboxConfig,
    bool isDisabled,
    size_t maxCharsDisplayed,
    const char *charsDefault)
{
    if (!textboxData.isInit[textboxId])
    {
        textboxData.textboxBuffers[textboxId].numberboxConfig = numberboxConfig;
        textboxData.isInit[textboxId] = true;
    }

    textboxData.textboxBuffers[textboxId].charsDefault = charsDefault;
    textboxData.isEnabled[textboxId] = !isDisabled;

    bool focused = (textboxData.focusData.focusIndex == textboxId);

    CLAY({
        .id = CLAY_IDI("Textbox", textboxId),
        .layout = {
            .childGap = fontData.charWidth,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
        },
    })
    {
        if (label.length > 0)
        {
            CLAY_TEXT(label, TEXT_CONFIG_BOLD);
        }

        uint16_t borderWidth = focused ? 2 : 1;
        CLAY({
            .layout = {
                .sizing = {
                    .width = {
                        .size = {
                            .minMax = {
                                .min = fontData.charWidth * (maxCharsDisplayed + 2),
                            },
                        },
                    },
                },
                .padding = defaultBoxPadding,
            },
            .backgroundColor = isDisabled ? COLOR_BG_TEXTBOX_DISABLED : COLOR_BG_TEXTBOX,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
            .border = {
                .color = focused ? COLOR_BORDER_TEXTBOX_FOCUSED : COLOR_BORDER_TEXTBOX,
                .width = CLAY_BORDER_OUTSIDE(borderWidth),
            },
        })
        {
            if (!isDisabled)
            {
                Clay_OnHover(HandleTextboxInteraction, textboxId);
                if (Clay_Hovered())
                {
                    textboxData.hoveredTextbox = textboxId;
                }
            }

            bool offInterval = (int)(floor((GetTime() - textboxData.focusData.focusStartTime) * 2)) % 2;
            CLAY({
                .border = {
                    .color = offInterval ? COLOR_TRANSPARENT : COLOR_WHITE,
                    .width = focused ? (Clay_BorderWidth){0, 0, 0, 0, 2} : (Clay_BorderWidth){0},
                },
            })
            {
                TextboxBuffer *buffer = &textboxData.textboxBuffers[textboxId];

                if (buffer->length == 0)
                {
                    CLAY_TEXT(CLAY_STRING(""), TEXT_CONFIG_DEFAULT);

                    size_t defaultLength = strlen(charsDefault);
                    if (defaultLength > 0)
                    {
                        Clay_String str = {
                            .isStaticallyAllocated = true,
                            .length = defaultLength,
                            .chars = charsDefault,
                        };
                        CLAY_TEXT(str, TEXT_CONFIG_FAINT);
                    }
                    else
                    {
                        CLAY_TEXT(CLAY_STRING(" "), TEXT_CONFIG_DEFAULT);
                    }
                }
                else
                {
                    size_t displayStart = 0;
                    size_t displayEnd = buffer->length;

                    if (buffer->length > maxCharsDisplayed)
                    {
                        if (buffer->cursorPosition < buffer->length - maxCharsDisplayed)
                        {
                            displayStart = buffer->cursorPosition;
                        }
                        else
                        {
                            displayStart = buffer->length - maxCharsDisplayed;
                        }
                        displayEnd = displayStart + maxCharsDisplayed;
                    }

                    Clay_String textBeforeCursor = {
                        .isStaticallyAllocated = true,
                        .length = buffer->cursorPosition - displayStart,
                        .chars = buffer->chars + displayStart,
                    };
                    Clay_String textAfterCursor = {
                        .isStaticallyAllocated = true,
                        .length = displayEnd - buffer->cursorPosition,
                        .chars = buffer->chars + buffer->cursorPosition,
                    };

                    if (isDisabled)
                    {
                        CLAY_TEXT(textBeforeCursor, TEXT_CONFIG_FAINT);
                        CLAY_TEXT(textAfterCursor, TEXT_CONFIG_FAINT);
                    }
                    else
                    {
                        CLAY_TEXT(textBeforeCursor, TEXT_CONFIG_DEFAULT);
                        CLAY_TEXT(textAfterCursor, TEXT_CONFIG_DEFAULT);
                    }
                }
            }
        }
    }
}

void RenderDropdown(Clay_String label, DropdownID dropdownId, DropdownOption *options)
{
    if (!dropdownData.isInit[dropdownId])
    {
        dropdownData.selectedOptions[dropdownId] = 0;
        dropdownData.selectedValues[dropdownId] = options[0].value;
        dropdownData.isInit[dropdownId] = true;
    }

    bool dropdownHovered = Clay_PointerOver(CLAY_IDI("DropdownButton", dropdownId)) ||
                           Clay_PointerOver(CLAY_IDI("DropdownOptions", dropdownId));

    size_t maxLength = strlen(options[dropdownData.selectedOptions[dropdownId]].label) + 3;
    size_t dropdownSize;
    for (dropdownSize = 0; options[dropdownSize].value != NULL; dropdownSize++)
    {
        size_t optionLength = strlen(options[dropdownSize].label);
        if (optionLength > maxLength)
        {
            maxLength = optionLength;
        }
    }

    CLAY({
        .id = CLAY_IDI("Dropdown", dropdownId),
        .layout = {
            .childGap = fontData.charWidth,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
        },
    })
    {
        if (label.length > 0)
        {
            CLAY_TEXT(label, TEXT_CONFIG_BOLD);
        }

        Clay_CornerRadius buttonCornerRadius;
        Clay_BorderWidth buttonBorderWidth;
        bool expandDropdown = dropdownHovered && dropdownSize > 1;
        if (expandDropdown)
        {
            buttonCornerRadius = (Clay_CornerRadius){8, 8, 0, 0};
            buttonBorderWidth = (Clay_BorderWidth){1, 1, 1, 0, 0};
        }
        else
        {
            buttonCornerRadius = CLAY_CORNER_RADIUS(8);
            buttonBorderWidth = (Clay_BorderWidth)CLAY_BORDER_OUTSIDE(1);
        }

        Clay_Sizing dropdownSizing = {.width = CLAY_SIZING_FIXED(fontData.charWidth * (maxLength + 6))};

        CLAY({
            .id = CLAY_IDI("DropdownButton", dropdownId),
            .layout = {
                .sizing = dropdownSizing,
                .padding = dropdownPadding,
                .childAlignment = {.x = CLAY_ALIGN_X_CENTER},
            },
            .backgroundColor = COLOR_BG_DROPDOWN,
            .cornerRadius = buttonCornerRadius,
            .border = {
                .color = COLOR_BORDER_DROPDOWN,
                .width = buttonBorderWidth,
            },
        })
        {
            const char *chars = options[dropdownData.selectedOptions[dropdownId]].label;
            size_t length = strlen(chars);
            Clay_String str = {
                .isStaticallyAllocated = true,
                .length = length,
                .chars = chars,
            };
            // CLAY({.layout = {.childGap = fontData.charWidth * 2}})
            // {
            CLAY_TEXT(str, TEXT_CONFIG_DEFAULT);
            // CLAY_TEXT(expandDropdown ? CLAY_STRING("▲") : dropdownSize > 1 ? CLAY_STRING("▼")
            //                                                                : CLAY_STRING("▽"),
            //           TEXT_CONFIG_SYMBOL);
            // }

            if (expandDropdown)
            {
                CLAY({
                    .id = CLAY_IDI("DropdownOptions", dropdownId),
                    .layout = {
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .cornerRadius = (Clay_CornerRadius){0, 0, 8, 8},
                    .border = {
                        .color = COLOR_BORDER_DROPDOWN,
                        .width = CLAY_BORDER_ALL(1),
                    },
                    .floating = {
                        .attachPoints = {
                            .element = CLAY_ATTACH_POINT_LEFT_TOP,
                            .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM,
                        },
                        .attachTo = CLAY_ATTACH_TO_PARENT,
                    },
                })
                {
                    for (size_t i = 0; options[i].value != NULL; i++)
                    {
                        if (i != dropdownData.selectedOptions[dropdownId])
                        {
                            bool hovered;

                            CLAY({
                                .layout = {
                                    .sizing = dropdownSizing,
                                    .padding = dropdownPadding,
                                    .childAlignment = {.x = CLAY_ALIGN_X_CENTER},
                                },
                                .backgroundColor = (hovered = Clay_Hovered()) ? COLOR_BG_DROPDOWN_HOVERED : COLOR_BG_DROPDOWN,
                            })
                            {
                                if (hovered)
                                {
                                    dropdownData.hoveredOption = i;
                                    dropdownData.hoveredValue = options[i].value;
                                }

                                Clay_OnHover(HandleDropdownOptionInteraction, dropdownId);

                                const char *chars = options[i].label;
                                size_t length = strlen(chars);
                                Clay_String str = {
                                    .isStaticallyAllocated = true,
                                    .length = length,
                                    .chars = chars,
                                };
                                CLAY_TEXT(str, TEXT_CONFIG_DEFAULT);
                            }
                        }
                    }
                }
            }
        }
    }
}

void RenderButton(
    Clay_String label,
    ButtonID buttonId,
    bool isDisabled,
    void (*onHoverFunction)(
        Clay_ElementId elementId,
        Clay_PointerData pointerData,
        intptr_t userData),
    intptr_t userData,
    Clay_Padding padding)
{
    CLAY({
        .layout = {
            .padding = padding,
        },
        .backgroundColor = /*buttonData.isDisabled[buttonId]*/ isDisabled ? COLOR_BG_BUTTON_DISABLED : Clay_Hovered() ? COLOR_BG_BUTTON_HOVERED
                                                                                                                      : COLOR_BG_BUTTON,
        .cornerRadius = CLAY_CORNER_RADIUS(8),
        .border = {
            .color = COLOR_BORDER_BUTTON,
            .width = CLAY_BORDER_OUTSIDE(1),
        },
    })
    {
        if (!isDisabled)
        {
            Clay_OnHover(onHoverFunction, userData);
        }
        CLAY_TEXT(label, /*buttonData.isDisabled[buttonId]*/ isDisabled ? TEXT_CONFIG_FAINT : TEXT_CONFIG_BOLD);
    }
}

static void formatSeconds(char *buffer, size_t bufferSize, double seconds)
{
    if (seconds < 0.0)
    {
        snprintf(buffer, bufferSize, "--:--");
        return;
    }

    int total = (int)(seconds + 0.5);
    int hours = total / 3600;
    int minutes = (total / 60) % 60;
    int secs = total % 60;
    if (hours > 0)
    {
        snprintf(buffer, bufferSize, "%d:%02d:%02d", hours, minutes, secs);
    }
    else
    {
        snprintf(buffer, bufferSize, "%02d:%02d", minutes, secs);
    }
}

void RenderConvertProgress(void)
{
    LayoutConvertProgress *progress = &streamData.progress;
    const float progressWidth = 360.0f;
    static char lineOne[128];
    static char lineTwo[128];
    static char outTime[32];
    static char totalTime[32];
    static char etaTime[32];
    formatSeconds(outTime, sizeof(outTime), progress->outTimeSeconds);
    formatSeconds(totalTime, sizeof(totalTime), progress->estimatedDurationSeconds);

    if (progress->hasPercent)
    {
        snprintf(
            lineOne,
            sizeof(lineOne),
            "%3.0f%%  Converted %s / %s",
            progress->percent * 100.0,
            outTime,
            totalTime);
    }
    else
    {
        snprintf(
            lineOne,
            sizeof(lineOne),
            "Converted %s",
            outTime);
    }

    if (progress->finished)
    {
        snprintf(lineTwo, sizeof(lineTwo), "%s", "Complete");
    }
    else
    {
        double eta = LayoutModel_EstimateProgressEtaSeconds(progress);
        if (eta >= 0.0)
        {
            formatSeconds(etaTime, sizeof(etaTime), eta);
            snprintf(
                lineTwo,
                sizeof(lineTwo),
                "Processing %.2fx realtime  ETA %s",
                progress->speed > 0.0 ? progress->speed : 0.0,
                etaTime);
        }
        else
        {
            snprintf(
                lineTwo,
                sizeof(lineTwo),
                "Processing %.2fx realtime",
                progress->speed > 0.0 ? progress->speed : 0.0);
        }
    }

    Clay_String lineOneString = {
        .isStaticallyAllocated = false,
        .length = (int32_t)strlen(lineOne),
        .chars = lineOne,
    };
    Clay_String lineTwoString = {
        .isStaticallyAllocated = false,
        .length = (int32_t)strlen(lineTwo),
        .chars = lineTwo,
    };

    CLAY({
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_FIXED(progressWidth),
            },
            .childGap = fontData.charHeightOverFour,
        },
    })
    {
        CLAY_TEXT(lineOneString, TEXT_CONFIG_DEFAULT);
        CLAY_TEXT(lineTwoString, TEXT_CONFIG_FAINT);

        CLAY({
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIXED(progressWidth),
                    .height = CLAY_SIZING_FIXED(8),
                },
            },
            .backgroundColor = COLOR_BG_TEXTBOX,
            .cornerRadius = CLAY_CORNER_RADIUS(4),
        })
        {
            if (progress->hasPercent)
            {
                float fillWidth = (float)(progress->percent * progressWidth);
                CLAY({
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_FIXED(fillWidth),
                            .height = CLAY_SIZING_FIXED(8),
                        },
                    },
                    .backgroundColor = COLOR_BG_THUMB,
                    .cornerRadius = CLAY_CORNER_RADIUS(4),
                })
                {
                }
            }
        }
    }
}

void RenderTab(Clay_String label, StreamID streamId)
{
    CLAY({
        .layout = {
            .padding = defaultBoxPadding,
        },
        .backgroundColor = tabData.isDisabled[streamId] ? COLOR_BG_TAB_DISABLED : streamId == tabData.selectedTab ? COLOR_BG_TAB_SELECTED
                                                                              : Clay_Hovered()                    ? COLOR_BG_TAB_HOVERED
                                                                                                                  : COLOR_BG_TAB,
        // .cornerRadius = (Clay_CornerRadius){16, 16, 0, 0},
        .border = {
            .color = COLOR_BORDER_TAB,
            .width = (Clay_BorderWidth){1, 1, 1, 0, 0},
        },
    })
    {
        Clay_OnHover(HandleTabInteraction, streamId);

        CLAY_TEXT(label, tabData.isDisabled[streamId] ? TEXT_CONFIG_FAINT : TEXT_CONFIG_BOLD);
    }
}

void RenderScrollBar(ScrollID scrollId, bool vertical, bool horizontal)
{
    if (vertical)
    {
        CLAY({
            .layout = {
                .sizing = {
                    .width = 16,
                    .height = scrollData.data[scrollId].scrollContainerDimensions.height,
                }},
            .backgroundColor = COLOR_BG_SCROLLBAR,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
            .floating = {
                .attachPoints = {
                    .element = CLAY_ATTACH_POINT_RIGHT_TOP,
                    .parent = CLAY_ATTACH_POINT_RIGHT_TOP,
                },
                .attachTo = CLAY_ATTACH_TO_PARENT,
            },
        })
        {
            float ratio = scrollData.data[scrollId].scrollContainerDimensions.height / scrollData.data[scrollId].contentDimensions.height;
            CLAY({
                .layout = {
                    .sizing = {
                        .width = 16,
                        .height = (ratio)*scrollData.data[scrollId].scrollContainerDimensions.height,
                    },
                },
                .backgroundColor = (scrollData.scrolling == scrollId && scrollData.directionLock == DIRECTION_ID_VERTICAL) || Clay_Hovered() ? COLOR_BG_THUMB_HOVERED : COLOR_BG_THUMB,
                .cornerRadius = CLAY_CORNER_RADIUS(8),
                .floating = {
                    .offset = (Clay_Vector2){0, -scrollData.data[scrollId].scrollPosition->y * ratio},
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                },
            })
            {
                Clay_OnHover(HandleThumbVerticalInteraction, scrollId);
            }

            if (scrollData.scrolling == scrollId && scrollData.directionLock != DIRECTION_ID_HORIZONTAL)
            {
                CLAY({
                    .id = CLAY_IDI("VerticalScrollBox", scrollId),
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_GROW(0),
                            .height = CLAY_SIZING_GROW(0),
                        },
                    },
                    .floating = {
                        .attachTo = CLAY_ATTACH_TO_ROOT,
                    },
                })
                {
                    Clay_OnHover(HandleThumbVerticalInteraction, scrollId);
                }
            }
        }
    }

    if (horizontal)
    {
        CLAY({
            .layout = {
                .sizing = {
                    .width = scrollData.data[scrollId].scrollContainerDimensions.width - 16,
                    .height = 16,
                }},
            .backgroundColor = COLOR_BG_SCROLLBAR,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
            .floating = {
                .attachPoints = {
                    .element = CLAY_ATTACH_POINT_LEFT_BOTTOM,
                    .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM,
                },
                .attachTo = CLAY_ATTACH_TO_PARENT,
            },
        })
        {
            float ratio = (scrollData.data[scrollId].scrollContainerDimensions.width - 16) / scrollData.data[scrollId].contentDimensions.width;
            CLAY({
                .layout = {
                    .sizing = {
                        .width = ratio * scrollData.data[scrollId].scrollContainerDimensions.width,
                        .height = 16,
                    },
                },
                .backgroundColor = (scrollData.scrolling == scrollId && scrollData.directionLock == DIRECTION_ID_HORIZONTAL) || Clay_Hovered() ? COLOR_BG_THUMB_HOVERED : COLOR_BG_THUMB,
                .cornerRadius = CLAY_CORNER_RADIUS(8),
                .floating = {
                    .offset = (Clay_Vector2){-scrollData.data[scrollId].scrollPosition->x * ratio, 0},
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                },
            })
            {
                Clay_OnHover(HandleThumbHorizontalInteraction, scrollId);
            }

            if (scrollData.scrolling == scrollId && scrollData.directionLock != DIRECTION_ID_VERTICAL)
            {
                CLAY({
                    .id = CLAY_IDI("HorizontalScrollBox", scrollId),
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_GROW(0),
                            .height = CLAY_SIZING_GROW(0),
                        },
                    },
                    .floating = {
                        .attachTo = CLAY_ATTACH_TO_ROOT,
                    },
                })
                {
                    Clay_OnHover(HandleThumbHorizontalInteraction, scrollId);
                }
            }
        }
    }
}

void RenderTabContentDimensions(bool visible)
{
    Clay_ElementDeclaration wrapperConfig = visible
                                                ? (Clay_ElementDeclaration){
                                                      .layout = {
                                                          .sizing = {
                                                              .width = CLAY_SIZING_GROW(0),
                                                              .height = CLAY_SIZING_GROW(0),
                                                          },
                                                          .childGap = fontData.charWidth * 4,
                                                      }}
                                                : (Clay_ElementDeclaration){
                                                      .floating = {
                                                          .offset = {.x = GetScreenWidth()},
                                                          .attachTo = CLAY_ATTACH_TO_ROOT,
                                                      },
                                                  };
    CLAY(wrapperConfig)
    {
        CLAY(styleFilters)
        {
            CLAY(styleFilterGroup)
            {
                CLAY_TEXT(CLAY_STRING("Crop"), TEXT_CONFIG_BOLD);

                CLAY(styleFilterItemGroup)
                {
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("width:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_CROP_W,
                                (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 1, .max = FLOAT_MAX},
                                !visible,
                                6,
                                "in_w");
                        }
                    }
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("height:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_CROP_H,
                                (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 1, .max = FLOAT_MAX},
                                !visible,
                                6,
                                "in_h");
                        }
                    }
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("x-offset:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_CROP_X,
                                (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 0, .max = FLOAT_MAX},
                                !visible,
                                6,
                                "0");
                        }
                    }
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("y-offset:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_CROP_Y,
                                (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 0, .max = FLOAT_MAX},
                                !visible,
                                6,
                                "0");
                        }
                    }
                }
            }

            CLAY(styleFilterGroup)
            {
                CLAY_TEXT(CLAY_STRING("Scale"), TEXT_CONFIG_BOLD);

                CLAY(styleFilterItemGroup)
                {
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("width:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_SCALE_W,
                                (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 0, .max = FLOAT_MAX},
                                !visible,
                                6,
                                textboxData.textboxBuffers[TEXTBOX_ID_SCALE_H].chars[0] == '\0' ? "in_w" : "-1");
                        }
                    }
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("height:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_SCALE_H,
                                (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 0, .max = FLOAT_MAX},
                                !visible,
                                6,
                                textboxData.textboxBuffers[TEXTBOX_ID_SCALE_W].chars[0] == '\0' ? "in_h" : "-1");
                        }
                    }
                }
            }
        }

        CLAY({
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childGap = fontData.charHeight,
            },
        })
        {
            bool imageLoaded = visible && previewImageData.imageTexture.id > 0;

            CLAY({.layout = {.childGap = fontData.charWidth}})
            {
                RenderButton(
                    CLAY_STRING("Load Preview"),
                    BUTTON_ID_LOAD_PREVIEW,
                    !visible,
                    HandleLoadPreviewButtonInteraction,
                    0,
                    buttonPadding);

                CLAY({.layout = {.childGap = fontData.charHeightOverFour}})
                {
                    RenderButton(
                        CLAY_STRING("-"),
                        BUTTON_ID_PREVIEW_SIZE_DOWN,
                        !imageLoaded,
                        HandlePreviewSizeDownButtonInteraction,
                        0,
                        defaultBoxPadding);

                    RenderButton(
                        CLAY_STRING("+"),
                        BUTTON_ID_PREVIEW_SIZE_UP,
                        !imageLoaded,
                        HandlePreviewSizeUpButtonInteraction,
                        0,
                        defaultBoxPadding);
                }
            }

            if (imageLoaded)
            {
                float previewWidth = previewImageData.imageTexture.width > previewImageData.imageTexture.height
                                         ? previewImageData.imageSize
                                         : previewImageData.imageTexture.width * previewImageData.imageSize / previewImageData.imageTexture.height;
                float previewHeight = previewImageData.imageTexture.height > previewImageData.imageTexture.width
                                          ? previewImageData.imageSize
                                          : previewImageData.imageTexture.height * previewImageData.imageSize / previewImageData.imageTexture.width;
                CLAY({
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_FIXED(previewWidth),
                            .height = CLAY_SIZING_FIXED(previewHeight),
                        },
                    },
                    .image = {
                        .imageData = &previewImageData.imageTexture,
                    },
                })
                {
                }
            }
        }
    }
}

void RenderTabContentVideo(bool visible)
{
    Clay_ElementDeclaration wrapperConfig = visible
                                                ? (Clay_ElementDeclaration){
                                                      .layout = {
                                                          .sizing = {
                                                              .width = CLAY_SIZING_GROW(0),
                                                              .height = CLAY_SIZING_GROW(0),
                                                          },
                                                      }}
                                                : (Clay_ElementDeclaration){
                                                      .floating = {
                                                          .offset = {.x = GetScreenWidth()},
                                                          .attachTo = CLAY_ATTACH_TO_ROOT,
                                                      },
                                                  };
    CLAY(wrapperConfig)
    {
        CLAY(styleFilters)
        {
            CLAY(styleFilterGroup)
            {
                CLAY_TEXT(CLAY_STRING("FPS"), TEXT_CONFIG_BOLD);

                CLAY(styleFilterItemGroup)
                {
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("fps:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_FPS,
                                (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 1, .max = 120},
                                !visible,
                                3,
                                "30");
                        }
                    }
                }
            }

            CLAY(styleFilterGroup)
            {
                CLAY_TEXT(CLAY_STRING("Duration"), TEXT_CONFIG_BOLD);

                CLAY(styleFilterItemGroup)
                {
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("start:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            const char *startAudio = textboxData.textboxBuffers[TEXTBOX_ID_DURATION_START_AUDIO].chars;
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_DURATION_START_VIDEO,
                                (NumberboxConfig){.isNumberbox = true, .min = 0, .max = FLOAT_MAX},
                                !visible,
                                6,
                                startAudio[0] == '\0' ? "0.0" : startAudio);
                        }
                    }
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("end:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            const char *endAudio = textboxData.textboxBuffers[TEXTBOX_ID_DURATION_END_AUDIO].chars;
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_DURATION_END_VIDEO,
                                (NumberboxConfig){.isNumberbox = true, .min = 0, .max = FLOAT_MAX},
                                !visible,
                                6,
                                endAudio[0] == '\0' ? "end" : endAudio);
                        }
                    }
                }
            }

            CLAY(styleFilterGroup)
            {
                CLAY_TEXT(CLAY_STRING("Playback Speed"), TEXT_CONFIG_BOLD);

                CLAY(styleFilterItemGroup)
                {
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("multiplier:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            const char *speedAudio = textboxData.textboxBuffers[TEXTBOX_ID_SPEED_AUDIO].chars;
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_SPEED_VIDEO,
                                (NumberboxConfig){.isNumberbox = true, .min = 0.01, .max = 100},
                                !visible,
                                4,
                                speedAudio[0] == '\0' ? "1.0" : speedAudio);
                        }
                    }
                }
            }
        }
    }
}

void RenderTabContentAudio(bool visible)
{
    Clay_ElementDeclaration wrapperConfig = visible
                                                ? (Clay_ElementDeclaration){
                                                      .layout = {
                                                          .sizing = {
                                                              .width = CLAY_SIZING_GROW(0),
                                                              .height = CLAY_SIZING_GROW(0),
                                                          },
                                                      }}
                                                : (Clay_ElementDeclaration){
                                                      .floating = {
                                                          .offset = {.x = GetScreenWidth()},
                                                          .attachTo = CLAY_ATTACH_TO_ROOT,
                                                      },
                                                  };
    CLAY(wrapperConfig)
    {
        CLAY(styleFilters)
        {
            CLAY(styleFilterGroup)
            {
                CLAY_TEXT(CLAY_STRING("Volume"), TEXT_CONFIG_BOLD);

                CLAY(styleFilterItemGroup)
                {
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("multiplier:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_VOLUME,
                                (NumberboxConfig){.isNumberbox = true, .min = 0, .max = 3},
                                !visible,
                                4,
                                "1.0");
                        }
                    }
                }
            }

            CLAY(styleFilterGroup)
            {
                CLAY_TEXT(CLAY_STRING("Duration"), TEXT_CONFIG_BOLD);

                CLAY(styleFilterItemGroup)
                {
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("start:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            const char *startVideo = textboxData.textboxBuffers[TEXTBOX_ID_DURATION_START_VIDEO].chars;
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_DURATION_START_AUDIO,
                                (NumberboxConfig){.isNumberbox = true, .min = 0, .max = FLOAT_MAX},
                                !visible,
                                6,
                                startVideo[0] == '\0' ? "0.0" : startVideo);
                        }
                    }
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("end:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            const char *endVideo = textboxData.textboxBuffers[TEXTBOX_ID_DURATION_END_VIDEO].chars;
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_DURATION_END_AUDIO,
                                (NumberboxConfig){.isNumberbox = true, .min = 0, .max = FLOAT_MAX},
                                !visible,
                                6,
                                endVideo[0] == '\0' ? "end" : endVideo);
                        }
                    }
                }
            }

            CLAY(styleFilterGroup)
            {
                CLAY_TEXT(CLAY_STRING("Playback Speed"), TEXT_CONFIG_BOLD);

                CLAY(styleFilterItemGroup)
                {
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("multiplier:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            const char *speedVideo = textboxData.textboxBuffers[TEXTBOX_ID_SPEED_VIDEO].chars;
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_SPEED_AUDIO,
                                (NumberboxConfig){.isNumberbox = true, .min = 0.01, .max = 100},
                                !visible,
                                4,
                                speedVideo[0] == '\0' ? "1.0" : speedVideo);
                        }
                    }
                }
            }

            CLAY(styleFilterGroup)
            {
                CLAY_TEXT(CLAY_STRING("Delay"), TEXT_CONFIG_BOLD);

                CLAY(styleFilterItemGroup)
                {
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("delay (ms):"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_DELAY,
                                (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 0, .max = 10000},
                                !visible,
                                5,
                                "0");
                        }
                    }
                }
            }

            CLAY(styleFilterGroup)
            {
                CLAY_TEXT(CLAY_STRING("Loudness Normalization"), TEXT_CONFIG_BOLD);

                CLAY(styleFilterItemGroup)
                {
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("enable:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            RenderDropdown(
                                CLAY_STRING(""),
                                DROPDOWN_ID_LOUDNORM_ENABLE,
                                (DropdownOption[]){
                                    {"Disable", ""},
                                    {"Enable", "enable"},
                                    DROPDOWN_OPTION_NULL,
                                });
                        }
                    }
                }
            }

            CLAY(styleFilterGroup)
            {
                CLAY_TEXT(CLAY_STRING("Format"), TEXT_CONFIG_BOLD);

                CLAY(styleFilterItemGroup)
                {
                    CLAY(styleFilterItem)
                    {
                        CLAY(styleFilterItemLabel)
                        {
                            CLAY_TEXT(CLAY_STRING("channel layout:"), TEXT_CONFIG_BOLD);
                        }
                        CLAY(styleFilterItemField)
                        {
                            RenderDropdown(CLAY_STRING(""),
                                           DROPDOWN_ID_CHANNEL_LAYOUT,
                                           (DropdownOption[]){
                                               DROPDOWN_OPTION_UNSELECTED,
                                               {"Stereo", "stereo"},
                                               {"Mono", "mono"},
                                               DROPDOWN_OPTION_NULL,
                                           });
                        }
                    }
                }
            }
        }
    }
}

void RenderTabContentSubtitles(bool visible)
{
    Clay_ElementDeclaration wrapperConfig = visible
                                                ? (Clay_ElementDeclaration){
                                                      .layout = {
                                                          .sizing = {
                                                              .width = CLAY_SIZING_GROW(0),
                                                              .height = CLAY_SIZING_GROW(0),
                                                          },
                                                      }}
                                                : (Clay_ElementDeclaration){
                                                      .floating = {
                                                          .offset = {.x = GetScreenWidth()},
                                                          .attachTo = CLAY_ATTACH_TO_ROOT,
                                                      },
                                                  };
    CLAY(wrapperConfig)
    {
        CLAY(styleFilters)
        {
            CLAY(styleFilterGroup)
            {
                CLAY_TEXT(CLAY_STRING("Subtitles"), TEXT_CONFIG_BOLD);

                CLAY(styleFilterItem)
                {
                    CLAY(styleFilterItemLabel)
                    {
                        CLAY_TEXT(CLAY_STRING("subtitles:"), TEXT_CONFIG_BOLD);
                    }
                    CLAY(styleFilterItemField)
                    {
                        RenderDropdown(
                            CLAY_STRING(""),
                            DROPDOWN_ID_SUBTITLES,
                            (DropdownOption[]){
                                DROPDOWN_OPTION_UNSELECTED,
                                {"Burn-in", "b"},
                                DROPDOWN_OPTION_NULL,
                            });
                    }
                }
                CLAY(styleFilterItem)
                {
                    bool subtitles = getDropdownValue(DROPDOWN_ID_SUBTITLES)[0] != '\0';

                    CLAY(styleFilterItemLabel)
                    {
                        CLAY_TEXT(CLAY_STRING("source file:"), TEXT_CONFIG_BOLD);
                    }
                    CLAY(styleFilterItemField)
                    {
                        CLAY({.layout = {.childGap = fontData.charWidth}})
                        {
                            RenderTextbox(
                                CLAY_STRING(""),
                                TEXTBOX_ID_SUBTITLES_SOURCE,
                                (NumberboxConfig){0},
                                !subtitles,
                                8,
                                "");

                            RenderButton(
                                CLAY_STRING("..."),
                                BUTTON_ID_BROWSE_SUBTITLES_SOURCE,
                                !subtitles,
                                HandleBrowseButtonInteraction,
                                TEXTBOX_ID_SUBTITLES_SOURCE,
                                defaultBoxPadding);
                        }
                    }
                }
            }
        }
    }
}

void RenderErrorPopups(void)
{
    if (errorPopupData.count == 0)
    {
        return;
    }

    double now = GetTime();

    CLAY({
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childGap = fontData.charHeightOverFour,
        },
        .backgroundColor = COLOR_TRANSPARENT,
        .floating = {
            .attachPoints = {
                .element = CLAY_ATTACH_POINT_RIGHT_TOP,
                .parent = CLAY_ATTACH_POINT_RIGHT_TOP,
            },
            .offset = (Clay_Vector2){-16, 16},
            .attachTo = CLAY_ATTACH_TO_ROOT,
        },
    })
    {
        for (size_t i = 0; i < errorPopupData.count; i++)
        {
            double age = now - errorPopupData.popups[i].createdAt;
            float alpha = 1.0f - (float)(age / 10.0f);
            alpha = CLAMP(alpha, 0.35f, 1.0f);

            Clay_Color popupColor = (Clay_Color){220, 60, 60, (uint8_t)(alpha * 230.0f)};
            CLAY({
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(320),
                    },
                    .padding = defaultBoxPadding,
                    .childGap = fontData.charWidth,
                    .childAlignment = {
                        .x = CLAY_ALIGN_X_RIGHT,
                        .y = CLAY_ALIGN_Y_CENTER,
                    },
                },
                .backgroundColor = popupColor,
                .cornerRadius = CLAY_CORNER_RADIUS(10),
                .border = {
                    .color = (Clay_Color){255, 255, 255, (uint8_t)(alpha * 80.0f)},
                    .width = CLAY_BORDER_OUTSIDE(1),
                },
            })
            {
                if (Clay_Hovered())
                {
                    errorPopupData.popups[i].createdAt = now;
                }

                // CLAY({
                //     .layout = {
                //         .childGap = fontData.charWidth,
                //         .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
                //     },
                // })
                // {
                Clay_String str = {
                    .isStaticallyAllocated = true,
                    .length = strlen(errorPopupData.popups[i].message),
                    .chars = errorPopupData.popups[i].message,
                };
                CLAY_TEXT(str, CLAY_TEXT_CONFIG({
                                   .fontId = FONT_ID_BODY,
                                   .fontSize = fontData.fontSize,
                                   .textColor = (Clay_Color){255, 255, 255, (uint8_t)(alpha * 255.0f)},
                               }));

                CLAY({
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_FIXED(24),
                            .height = CLAY_SIZING_FIXED(24),
                        },
                        .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                    },
                    .backgroundColor = Clay_Hovered() ? (Clay_Color){255, 255, 255, 80} : COLOR_TRANSPARENT,
                    .cornerRadius = CLAY_CORNER_RADIUS(8),
                })
                {
                    Clay_OnHover(HandleErrorPopupCloseButtonInteraction, i);
                    CLAY_TEXT(CLAY_STRING("x"), TEXT_CONFIG_BOLD);
                }
                // }
            }
        }
    }
}

void setStyles()
{
    defaultBoxPadding = (Clay_Padding){
        fontData.charWidth,
        fontData.charWidth,
        fontData.charHeightOverFour,
        fontData.charHeightOverFour,
    };

    buttonPadding = (Clay_Padding){
        fontData.charWidth * 3,
        fontData.charWidth * 3,
        fontData.charHeightOverFour,
        fontData.charHeightOverFour,
    };

    dropdownPadding = (Clay_Padding){
        0,
        0,
        fontData.charHeightOverFour,
        fontData.charHeightOverFour,
    };

    styleFilters = (Clay_ElementDeclaration){
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childGap = fontData.charHeightOverFour * 6,
        },
    };

    styleFilterGroup = (Clay_ElementDeclaration){
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_FIXED(300),
            },
            .childGap = fontData.charHeightOverFour * 3,
        },
        .border = {
            .color = COLOR_GRAY,
            .width = (Clay_BorderWidth){0, 0, 0, 0, 1},
        },
    };

    styleFilterItemGroup = (Clay_ElementDeclaration){
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
            },
            .childGap = fontData.charWidth,
        },
    };

    styleFilterItem = (Clay_ElementDeclaration){
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
            },
            .childGap = fontData.charWidth,
        },
    };

    styleFilterItemLabel = (Clay_ElementDeclaration){
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_GROW(0),
            },
            .childAlignment = {
                .x = CLAY_ALIGN_X_RIGHT,
                .y = CLAY_ALIGN_X_CENTER,
            },
        },
    };

    styleFilterItemField = (Clay_ElementDeclaration){
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
            },
        },
    };
}

void LayoutCreator_Initialize()
{
    fontData = (FontData){
        .fontSize = 16,
        .fontSizeMin = 8,
        .fontSizeMax = 32,
        .charWidth = 8,
        .charHeight = 16,
        .charHeightOverFour = 4,
    };

    textboxData = (TextboxData){
        .textboxBuffers = {0},
        .isInit = {0},
        .isEnabled = {0},
        .hoveredTextbox = -1,
        .focusData = {
            .focusRegistered = false,
            .focusIndex = -1,
        },
    };

    dropdownData = (DropdownData){
        .selectedOptions = {0},
        .selectedValues = {0},
        .isInit = {0},
        .hoveredOption = 0,
        .hoveredValue = NULL,
    };

    tabData = (TabData){
        .selectedTab = TAB_ID_DIMENSIONS,
        .isDisabled = {0},
    };

    scrollData = (ScrollData){
        .data = {0},
        .scrolling = -1,
        .directionLock = DIRECTION_ID_NONE,
        .middleClickPosition = {0},
    };

    streamData = (StreamData){
        .inputPath = {0},
        .streamCounts = {0},
        .inputDurationSeconds = -1.0,
        .convertProcess = {
#ifdef _WIN32
            .process = NULL,
            .stdoutRead = NULL,
#else
            .pid = 0,
            .stdoutfd = 0,
#endif
        },
        .convertOutput = {0},
        .convertOutputLength = 0,
        .progress = {0},
        .progressPartialLine = {0},
        .progressPartialLineLength = 0,
    };

    errorPopupData = (ErrorPopupData){0};

    previewImageData = (ImageData){
        .imageTexture = {0},
        .imageSize = 500,
        .imageSizeMin = 100,
        .imageSizeMax = 1500,
    };

    setStyles();

    if (NFD_Init() != NFD_OKAY)
    {
        ERROR("%s", NFD_GetError());
    }
}

void LayoutCreator_Destroy()
{
    childKill(&streamData.convertProcess);

    if (previewImageData.imageTexture.id != 0)
    {
        UnloadTexture(previewImageData.imageTexture);
        previewImageData.imageTexture.id = 0;
    }

    NFD_Quit();
}

Clay_RenderCommandArray LayoutCreator_CreateLayout()
{
    Clay_BeginLayout();

    updateErrorPopups();

    textboxData.hoveredTextbox = -1;
    textboxData.focusData.focusRegistered = false;

    scrollData.data[SCROLL_ID_WINDOW] = Clay_GetScrollContainerData(CLAY_ID("WindowContainer"));
    bool scrollingWindowVertical = false;
    bool scrollingWindowHorizontal = false;
    if (scrollData.data[SCROLL_ID_WINDOW].found)
    {
        scrollingWindowVertical = scrollData.data[SCROLL_ID_WINDOW].scrollContainerDimensions.height < scrollData.data[SCROLL_ID_WINDOW].contentDimensions.height;
        scrollingWindowHorizontal = scrollData.data[SCROLL_ID_WINDOW].scrollContainerDimensions.width < scrollData.data[SCROLL_ID_WINDOW].contentDimensions.width;
    }

    if (streamData.convertProcess.valid)
    {
        while (childPoll(&streamData.convertProcess))
        {
            char buffer[4096];
            int n = childRead(&streamData.convertProcess, buffer, sizeof(buffer));
            if (n > 0)
            {
                LayoutModel model;
                copyGuiStateToModel(&model);
                LayoutModel_AppendConvertOutput(&model, buffer, (size_t)n);
                copyModelStreamToGui(&model);
                LOG("%s", buffer);
            }
            else
            {
                break;
            }
        }

        if (childExited(&streamData.convertProcess))
        {
            if (streamData.progressPartialLineLength > 0)
            {
                LayoutModel model;
                copyGuiStateToModel(&model);
                LayoutModel_AppendConvertOutput(&model, "\n", 1);
                copyModelStreamToGui(&model);
            }
            if (streamData.convertOutputLength > 0)
            {
                const char *errorMessage = sanitizeErrorMessage(streamData.convertOutput);
                if (strstr(errorMessage, "The selected output height") != NULL || strstr(errorMessage, "Invalid argument") != NULL || strstr(errorMessage, "Error") != NULL || strstr(errorMessage, "error") != NULL)
                {
                    reportErrorf("%s", errorMessage);
                }
            }
            if (streamData.convertOutputLength == 0 && !streamData.progress.finished)
            {
                LayoutModel model;
                copyGuiStateToModel(&model);
                LayoutModel_ResetConvertProgress(&model);
                copyModelStreamToGui(&model);
            }
            childReset(&streamData.convertProcess);
        }
    }

    CLAY({
        .id = CLAY_ID("WindowContainer"),
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_GROW(0),
            },
            .padding = (Clay_Padding){16, scrollingWindowVertical ? 32 : 16, 16, scrollingWindowHorizontal ? 32 : 16},
        },
        .backgroundColor = COLOR_BG_MAIN,
        .clip = {
            .vertical = true,
            .horizontal = true,
            .childOffset = Clay_GetScrollOffset(),
        },
    })
    {
        RenderScrollBar(SCROLL_ID_WINDOW, scrollingWindowVertical, scrollingWindowHorizontal);
        RenderErrorPopups();

        if (scrollData.scrolling == SCROLL_ID_DUMMY_LAST)
        {
            CLAY({
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(30),
                        .height = CLAY_SIZING_FIXED(30),
                    }},
                .backgroundColor = (Clay_Color){200, 200, 200, 150},
                .cornerRadius = CLAY_CORNER_RADIUS(15),
                .floating = {
                    .offset = {scrollData.middleClickPosition.x - 15, scrollData.middleClickPosition.y - 15},
                    .attachTo = CLAY_ATTACH_TO_ROOT,
                },
            })
            {
            }
        }

        CLAY({
            .id = CLAY_ID("MainSectionContainer"),
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_GROW(0),
                },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = fontData.charHeight,
            },
            .backgroundColor = COLOR_BG_SECTION,
            .cornerRadius = CLAY_CORNER_RADIUS(16),
        })
        {
            CLAY({
                .layout = {
                    .childGap = fontData.charWidth * 2,
                    .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}},
            })
            {
                CLAY({
                    .layout = {
                        .childGap = fontData.charHeightOverFour,
                        .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}},
                })
                {
                    RenderTextbox(
                        CLAY_STRING(" Input File:"),
                        TEXTBOX_ID_INPUT_PATH,
                        (NumberboxConfig){0},
                        false,
                        30,
                        "");

                    RenderButton(
                        CLAY_STRING("..."),
                        BUTTON_ID_BROWSE_INPUT,
                        false,
                        HandleBrowseButtonInteraction,
                        TEXTBOX_ID_INPUT_PATH,
                        defaultBoxPadding);
                }

                RenderButton(
                    CLAY_STRING("Load File"),
                    BUTTON_ID_LOAD_INPUT,
                    trim(getTextboxValue(TEXTBOX_ID_INPUT_PATH))[0] == '\0',
                    HandleLoadInputButtonInteraction,
                    0,
                    buttonPadding);
            }

            CLAY({.layout = {.childGap = fontData.charWidth}})
            {
                RenderDropdown(
                    CLAY_STRING("Output Type:"),
                    DROPDOWN_ID_OUTPUT_TYPE,
                    streamData.streamCounts[STREAM_ID_VIDEO] > 0   ? streamData.streamCounts[STREAM_ID_AUDIO] > 0 ? (DropdownOption[]){
                                                                                                                        DROPDOWN_OPTION_OUTPUT_TYPE_VIDEO,
                                                                                                                        DROPDOWN_OPTION_OUTPUT_TYPE_AUDIO,
                                                                                                                        DROPDOWN_OPTION_OUTPUT_TYPE_IMAGE,
                                                                                                                        DROPDOWN_OPTION_NULL,
                                                                                                                    }
                                                                                                                  : (DropdownOption[]){
                                                                                                                        DROPDOWN_OPTION_OUTPUT_TYPE_VIDEO,
                                                                                                                        DROPDOWN_OPTION_OUTPUT_TYPE_IMAGE,
                                                                                                                        DROPDOWN_OPTION_NULL,
                                                                                                                    }
                    : streamData.streamCounts[STREAM_ID_AUDIO] > 0 ? (DropdownOption[]){
                                                                         DROPDOWN_OPTION_OUTPUT_TYPE_AUDIO,
                                                                         DROPDOWN_OPTION_NULL,
                                                                     }
                                                                   : (DropdownOption[]){
                                                                         DROPDOWN_OPTION_UNSELECTED,
                                                                         DROPDOWN_OPTION_NULL,
                                                                     });
            }

            CLAY({
                .id = CLAY_ID("TabbedBox"),
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .sizing = {
                        .width = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_GROW(0),
                    },
                },
            })
            {
                CLAY({
                    .id = CLAY_ID("TabBar"),
                    .layout = {.sizing = {.width = CLAY_SIZING_GROW(0)}},
                })
                {
                    char outputType = getDropdownValue(DROPDOWN_ID_OUTPUT_TYPE)[0];

                    tabData.isDisabled[TAB_ID_DIMENSIONS] = streamData.streamCounts[STREAM_ID_VIDEO] == 0 || (outputType != 'v' && outputType != 'i');
                    tabData.isDisabled[TAB_ID_VIDEO] = streamData.streamCounts[STREAM_ID_VIDEO] == 0 || outputType != 'v';
                    tabData.isDisabled[TAB_ID_AUDIO] = streamData.streamCounts[STREAM_ID_AUDIO] == 0 || (outputType != 'v' && outputType != 'a');
                    tabData.isDisabled[TAB_ID_SUBTITLES] = outputType != 'v';

                    if (tabData.isDisabled[tabData.selectedTab])
                    {
                        for (tabData.selectedTab = 0; tabData.selectedTab < TAB_ID_DUMMY_LAST; tabData.selectedTab++)
                        {
                            if (!tabData.isDisabled[tabData.selectedTab])
                            {
                                break;
                            }
                        }
                    }

                    RenderTab(
                        CLAY_STRING("Dimensions"),
                        TAB_ID_DIMENSIONS);
                    RenderTab(
                        CLAY_STRING("Video"),
                        TAB_ID_VIDEO);
                    RenderTab(
                        CLAY_STRING("Audio"),
                        TAB_ID_AUDIO);
                    RenderTab(
                        CLAY_STRING("Subtitles"),
                        TAB_ID_SUBTITLES);

                    CLAY({
                        .layout = {
                            .sizing = {
                                .width = CLAY_SIZING_GROW(0),
                                .height = CLAY_SIZING_GROW(0),
                            },
                        },
                        .backgroundColor = COLOR_TRANSPARENT,
                        .border = {
                            .color = COLOR_BORDER_TAB,
                            .width = (Clay_BorderWidth){0, 0, 0, 1, 0},
                        },
                    })
                    {
                    }
                }

                CLAY({
                    .id = CLAY_ID("TabbedBoxContent"),
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_GROW(0),
                            .height = CLAY_SIZING_GROW(0),
                        },
                        .padding = CLAY_PADDING_ALL(16),
                    },
                    .backgroundColor = COLOR_BG_TAB_SELECTED,
                    .cornerRadius = (Clay_CornerRadius){0, 0, 16, 16},
                    .border = {
                        .color = COLOR_BORDER_TAB,
                        .width = (Clay_BorderWidth){1, 1, 0, 1, 0},
                    },
                })
                {
                    RenderTabContentDimensions(tabData.selectedTab == TAB_ID_DIMENSIONS);
                    RenderTabContentVideo(tabData.selectedTab == TAB_ID_VIDEO);
                    RenderTabContentAudio(tabData.selectedTab == TAB_ID_AUDIO);
                    RenderTabContentSubtitles(tabData.selectedTab == TAB_ID_SUBTITLES);
                }
            }

            CLAY({
                .layout = {
                    .childGap = fontData.charWidth * 2,
                    .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}},
            })
            {
                bool hasInput = streamData.inputPath[0] != '\0';

                CLAY({
                    .layout = {
                        .childGap = fontData.charHeightOverFour,
                        .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}},
                })
                {
                    RenderTextbox(
                        CLAY_STRING("Output File:"),
                        TEXTBOX_ID_OUTPUT_PATH,
                        (NumberboxConfig){0},
                        !hasInput,
                        30,
                        "");

                    RenderButton(
                        CLAY_STRING("..."),
                        BUTTON_ID_BROWSE_OUTPUT,
                        !hasInput,
                        HandleBrowseButtonInteraction,
                        TEXTBOX_ID_OUTPUT_PATH,
                        defaultBoxPadding);
                }

                if (streamData.convertProcess.valid)
                {
                    RenderButton(
                        CLAY_STRING("Cancel"),
                        BUTTON_ID_CONVERT_CANCEL,
                        false,
                        HandleConvertCancelButtonInteraction,
                        0,
                        buttonPadding);
                    RenderConvertProgress();
                }
                else
                {
                    RenderButton(
                        CLAY_STRING("Convert"),
                        BUTTON_ID_CONVERT,
                        !hasInput || trim(getTextboxValue(TEXTBOX_ID_OUTPUT_PATH))[0] == '\0',
                        HandleConvertButtonInteraction,
                        0,
                        buttonPadding);
                }
            }
        }
    }

    // Handle drag-and-dropped files
    if (IsFileDropped())
    {
        FilePathList pathList = LoadDroppedFiles();

        char temp[TEXTBOX_BUFFER_SIZE];
        int ret = snprintf(
            temp,
            sizeof(temp),
            "%s",
            pathList.paths[0]);

        if (ret >= 0 && ret < sizeof(temp))
        {
            TextboxBuffer *buffer = &textboxData.textboxBuffers[TEXTBOX_ID_INPUT_PATH];
            snprintf(buffer->chars, sizeof(buffer->chars), "%s", temp);
            buffer->length = strlen(buffer->chars);
            buffer->cursorPosition = buffer->length;
        }
        else
        {
            ERROR("Input file path is too long.");
            reportErrorf("The dropped file path is too long to use.");
        }

        UnloadDroppedFiles(pathList);
    }

    // Handle middle-click
    if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
    {
        scrollData.scrolling = SCROLL_ID_DUMMY_LAST;
        scrollData.middleClickPosition = GetMousePosition();
    }
    else if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
    {
        HandleThumbVerticalInteraction(CLAY_ID(""), (Clay_PointerData){0}, SCROLL_ID_DUMMY_LAST);
        HandleThumbHorizontalInteraction(CLAY_ID(""), (Clay_PointerData){0}, SCROLL_ID_DUMMY_LAST);
    }
    else if (scrollData.scrolling == SCROLL_ID_DUMMY_LAST && IsMouseButtonUp(MOUSE_BUTTON_MIDDLE))
    {
        scrollData.scrolling = -1;
    }

    LayoutModel model;
    GuiInputFrame input = {0};
    int key;
    while (input.typedCharCount < LAYOUT_MAX_TYPED_CHARS && (key = GetCharPressed()))
    {
        input.typedChars[input.typedCharCount++] = key;
    }
    input.keyPressed[LAYOUT_KEY_BACKSPACE] = IsKeyPressed(KEY_BACKSPACE);
    input.keyRepeated[LAYOUT_KEY_BACKSPACE] = IsKeyPressedRepeat(KEY_BACKSPACE);
    input.keyPressed[LAYOUT_KEY_DELETE] = IsKeyPressed(KEY_DELETE);
    input.keyRepeated[LAYOUT_KEY_DELETE] = IsKeyPressedRepeat(KEY_DELETE);
    input.keyPressed[LAYOUT_KEY_LEFT] = IsKeyPressed(KEY_LEFT);
    input.keyRepeated[LAYOUT_KEY_LEFT] = IsKeyPressedRepeat(KEY_LEFT);
    input.keyPressed[LAYOUT_KEY_RIGHT] = IsKeyPressed(KEY_RIGHT);
    input.keyRepeated[LAYOUT_KEY_RIGHT] = IsKeyPressedRepeat(KEY_RIGHT);
    input.keyPressed[LAYOUT_KEY_TAB] = IsKeyPressed(KEY_TAB);
    input.keyRepeated[LAYOUT_KEY_TAB] = IsKeyPressedRepeat(KEY_TAB);
    input.ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    input.shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    input.leftMouseReleased = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    input.now = GetTime();

    copyGuiStateToModel(&model);
    LayoutModel_ApplyTextboxInput(&model, &input);
    copyModelTextboxesToGui(&model);

    SetMouseCursor(IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) ? MOUSE_CURSOR_RESIZE_ALL : textboxData.hoveredTextbox >= 0 ? MOUSE_CURSOR_IBEAM
                                                                                                                      : MOUSE_CURSOR_DEFAULT);

    return Clay_EndLayout();
}
