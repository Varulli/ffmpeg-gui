#include "clay.h"
#include "raylib.h"
#include "nfd.h"
#include "cJSON.h"
#include <stdio.h>
#include <stddef.h>

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
#define ERROR(format, ...) fprintf(stderr, "\x1b[31mERROR: " format "\x1b[0m\n", ##__VA_ARGS__)

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
    ChildProcessData convertProcess;
} StreamData;

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

const char *getTextboxValue(TextboxID textboxId)
{
    if (textboxData.textboxBuffers[textboxId].length > 0)
    {
        return textboxData.textboxBuffers[textboxId].chars;
    }
    return textboxData.textboxBuffers[textboxId].charsDefault;
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

static char *buildCmdline(char *argv[])
{
    size_t cap = 256;
    size_t len = 0;
    char *cmd = malloc(cap);

    for (size_t i = 1; argv[i] != NULL; i++)
    {
        const char *arg = argv[i];

        if (len + strlen(arg) + 2 >= cap)
        {
            cap *= 2;
            cmd = realloc(cmd, cap);
        }

        if (i > 1)
        {
            cmd[len++] = ' ';
        }

        len += sprintf(cmd + len, "%s", arg);
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
    ssize_t r = read(c->stdout_fd, buffer, len);
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

int convert()
{
    char buffer[4096];
    int ret;

    // Validate input path
    if (!FileExists(streamData.inputPath))
    {
        ERROR("Input file does not exist (\"%s\").", streamData.inputPath);
        return 1;
    }

    // Validate output path
    const char *outputPath = trim(getTextboxValue(TEXTBOX_ID_OUTPUT_PATH));
    if (outputPath[0] == '\0')
    {
        ERROR("Output file is missing.");
        return 2;
    }
    const char *outputDir = GetDirectoryPath(outputPath);
    if (!DirectoryExists(outputDir))
    {
        ERROR("Output directory does not exist (%s).", outputDir);
        return 3;
    }
    const char *outputName = GetFileNameWithoutExt(outputPath);
    if (!IsFileNameValid(outputName))
    {
        ERROR("Output filename is invalid (%s).", outputName);
        return 4;
    }
    char outputType = getDropdownValue(DROPDOWN_ID_OUTPUT_TYPE)[0];
    const char *outputExt = GetFileExtension(outputPath);
    switch (outputType)
    {
    case 'v':
        if (outputExt == NULL || (strcmp(outputExt, ".gif") && strcmp(outputExt, ".mkv") && strcmp(outputExt, ".mov") && strcmp(outputExt, ".mp4") && strcmp(outputExt, ".webm")))
        {
            LOG("VIDEO - changing ext (%s -> %s)", outputExt, defaultExtVideo);
            outputExt = defaultExtVideo;
        }
        break;

    case 'a':
        if (outputExt == NULL || (strcmp(outputExt, ".wav") && strcmp(outputExt, ".wave") && strcmp(outputExt, ".mp3") && strcmp(outputExt, ".m4a") && strcmp(outputExt, ".flac") && strcmp(outputExt, ".ogg") && strcmp(outputExt, ".oga") && strcmp(outputExt, ".opus")))
        {
            LOG("AUDIO - changing ext (%s -> %s)", outputExt, defaultExtAudio);
            outputExt = defaultExtAudio;
        }
        break;

    case 'i':
        if (outputExt == NULL || (strcmp(outputExt, ".jpg") && strcmp(outputExt, ".jpeg") && strcmp(outputExt, ".jpe") && strcmp(outputExt, ".jfif") && strcmp(outputExt, ".png") && strcmp(outputExt, ".tiff") && strcmp(outputExt, ".tif") && strcmp(outputExt, ".webp")))
        {
            LOG("IMAGE - changing ext (%s -> %s)", outputExt, defaultExtImage);
            outputExt = defaultExtImage;
        }
        break;

    default:
        ERROR("Invalid output type selected (%c).", outputType);
        return 5;
        break;
    }

    char slash = '/';
#ifdef _WIN32
    const char *slashPtr = strchr(outputDir, '\\');
    if (slashPtr != NULL)
    {
        slash = '\\';
    }
#endif

    size_t fullOutputPathSize = strlen(outputDir) + strlen(outputName) + strlen(outputExt) + 2;
    char *fullOutputPath = malloc(fullOutputPathSize);
    if (fullOutputPath == NULL)
    {
        ERROR("Failed to allocate memory.");
        return 6;
    }
    ret = snprintf(fullOutputPath, fullOutputPathSize, "%s%c%s%s", outputDir, slash, outputName, outputExt);
    if (ret < 0 || ret >= fullOutputPathSize)
    {
        ERROR("Failed to write output into buffer.");
        free(fullOutputPath);
        return 7;
    }
    if (strcmp(streamData.inputPath, fullOutputPath) == 0)
    {
        ERROR("Input and output are the same.");
        free(fullOutputPath);
        return 8;
    }
    free(fullOutputPath);

    bool outputVideo = outputType == 'v';
    bool gifInput = strcmp(GetFileExtension(streamData.inputPath), ".gif") == 0;
    bool outputAudio = (outputType == 'a' || (outputVideo && strcmp(outputExt, ".gif"))) && streamData.streamCounts[STREAM_ID_AUDIO] > 0;
    bool outputImage = outputType == 'i';

    StringBuilder sb;
    sbInit(&sb);
    sbAppend(&sb, "\"");
    if (outputVideo)
    {
        bool burnSubtitles = getDropdownValue(DROPDOWN_ID_SUBTITLES)[0] != '\0';
        sbAppendf(
            &sb,
            "[0:v]fps=%s,trim=%s:%s,setpts=(PTS-STARTPTS)/%s,crop=%s:%s:%s:%s,scale=%s:%s%s%s%s%s[out_v];",
            getTextboxValue(TEXTBOX_ID_FPS),
            getTextboxValue(TEXTBOX_ID_DURATION_START_VIDEO),
            getTextboxValue(TEXTBOX_ID_DURATION_END_VIDEO)[0] == 'e' ? "" : getTextboxValue(TEXTBOX_ID_DURATION_END_VIDEO),
            getTextboxValue(TEXTBOX_ID_SPEED_VIDEO),
            getTextboxValue(TEXTBOX_ID_CROP_W),
            getTextboxValue(TEXTBOX_ID_CROP_H),
            getTextboxValue(TEXTBOX_ID_CROP_X),
            getTextboxValue(TEXTBOX_ID_CROP_Y),
            getTextboxValue(TEXTBOX_ID_SCALE_W),
            getTextboxValue(TEXTBOX_ID_SCALE_H),
            !burnSubtitles ? "" : ",subtitles='",
            !burnSubtitles ? "" : trim(getTextboxValue(TEXTBOX_ID_SUBTITLES_SOURCE)),
            !burnSubtitles ? "" : "'",
            !gifInput ? "" : ",format=yuv420p");
    }
    if (outputAudio)
    {
        bool channelLayout = getDropdownValue(DROPDOWN_ID_CHANNEL_LAYOUT)[0] != '\0';
        sbAppendf(
            &sb,
            "[0:a]atrim=%s:%s,",
            getTextboxValue(TEXTBOX_ID_DURATION_START_AUDIO),
            getTextboxValue(TEXTBOX_ID_DURATION_END_AUDIO)[0] == 'e' ? "" : getTextboxValue(TEXTBOX_ID_DURATION_END_AUDIO));

        float multiplier = atof(getTextboxValue(TEXTBOX_ID_SPEED_AUDIO));
        while (multiplier < 0.5)
        {
            sbAppend(&sb, "atempo=0.5,");
            multiplier *= 2;
        }
        sbAppendf(&sb, "atempo=%f,", multiplier);

        sbAppendf(
            &sb,
            "adelay=%s:1%s,aformat=%s%s[out_a]",
            getTextboxValue(TEXTBOX_ID_DELAY),
            getDropdownValue(DROPDOWN_ID_LOUDNORM_ENABLE)[0] == '\0' ? "" : ",loudnorm",
            !channelLayout ? "" : "channel_layouts=",
            !channelLayout ? "" : getDropdownValue(DROPDOWN_ID_CHANNEL_LAYOUT));
    }
    if (outputImage)
    {
        sbAppendf(
            &sb,
            "[0:v]crop=%s:%s:%s:%s,scale=%s:%s[out_v]",
            getTextboxValue(TEXTBOX_ID_CROP_W),
            getTextboxValue(TEXTBOX_ID_CROP_H),
            getTextboxValue(TEXTBOX_ID_CROP_X),
            getTextboxValue(TEXTBOX_ID_CROP_Y),
            getTextboxValue(TEXTBOX_ID_SCALE_W),
            getTextboxValue(TEXTBOX_ID_SCALE_H));
    }
    sbAppend(&sb, "\"");
    // sbAppendf(
    //     &sb,
    //     "\" %s %s %s %s \"%s%c%s%s\"",
    //     outputVideo || outputImage ? "-map \"[out_v]\"" : "",
    //     outputVideo && gifInput ? "-c:v libx264 -movflags +faststart" : "",
    //     outputAudio ? "-map \"[out_a]\"" : "",
    //     outputImage ? "-vframes 1" : "",
    //     outputDir, slash, outputName, outputExt);

    ArgvBuilder a;
    argvInit(&a);
    argvPush(&a, strdup("ffmpeg"));
    argvPush(&a, strdup("ffmpeg"));
    argvPush(&a, strdup("-v"));
    argvPush(&a, strdup("error"));
    argvPush(&a, strdup("-progress"));
    argvPush(&a, strdup("pipe:1"));
    argvPush(&a, strdup("-y"));
    argvPush(&a, strdup("-i"));
    argvPush(&a, argprintf("\"%s\"", streamData.inputPath));
    argvPush(&a, strdup("-filter_complex"));
    argvPush(&a, sb.buffer);
    if (outputVideo || outputImage)
    {
        argvPush(&a, strdup("-map"));
        argvPush(&a, strdup("\"[out_v]\""));
    }
    if (outputVideo && gifInput)
    {
        argvPush(&a, strdup("-c:v"));
        argvPush(&a, strdup("libx264"));
        argvPush(&a, strdup("-movflags"));
        argvPush(&a, strdup("+faststart"));
    }
    if (outputAudio)
    {
        argvPush(&a, strdup("-map"));
        argvPush(&a, strdup("\"[out_a]\""));
    }
    if (outputImage)
    {
        argvPush(&a, strdup("-vframes"));
        argvPush(&a, strdup("1"));
    }
    argvPush(&a, argprintf("\"%s%c%s%s\"", outputDir, slash, outputName, outputExt));

    ret = childCreate(&streamData.convertProcess, a.v);
    argvFree(&a);
    if (ret)
    {
        ERROR("Failed to create child process (%d)", ret);
        return ret;
    }

    /*
    #ifdef _WIN32
        ret = snprintf(
            buffer,
            sizeof(buffer),
            "ffmpeg -v error -progress pipe:1 -y -i \"%s\" -filter_complex \"",
            streamData.inputPath);
        if (outputVideo)
        {
            bool burnSubtitles = getDropdownValue(DROPDOWN_ID_SUBTITLES)[0] != '\0';
            ret += snprintf(
                buffer + ret,
                sizeof(buffer) - ret,
                "[0:v]fps=%s,trim=%s:%s,setpts=(PTS-STARTPTS)/%s,crop=%s:%s:%s:%s,scale=%s:%s%s%s%s%s[out_v];",
                getTextboxValue(TEXTBOX_ID_FPS),
                getTextboxValue(TEXTBOX_ID_DURATION_START_VIDEO),
                getTextboxValue(TEXTBOX_ID_DURATION_END_VIDEO)[0] == 'e' ? "" : getTextboxValue(TEXTBOX_ID_DURATION_END_VIDEO),
                getTextboxValue(TEXTBOX_ID_SPEED_VIDEO),
                getTextboxValue(TEXTBOX_ID_CROP_W),
                getTextboxValue(TEXTBOX_ID_CROP_H),
                getTextboxValue(TEXTBOX_ID_CROP_X),
                getTextboxValue(TEXTBOX_ID_CROP_Y),
                getTextboxValue(TEXTBOX_ID_SCALE_W),
                getTextboxValue(TEXTBOX_ID_SCALE_H),
                !burnSubtitles ? "" : ",subtitles='",
                !burnSubtitles ? "" : trim(getTextboxValue(TEXTBOX_ID_SUBTITLES_SOURCE)),
                !burnSubtitles ? "" : "'",
                !gifInput ? "" : ",format=yuv420p");
        }
        if (outputAudio)
        {
            bool channelLayout = getDropdownValue(DROPDOWN_ID_CHANNEL_LAYOUT)[0] != '\0';
            ret += snprintf(
                buffer + ret,
                sizeof(buffer) - ret,
                "[0:a]atrim=%s:%s,",
                getTextboxValue(TEXTBOX_ID_DURATION_START_AUDIO),
                getTextboxValue(TEXTBOX_ID_DURATION_END_AUDIO)[0] == 'e' ? "" : getTextboxValue(TEXTBOX_ID_DURATION_END_AUDIO));

            float multiplier = atof(getTextboxValue(TEXTBOX_ID_SPEED_AUDIO));
            while (multiplier < 0.5)
            {
                ret += snprintf(buffer + ret, sizeof(buffer) - ret, "atempo=0.5,");
                multiplier *= 2;
            }
            ret += snprintf(buffer + ret, sizeof(buffer) - ret, "atempo=%f,", multiplier);

            ret += snprintf(
                buffer + ret,
                sizeof(buffer) - ret,
                "adelay=%s:1%s,aformat=%s%s[out_a]",
                getTextboxValue(TEXTBOX_ID_DELAY),
                getDropdownValue(DROPDOWN_ID_LOUDNORM_ENABLE)[0] == '\0' ? "" : ",loudnorm",
                !channelLayout ? "" : "channel_layouts=",
                !channelLayout ? "" : getDropdownValue(DROPDOWN_ID_CHANNEL_LAYOUT));
        }
        if (outputImage)
        {
            ret += snprintf(
                buffer + ret,
                sizeof(buffer) - ret,
                "[0:v]crop=%s:%s:%s:%s,scale=%s:%s[out_v]",
                getTextboxValue(TEXTBOX_ID_CROP_W),
                getTextboxValue(TEXTBOX_ID_CROP_H),
                getTextboxValue(TEXTBOX_ID_CROP_X),
                getTextboxValue(TEXTBOX_ID_CROP_Y),
                getTextboxValue(TEXTBOX_ID_SCALE_W),
                getTextboxValue(TEXTBOX_ID_SCALE_H));
        }
        ret += snprintf(
            buffer + ret,
            sizeof(buffer) - ret,
            "\" %s %s %s %s \"%s%c%s%s\"",
            outputVideo || outputImage ? "-map \"[out_v]\"" : "",
            outputVideo && gifInput ? "-c:v libx264 -movflags +faststart" : "",
            outputAudio ? "-map \"[out_a]\"" : "",
            outputImage ? "-vframes 1" : "",
            outputDir, slash, outputName, outputExt);

        if (ret < 0 || ret >= sizeof(buffer))
        {
            ERROR("Failed to write command into buffer.");
            return 9;
        }

        LOG("cmd = \"%s\"", buffer);

        // Create read/write pipes
        HANDLE hStdOutRead, hStdOutWrite;

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0);

        // Create child process
        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = hStdOutWrite;

        SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

        PROCESS_INFORMATION pi = {0};

        if (!CreateProcess(NULL, buffer, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        {
            ERROR("CreateProcess failed (%d)", GetLastError());
            CloseHandle(hStdOutRead);
            CloseHandle(hStdOutWrite);
            return GetLastError();
        }

        // Close write handles in parent
        CloseHandle(hStdOutWrite);

        streamData.convertProcess.process = pi.hProcess;
        streamData.convertProcess.stdoutRead = hStdOutRead;
        streamData.convertProcess.valid = true;
    #else

    #endif
    */
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
            "ffprobe -v error -show_streams -of json \"%s\"",
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
            return;
        }

        ret = fread(buffer, 1, sizeof(buffer), fp);
        pclose(fp);
        if (ret < sizeof(buffer) && ferror(fp))
        {
            ERROR("Failed to read command output.");
            return;
        }

        cJSON *json = cJSON_ParseWithLength(buffer, ret);
        if (json == NULL)
        {
            ERROR("Failed to parse output as JSON.");
            const char *errorPtr = cJSON_GetErrorPtr();
            if (errorPtr != NULL)
            {
                ERROR("Error before: %s", errorPtr);
            }
            return;
        }

        // cJSON_PrintPreallocated(json, buffer, sizeof(buffer) - 5, cJSON_True);
        // LOG("JSON:\n%s", buffer);

        cJSON *streams = cJSON_GetObjectItemCaseSensitive(json, "streams");
        if (streams == NULL)
        {
            ERROR("Failed to get streams object from JSON.");
            cJSON_Delete(json);
            return;
        }

        memset(&streamData, 0, sizeof(streamData));
        memset(&previewImageData.imageTexture, 0, sizeof(Texture2D));

        cJSON *stream;
        size_t i = 0;
        cJSON_ArrayForEach(stream, streams)
        {
            cJSON *codecType = cJSON_GetObjectItemCaseSensitive(stream, "codec_type");
            if (cJSON_IsString(codecType) && codecType->valuestring != NULL)
            {
                // LOG("codec_type = %s", codecType->valuestring);
                if (strcmp(codecType->valuestring, "video") == 0)
                {
                    // LOG("Detected stream %d as video", i);
                    streamData.streamCounts[STREAM_ID_VIDEO]++;
                }
                else if (strcmp(codecType->valuestring, "audio") == 0)
                {
                    // LOG("Detected stream %d as audio", i);
                    streamData.streamCounts[STREAM_ID_AUDIO]++;
                }
                else if (strcmp(codecType->valuestring, "subtitle") == 0)
                {
                    // LOG("Detected stream %d as subtitle", i);
                    streamData.streamCounts[STREAM_ID_SUBTITLES]++;
                }
            }
            else
            {
                ERROR("Failed to read streams[%d]", i);
            }

            i++;
        }
        // LOG("v: %zu, a: %zu, s: %zu", streamData.streamCounts[STREAM_ID_VIDEO], streamData.streamCounts[STREAM_ID_AUDIO], streamData.streamCounts[STREAM_ID_SUBTITLES]);

        memcpy(streamData.inputPath, trim(getTextboxValue(TEXTBOX_ID_INPUT_PATH)), TEXTBOX_BUFFER_SIZE);
        cJSON_Delete(json);
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

        FILE *fp = popen(buffer, "r");
        if (fp == NULL)
        {
            ERROR("Failed to execute command and establish pipe.");
            return;
        }

        int width;
        int height;
        ret = fscanf(fp, "%d,%d", &width, &height);
        pclose(fp);
        if (ret != 2)
        {
            ERROR("Failed to read command output.");
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

        ret = snprintf(
            buffer,
            sizeof(buffer),
            "ffmpeg -v error -i \"%s\" -vf \"crop=%d:%d:%d:%d,scale=%d:%d\" -vframes 1 -f rawvideo -pix_fmt rgba -",
            streamData.inputPath,
            cropWidth,
            cropHeight,
            cropOffsetX,
            cropOffsetY,
            scaleWidth,
            scaleHeight);

        if (ret < 0 || ret >= sizeof(buffer))
        {
            ERROR("Failed to write command into buffer.");
            return;
        }

        size_t imageBufferSize = scaleWidth * scaleHeight * 4;
        unsigned char *imageBuffer = calloc(imageBufferSize, sizeof(unsigned char));
        size_t totalBytesRead = 0;

#ifdef _WIN32
        // Create read/write pipes
        HANDLE hStdOutRead, hStdOutWrite;
        HANDLE hStdErrRead, hStdErrWrite;

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0);
        CreatePipe(&hStdErrRead, &hStdErrWrite, &sa, 0);

        // Create child process
        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = hStdOutWrite;
        si.hStdError = hStdErrWrite;

        PROCESS_INFORMATION pi = {0};

        if (!CreateProcess(NULL, buffer, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        {
            ERROR("CreateProcess failed (%d)", GetLastError());
            CloseHandle(hStdOutRead);
            CloseHandle(hStdErrRead);
            CloseHandle(hStdOutWrite);
            CloseHandle(hStdErrWrite);
            return;
        }

        // Close write handles in parent
        CloseHandle(hStdOutWrite);
        CloseHandle(hStdErrWrite);

        // Read from stdout
        DWORD nBytesRead;
        do
        {
            ReadFile(hStdOutRead, imageBuffer + totalBytesRead, imageBufferSize - totalBytesRead, &nBytesRead, NULL);
            totalBytesRead += nBytesRead;
        } while (nBytesRead > 0);

        unsigned char errBuffer[1024] = {0};
        DWORD nErrBytesRead;
        ReadFile(hStdErrRead, errBuffer, sizeof(errBuffer), &nErrBytesRead, NULL);
        if (nErrBytesRead > 0)
        {
            ERROR("%s", errBuffer);
            free(imageBuffer);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hStdOutRead);
            CloseHandle(hStdErrRead);
            return;
        }

        size_t overflow = 0;
        do
        {
            ReadFile(hStdOutRead, errBuffer + overflow, sizeof(errBuffer) - overflow, &nErrBytesRead, NULL);
        } while (nErrBytesRead > 0);
        if (overflow > 0)
        {
            ERROR("Received more bytes than expected (>=%zu).", overflow);
            free(imageBuffer);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hStdOutRead);
            CloseHandle(hStdErrRead);
            return;
        }

        // Wait for child process to exit and close handles
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hStdOutRead);
        CloseHandle(hStdErrRead);
#else
        // fp = popen(buffer, "r");
        // if (fp == NULL)
        // {
        //     ERROR("Failed to execute command and establish pipe.");
        //     return;
        // }

        // ret = 0
        // ret = fread(imageBuffer + ret, 1, imageBufferSize - ret, fp);
        // pclose(fp);
        // if (ret < imageBufferSize && ferror(fp))
        // {
        //     ERROR("Failed to read command output.");
        //     return;
        // }
#endif

        if (totalBytesRead != imageBufferSize)
        {
            ERROR("Total bytes read (%zu) less than image buffer size (%zu)", totalBytesRead, imageBufferSize);
            return;
        }

        Image image = {
            .data = imageBuffer,
            .width = scaleWidth,
            .height = scaleHeight,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            .mipmaps = 1,
        };
        previewImageData.imageTexture = LoadTextureFromImage(image);
        if (previewImageData.imageTexture.id == 0)
        {
            ERROR("Failed to load texture from image.");
        }
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
            strncpy(buffer->chars, outPath, TEXTBOX_BUFFER_SIZE);
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
        // textboxData.isDisabled[textboxId] = isDisabled;
        textboxData.isInit[textboxId] = true;
    }

    textboxData.textboxBuffers[textboxId].charsDefault = charsDefault;

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
        Clay_OnHover(onHoverFunction, userData);

        CLAY_TEXT(label, /*buttonData.isDisabled[buttonId]*/ isDisabled ? TEXT_CONFIG_FAINT : TEXT_CONFIG_BOLD);
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
        .convertProcess = {
#ifdef _WIN32
            .process = NULL,
            .stdoutRead = NULL,
#else
            .pid = 0,
            .stdoutfd = 0,
#endif
        },
    };

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

    NFD_Quit();
}

Clay_RenderCommandArray LayoutCreator_CreateLayout()
{
    Clay_BeginLayout();

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
        if (childPoll(&streamData.convertProcess))
        {
            char buffer[4096];
            int n = childRead(&streamData.convertProcess, buffer, sizeof(buffer));
            if (n > 0)
            {
                LOG("%s", buffer);
            }
        }

        if (childExited(&streamData.convertProcess))
        {
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
                    // switch (tabData.selectedTab)
                    // {
                    // case TAB_ID_DIMENSIONS:
                    //     RenderTabContentDimensions();
                    //     break;

                    // case TAB_ID_VIDEO:
                    //     RenderTabContentVideo();
                    //     break;

                    // case TAB_ID_AUDIO:
                    //     RenderTabContentAudio();
                    //     break;

                    // case TAB_ID_SUBTITLES:
                    //     RenderTabContentSubtitles();
                    //     break;

                    // case TAB_ID_DUMMY_LAST:
                    //     break;

                    // default:
                    //     ERROR("Invalid tab ID selected (%zu).", tabData.selectedTab);
                    //     break;
                    // }
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
            memcpy(buffer->chars, temp, TEXTBOX_BUFFER_SIZE);
            buffer->length = ret;
            buffer->cursorPosition = ret;
        }
        else
        {
            ERROR("Input file path is too long.");
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

    TextboxBuffer *buffer;
    if (textboxData.focusData.focusIndex >= 0)
    {
        buffer = &textboxData.textboxBuffers[textboxData.focusData.focusIndex];

        int key;
        while ((key = GetCharPressed()))
        {
            int charCodeLowerBound = ' ';
            int charCodeUpperBound = '~';
            if (buffer->numberboxConfig.isNumberbox)
            {
                charCodeLowerBound = '0';
                charCodeUpperBound = '9';
            }

            char *dotPosition = strchr(buffer->chars, '.');

            bool keyWithinBounds = key >= charCodeLowerBound && key <= charCodeUpperBound;
            bool validDecimalPoint = !buffer->numberboxConfig.isInt && key == '.' && dotPosition == NULL;

            if ((keyWithinBounds || validDecimalPoint) && buffer->length < TEXTBOX_BUFFER_SIZE - 1)
            {
                size_t i = ++buffer->length;
                for (i; i > buffer->cursorPosition; i--)
                {
                    buffer->chars[i] = buffer->chars[i - 1];
                }
                buffer->chars[buffer->cursorPosition] = key;
                buffer->cursorPosition++;

                if (buffer->numberboxConfig.isNumberbox)
                {
                    if (key == '.')
                    {
                        // Ensure valid float from decimal point input
                        if (buffer->length == 1)
                        {
                            strcpy(buffer->chars, "0.");
                            buffer->length = 2;
                            buffer->cursorPosition = 2;
                        }

                        // Clamp value if decimal point changes it
                        if (buffer->cursorPosition != buffer->length)
                        {
                            int charsWritten = clampFloatAsString(buffer->chars,
                                                                  buffer->numberboxConfig.min,
                                                                  buffer->numberboxConfig.max);
                            buffer->length = charsWritten;
                            buffer->cursorPosition = charsWritten;
                        }
                    }
                    else if (key == '0')
                    {
                        // If leading/trailing zero, don't rewrite
                        char *firstSigPosition;
                        char *lastSigPosition;

                        for (firstSigPosition = buffer->chars; firstSigPosition < dotPosition; firstSigPosition++)
                        {
                            if (*firstSigPosition != '0')
                            {
                                break;
                            }
                        }
                        for (lastSigPosition = buffer->chars + buffer->length - 1; lastSigPosition > dotPosition; lastSigPosition--)
                        {
                            if (*lastSigPosition != '0')
                            {
                                break;
                            }
                        }

                        bool leadingZero = buffer->cursorPosition - 1 <= firstSigPosition - buffer->chars;
                        bool trailingZero = buffer->cursorPosition - 1 >= lastSigPosition - buffer->chars;

                        if (!(leadingZero || trailingZero))
                        {
                            int charsWritten = clampFloatAsString(buffer->chars,
                                                                  buffer->numberboxConfig.min,
                                                                  buffer->numberboxConfig.max);
                            buffer->length = charsWritten;
                            buffer->cursorPosition = charsWritten;
                        }
                    }
                    else
                    {
                        // Any number changes value, so clamp it
                        int charsWritten = clampFloatAsString(buffer->chars,
                                                              buffer->numberboxConfig.min,
                                                              buffer->numberboxConfig.max);
                        buffer->length = charsWritten;
                        buffer->cursorPosition = charsWritten;
                    }
                }

                textboxData.focusData.focusStartTime = GetTime();
            }
        }

        bool nonZeroCursorPosition = buffer->cursorPosition > 0;
        bool nonMaxCursorPosition = buffer->cursorPosition < buffer->length;

        bool isCtrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        bool isShiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        // Handle BACKSPACE
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && nonZeroCursorPosition)
        {
            int offset = 0;

            if (isCtrlDown)
            {
                while ((int)buffer->cursorPosition - 1 - offset >= 0 &&
                       charMatchesAny(buffer->chars[buffer->cursorPosition - 1 - offset], " ./\\"))
                {
                    offset++;
                }
                while ((int)buffer->cursorPosition - 1 - offset >= 0 &&
                       !charMatchesAny(buffer->chars[buffer->cursorPosition - 1 - offset], " ./\\"))
                {
                    offset++;
                }
            }
            else
            {
                offset = 1;
            }

            for (size_t i = buffer->cursorPosition; i <= buffer->length; i++)
            {
                buffer->chars[i - offset] = buffer->chars[i];
            }
            buffer->cursorPosition -= offset;
            buffer->length -= offset;
            textboxData.focusData.focusStartTime = GetTime();

            if (buffer->numberboxConfig.isNumberbox)
            {
                // Ensure valid float after deletion
                if (buffer->length == 1 && buffer->chars[0] == '.')
                {
                    strcpy(buffer->chars, "0.");
                    buffer->length = 2;
                    buffer->cursorPosition += 1;
                }
                else if (buffer->length > 1)
                {
                    int charsWritten = clampFloatAsString(buffer->chars, buffer->numberboxConfig.min, buffer->numberboxConfig.max);
                    buffer->length = charsWritten;
                    if (buffer->cursorPosition > buffer->length)
                    {
                        buffer->cursorPosition = charsWritten;
                    }
                }
            }
        }

        // Handle DELETE
        else if ((IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE)) && nonMaxCursorPosition)
        {
            int offset = 0;

            if (isCtrlDown)
            {
                while ((int)buffer->cursorPosition + offset < buffer->length &&
                       charMatchesAny(buffer->chars[buffer->cursorPosition + offset], " ./\\"))
                {
                    offset++;
                }
                while ((int)buffer->cursorPosition + offset < buffer->length &&
                       !charMatchesAny(buffer->chars[buffer->cursorPosition + offset], " ./\\"))
                {
                    offset++;
                }
            }
            else
            {
                offset = 1;
            }

            for (size_t i = buffer->cursorPosition; i <= buffer->length - offset; i++)
            {
                buffer->chars[i] = buffer->chars[i + offset];
            }
            buffer->length -= offset;
            textboxData.focusData.focusStartTime = GetTime();

            if (buffer->numberboxConfig.isNumberbox)
            {
                // Ensure valid float after deletion
                if (buffer->length == 1 && buffer->chars[0] == '.')
                {
                    strcpy(buffer->chars, "0.");
                    buffer->length = 2;
                    buffer->cursorPosition += 1;
                }
                else if (buffer->length > 1)
                {
                    int charsWritten = clampFloatAsString(buffer->chars, buffer->numberboxConfig.min, buffer->numberboxConfig.max);
                    buffer->length = charsWritten;
                    if (buffer->cursorPosition > buffer->length)
                    {
                        buffer->cursorPosition = charsWritten;
                    }
                }
            }
        }

        // Handle LEFT
        else if ((IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) && nonZeroCursorPosition)
        {
            if (isCtrlDown)
            {
                while (buffer->cursorPosition > 0 &&
                       charMatchesAny(buffer->chars[buffer->cursorPosition - 1], " ./\\"))
                {
                    buffer->cursorPosition--;
                }
                while (buffer->cursorPosition > 0 &&
                       !charMatchesAny(buffer->chars[buffer->cursorPosition - 1], " ./\\"))
                {
                    buffer->cursorPosition--;
                }
            }
            else
            {
                buffer->cursorPosition--;
            }
            textboxData.focusData.focusStartTime = GetTime();
        }

        // Handle RIGHT
        else if ((IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) && nonMaxCursorPosition)
        {
            if (isCtrlDown)
            {
                while (buffer->cursorPosition < buffer->length &&
                       charMatchesAny(buffer->chars[buffer->cursorPosition], " ./\\"))
                {
                    buffer->cursorPosition++;
                }
                while (buffer->cursorPosition < buffer->length &&
                       !charMatchesAny(buffer->chars[buffer->cursorPosition], " ./\\"))
                {
                    buffer->cursorPosition++;
                }
            }
            else
            {
                buffer->cursorPosition++;
            }
            textboxData.focusData.focusStartTime = GetTime();
        }

        // Handle TAB while focused
        else if (IsKeyPressed(KEY_TAB) || IsKeyPressedRepeat(KEY_TAB))
        {
            if (isShiftDown)
            {
                // do
                // {
                textboxData.focusData.focusIndex = (textboxData.focusData.focusIndex + TEXTBOX_ID_DUMMY_LAST - 1) % TEXTBOX_ID_DUMMY_LAST;
                // } while (textboxData.isDisabled[textboxData.focusData.focusIndex]);
            }
            else
            {
                // do
                // {
                textboxData.focusData.focusIndex = (textboxData.focusData.focusIndex + 1) % TEXTBOX_ID_DUMMY_LAST;
                // } while (textboxData.isDisabled[textboxData.focusData.focusIndex]);
            }
            buffer = &textboxData.textboxBuffers[textboxData.focusData.focusIndex];
            buffer->cursorPosition = buffer->length;
            textboxData.focusData.focusStartTime = GetTime();
        }
    }
    else
    {
        // Handle TAB while unfocused
        if (IsKeyPressed(KEY_TAB))
        {
            // do
            // {
            textboxData.focusData.focusIndex = (textboxData.focusData.focusIndex + 1) % TEXTBOX_ID_DUMMY_LAST;
            // } while (textboxData.isDisabled[textboxData.focusData.focusIndex]);
            buffer = &textboxData.textboxBuffers[textboxData.focusData.focusIndex];
            buffer->cursorPosition = buffer->length;
            textboxData.focusData.focusStartTime = GetTime();
        }
    }

    // Unfocus on empty left-click
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && !textboxData.focusData.focusRegistered)
    {
        textboxData.focusData.focusIndex = -1;
    }

    SetMouseCursor(IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) ? MOUSE_CURSOR_RESIZE_ALL : textboxData.hoveredTextbox >= 0 ? MOUSE_CURSOR_IBEAM
                                                                                                                      : MOUSE_CURSOR_DEFAULT);

    return Clay_EndLayout();
}