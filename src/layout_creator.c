#include "clay.h"
#include "raylib.h"
#include "nfd.h"
#include "cJSON.h"
#include <stdio.h>

#ifdef _WIN32
// #include <windows.h>
#include <windef.h>
#include <winbase.h>
#include <processthreadsapi.h>
#include <errhandlingapi.h>
#include <namedpipeapi.h>
#else
#include <unistd.h>
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
    FONT_ID_SYMBOL,
    FONT_ID_DUMMY_LAST
} FontID;

typedef enum
{
    TEXTBOX_ID_INPUT_PATH,
    TEXTBOX_ID_FPS,
    TEXTBOX_ID_DURATION_START,
    TEXTBOX_ID_DURATION_END,
    TEXTBOX_ID_SPEED_VIDEO,
    TEXTBOX_ID_SCALE_W,
    TEXTBOX_ID_SCALE_H,
    TEXTBOX_ID_CROP_W,
    TEXTBOX_ID_CROP_H,
    TEXTBOX_ID_CROP_X,
    TEXTBOX_ID_CROP_Y,
    TEXTBOX_ID_VOLUME,
    TEXTBOX_ID_SPEED_AUDIO,
    TEXTBOX_ID_DELAY,
    TEXTBOX_ID_OUTPUT_PATH,
    TEXTBOX_ID_DUMMY_LAST
} TextboxID;

typedef enum
{
    DROPDOWN_ID_OUTPUT_TYPE,
    DROPDOWN_ID_LOUDNORM_ENABLE,
    DROPDOWN_ID_CHANNEL_LAYOUT,
    DROPDOWN_ID_DUMMY_LAST
} DropdownID;

typedef enum
{
    BUTTON_ID_BROWSE_INPUT,
    BUTTON_ID_LOAD_INPUT,
    BUTTON_ID_LOAD_PREVIEW,
    BUTTON_ID_BROWSE_OUTPUT,
    BUTTON_ID_CONVERT,
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
    // SCROLL_ID_TABBEDBOXCONTENT,
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
    // bool isDisabled[TEXTBOX_ID_DUMMY_LAST];
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
    // bool isDisabled[BUTTON_ID_DUMMY_LAST];
} ButtonData;

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
    const char *name;
    const char *extensions;
} FormatData;

typedef struct
{
    char inputPath[TEXTBOX_BUFFER_SIZE];
    size_t streamCounts[STREAM_ID_DUMMY_LAST];
    Texture2D imagePreviewTexture;
} StreamData;

FontData fontData;
TextboxData textboxData;
DropdownData dropdownData;
// ButtonData buttonData = {0};
TabData tabData;
ScrollData scrollData;
StreamData streamData;

// FormatData formatVideoData[FORMAT_VIDEO_ID_DUMMY_LAST] = {
//     {.name = "GIF", .extensions = "gif"},
//     {.name = "MKV", .extensions = "mkv"},
//     {.name = "MOV", .extensions = "mov"},
//     {.name = "MP4", .extensions = "mp4"},
//     {.name = "WEBM", .extensions = "webm"},
// };

// FormatData formatAudioData[FORMAT_AUDIO_ID_DUMMY_LAST] = {
//     {.name = "FLAC", .extensions = "flac"},
//     {.name = "M4A", .extensions = "m4a"},
//     {.name = "MP3", .extensions = "mp3"},
//     {.name = "OGG", .extensions = "ogg,oga"},
//     {.name = "OPUS", .extensions = "opus"},
//     {.name = "WAV", .extensions = "wav,wave"},
// };

// FormatData formatImageData[FORMAT_IMAGE_ID_DUMMY_LAST] = {
//     {.name = "JPEG", .extensions = "jpg,jpeg,jpe,jfif"},
//     {.name = "PNG", .extensions = "png"},
//     {.name = "TIFF", .extensions = "tiff,tif"},
//     {.name = "WEBP", .extensions = "webp"},
// };

Clay_Padding defaultBoxPadding;
Clay_Padding buttonPadding;
Clay_Padding dropdownPadding;

Clay_ElementDeclaration styleFilters;
Clay_ElementDeclaration styleFilterGroup;
Clay_ElementDeclaration styleFilterItemGroup;
Clay_ElementDeclaration styleFilterItem;
Clay_ElementDeclaration styleFilterItemLabel;
Clay_ElementDeclaration styleFilterItemField;

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

int convert()
{
    char buffer[4096];

    // Validate input path
    const char *inputPath = getTextboxValue(TEXTBOX_ID_INPUT_PATH);
    if (!FileExists(inputPath))
    {
        ERROR("Input file does not exist (\"%s\").", inputPath);
        return 1;
    }
    // if (!IsFileExtension(inputPath, getDropdownValue(DROPDOWN_ID_STREAM_TYPE)))
    // {
    //     ERROR("Incorrect input file format (%s).", GetFileExtension(inputPath));
    //     return 2;
    // }

    // Validate output path
    const char *outputPath = getTextboxValue(TEXTBOX_ID_OUTPUT_PATH);
    if (outputPath[0] == '\0')
    {
        ERROR("Output file is missing.");
        return 3;
    }
    const char *outputDir = GetDirectoryPath(outputPath);
    if (!DirectoryExists(outputDir))
    {
        ERROR("Output directory does not exist (%s).", outputDir);
        return 4;
    }
    const char *outputName = GetFileNameWithoutExt(outputPath);
    if (!IsFileNameValid(outputName))
    {
        ERROR("Output filename is invalid (%s).", outputName);
        return 5;
    }

#ifdef _WIN32
    int ret = snprintf(
        buffer,
        sizeof(buffer),
        "ffmpeg -y -i \"%s\" -vf \""
        "fps=%s,"
        "trim=%s:%s,"
        "setpts=(PTS-STARTPTS)/%s,"
        "scale=%s:%s,"
        "crop=%s:%s:%s:%s\" "
        "\"%s\"",
        inputPath,
        getTextboxValue(TEXTBOX_ID_FPS),
        getTextboxValue(TEXTBOX_ID_DURATION_START),
        getTextboxValue(TEXTBOX_ID_DURATION_END),
        getTextboxValue(TEXTBOX_ID_SPEED_VIDEO),
        getTextboxValue(TEXTBOX_ID_SCALE_W),
        getTextboxValue(TEXTBOX_ID_SCALE_H),
        getTextboxValue(TEXTBOX_ID_CROP_W),
        getTextboxValue(TEXTBOX_ID_CROP_H),
        getTextboxValue(TEXTBOX_ID_CROP_X),
        getTextboxValue(TEXTBOX_ID_CROP_Y),
        outputPath);

    if (ret < 0 || ret >= sizeof(buffer))
    {
        ERROR("Failed to write command into buffer.");
        return 5;
    }

    LOG("cmd = \"%s\"", buffer);

    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);

    if (!CreateProcess(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        ERROR("CreateProcess failed (%d)", GetLastError());
        return GetLastError();
    }
#else

#endif

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

        memcpy(streamData.inputPath, getTextboxValue(TEXTBOX_ID_INPUT_PATH), TEXTBOX_BUFFER_SIZE);
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
        // LOG("%d,%d", width, height);

        // LOG("%s,%s", getTextboxValue(TEXTBOX_ID_SCALE_W), getTextboxValue(TEXTBOX_ID_SCALE_H));
        int scaledWidth = atoi(getTextboxValue(TEXTBOX_ID_SCALE_W));
        int scaledHeight = atoi(getTextboxValue(TEXTBOX_ID_SCALE_H));
        if (scaledWidth == -1 && scaledHeight == -1)
        {
            ERROR("Both scaled dimensions are -1.");
            return;
        }
        if (scaledWidth == -1)
        {
            scaledWidth = width * scaledHeight / height;
        }
        else if (scaledWidth == 0)
        {
            scaledWidth = width;
        }
        if (scaledHeight == -1)
        {
            scaledHeight = height * scaledWidth / width;
        }
        else if (scaledHeight == 0)
        {
            scaledHeight = height;
        }

        // LOG("scaledWidth = %d, scaledHeight = %d", scaledWidth, scaledHeight);
        ret = snprintf(
            buffer,
            sizeof(buffer),
            "ffmpeg -v error -i \"%s\" -vf \"scale=%d:%d\" -vframes 1 -f rawvideo -pix_fmt rgba -",
            streamData.inputPath,
            scaledWidth,
            scaledHeight);
        // LOG("Command: %s", buffer);

        if (ret < 0 || ret >= sizeof(buffer))
        {
            ERROR("Failed to write command into buffer.");
            return;
        }

        size_t imageBufferSize = scaledWidth * scaledHeight * 4;
        // LOG("imageBufferSize = %zu", imageBufferSize);
        unsigned char *imageBuffer = calloc(imageBufferSize, sizeof(unsigned char));
        size_t totalBytesRead = 0;

#ifdef _WIN32
        // Create read/write pipes
        HANDLE hStdOutRead, hStdOutWrite;
        // HANDLE hStdErrRead, hStdErrWrite;

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0);
        // CreatePipe(&hStdErrRead, &hStdErrWrite, &sa, 0);

        // Create child process
        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = hStdOutWrite;
        // si.hStdError = hStdErrWrite;

        PROCESS_INFORMATION pi = {0};
        CreateProcess(NULL, buffer, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

        // Close write handles in parent
        CloseHandle(hStdOutWrite);
        // CloseHandle(hStdErrWrite);

        // Read from stdout
        DWORD nBytesRead;
        do
        {
            ReadFile(hStdOutRead, imageBuffer + totalBytesRead, imageBufferSize - totalBytesRead, &nBytesRead, NULL);
            totalBytesRead += nBytesRead;
            // LOG("nBytesRead = %zu, total = %zu", nBytesRead, total);
        } while (nBytesRead > 0);

        // size_t errBufferSize = 20000;
        // unsigned char *errBuffer = calloc(errBufferSize, sizeof(unsigned char));
        // DWORD nErrBytesRead;
        // ReadFile(hStdErrRead, errBuffer, errBufferSize, &nErrBytesRead, NULL);
        // if (nErrBytesRead > 0)
        // {
        //     ERROR("%s", errBuffer);
        // }

        // size_t errTotal = 0;
        // do
        // {
        //     ReadFile(hStdOutRead, errBuffer + errTotal, errBufferSize - errTotal, &nErrBytesRead, NULL);
        // } while (nErrBytesRead > 0);
        // if (errTotal > 0)
        // {
        //     ERROR("Received more bytes than expected (>=%zu).", errTotal);
        //     free(imageBuffer);
        //     free(errBuffer);
        //     return;
        // }
        // free(errBuffer);

        // Wait for child process to exit and close handles
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hStdOutRead);
        // CloseHandle(hStdErrRead);
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
            .width = scaledWidth,
            .height = scaledHeight,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            .mipmaps = 1,
        };
        streamData.imagePreviewTexture = LoadTextureFromImage(image);
        if (streamData.imagePreviewTexture.id == 0)
        {
            ERROR("Failed to load texture from image.");
        }
        UnloadImage(image);
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
                {"Videos", "mp4,mov,mkv,webm,flv,mpeg,gif"},
                {"GIFs", "gif"},
            };
            nfdopendialogu8args_t openDialogArgs = {
                .filterList = inputFilters,
                .filterCount = 2,
            };
            result = NFD_OpenDialogU8_With(&outPath, &openDialogArgs);
            break;

        case TEXTBOX_ID_OUTPUT_PATH:
            nfdu8filteritem_t outputFilters[] = {
                {"Videos", "mp4,mov,mkv,webm,flv,mpeg"},
                {"GIFs", "gif"},
            };
            nfdsavedialogu8args_t saveDialogArgs = {
                .filterList = outputFilters,
                .filterCount = 2,
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
        int result = convert();

        if (result)
        {
            ERROR("Convert failed (%d)", result);
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
            .backgroundColor = /*textboxData.isDisabled[textboxId] ? COLOR_BG_TEXTBOX_DISABLED :*/ COLOR_BG_TEXTBOX,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
            .border = {
                .color = focused ? COLOR_BORDER_TEXTBOX_FOCUSED : COLOR_BORDER_TEXTBOX,
                .width = CLAY_BORDER_OUTSIDE(borderWidth),
            },
        })
        {
            Clay_OnHover(HandleTextboxInteraction, textboxId);
            if (Clay_Hovered())
            {
                textboxData.hoveredTextbox = textboxId;
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

                    // if (textboxData.isDisabled[textboxId])
                    // {
                    //     CLAY_TEXT(textBeforeCursor, TEXT_CONFIG_FAINT);
                    //     CLAY_TEXT(textAfterCursor, TEXT_CONFIG_FAINT);
                    // }
                    // else
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
            CLAY({.layout = {.childGap = fontData.charWidth * 2}})
            {
                CLAY_TEXT(str, TEXT_CONFIG_DEFAULT);
                CLAY_TEXT(expandDropdown ? CLAY_STRING("▲") : dropdownSize > 1 ? CLAY_STRING("▼")
                                                                               : CLAY_STRING("▽"),
                          TEXT_CONFIG_SYMBOL);
            }

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

void setStyles()
{
    size_t charHeightOverFour = fontData.charHeight / 4;

    defaultBoxPadding = (Clay_Padding){
        fontData.charWidth,
        fontData.charWidth,
        charHeightOverFour,
        charHeightOverFour,
    };

    buttonPadding = (Clay_Padding){
        fontData.charWidth * 3,
        fontData.charWidth * 3,
        charHeightOverFour,
        charHeightOverFour,
    };

    dropdownPadding = (Clay_Padding){
        0,
        0,
        charHeightOverFour,
        charHeightOverFour,
    };

    styleFilters = (Clay_ElementDeclaration){
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childGap = charHeightOverFour * 6,
        },
    };

    styleFilterGroup = (Clay_ElementDeclaration){
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_FIXED(300),
            },
            .childGap = charHeightOverFour * 3,
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
    for (size_t i = 0; i < TAB_ID_DUMMY_LAST; i++)
    {
        tabData.isDisabled[i] = true;
    }

    fontData = (FontData){
        .fontSize = 16,
        .fontSizeMin = 8,
        .fontSizeMax = 32,
        .charWidth = 8,
        .charHeight = 16,
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

    // ButtonData buttonData = {0};

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
        .imagePreviewTexture = {0},
    };

    setStyles();

    NFD_Init();
}

void LayoutCreator_Destroy()
{
    NFD_Quit();
}

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

Clay_RenderCommandArray LayoutCreator_CreateLayout()
{
    Clay_BeginLayout();

    textboxData.hoveredTextbox = -1;
    textboxData.focusData.focusRegistered = false;

    scrollData.data[SCROLL_ID_WINDOW] = Clay_GetScrollContainerData(CLAY_ID("WindowContainer"));
    // scrollData.data[SCROLL_ID_TABBEDBOXCONTENT] = Clay_GetScrollContainerData(CLAY_ID("TabbedBoxContent"));
    bool scrollingWindowVertical = false;
    bool scrollingWindowHorizontal = false;
    if (scrollData.data[SCROLL_ID_WINDOW].found)
    {
        scrollingWindowVertical = scrollData.data[SCROLL_ID_WINDOW].scrollContainerDimensions.height < scrollData.data[SCROLL_ID_WINDOW].contentDimensions.height;
        scrollingWindowHorizontal = scrollData.data[SCROLL_ID_WINDOW].scrollContainerDimensions.width < scrollData.data[SCROLL_ID_WINDOW].contentDimensions.width;
    }
    // bool scrollingTabbedBoxContent = scrollData.data[SCROLL_ID_TABBEDBOXCONTENT].found && scrollData.data[SCROLL_ID_TABBEDBOXCONTENT].scrollContainerDimensions.width < scrollData.data[SCROLL_ID_TABBEDBOXCONTENT].contentDimensions.width;
    // LOG("scrollContainerDimensions.width = %g, contentDimensions.width = %g", scrollData.data[SCROLL_ID_TABBEDBOXCONTENT].scrollContainerDimensions.width, scrollData.data[SCROLL_ID_TABBEDBOXCONTENT].contentDimensions.width);

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
                        .childGap = 4,
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
                    tabData.isDisabled[TAB_ID_SUBTITLES] = streamData.streamCounts[STREAM_ID_SUBTITLES] == 0 || outputType != 'v';

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
                        .padding = /*scrollingTabbedBoxContent ? (Clay_Padding){16, 16, 16, 32} :*/ CLAY_PADDING_ALL(16),
                        .childGap = 32,
                    },
                    .backgroundColor = COLOR_BG_TAB_SELECTED,
                    .cornerRadius = (Clay_CornerRadius){0, 0, 16, 16},
                    .border = {
                        .color = COLOR_BORDER_TAB,
                        .width = (Clay_BorderWidth){1, 1, 0, 1, 0},
                    },
                    // .clip = {
                    //     .horizontal = true,
                    //     .childOffset = Clay_GetScrollOffset(),
                    // },
                })
                {
                    // if (scrollingTabbedBoxContent)
                    // {
                    //     RenderScrollBar(SCROLL_ID_TABBEDBOXCONTENT, false, true);
                    // }

                    switch (tabData.selectedTab)
                    {
                    case TAB_ID_DIMENSIONS:
                        CLAY(styleFilters)
                        {
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
                                                false,
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
                                                false,
                                                6,
                                                textboxData.textboxBuffers[TEXTBOX_ID_SCALE_W].chars[0] == '\0' ? "in_h" : "-1");
                                        }
                                    }
                                }
                            }

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
                                                (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 0, .max = FLOAT_MAX},
                                                false,
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
                                                (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 0, .max = FLOAT_MAX},
                                                false,
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
                                                false,
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
                                                false,
                                                6,
                                                "0");
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
                            RenderButton(
                                CLAY_STRING("Load Preview"),
                                BUTTON_ID_LOAD_PREVIEW,
                                false,
                                HandleLoadPreviewButtonInteraction,
                                0,
                                buttonPadding);

                            CLAY({
                                .layout = {
                                    .sizing = {
                                        .width = CLAY_SIZING_FIXED(512),
                                        .height = CLAY_SIZING_FIXED(512),
                                    },
                                },
                                // .backgroundColor = COLOR_GRAY,
                                .image = {
                                    .imageData = &streamData.imagePreviewTexture,
                                },
                            })
                            {
                            }
                        }
                        break;

                    case TAB_ID_VIDEO:
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
                                                false,
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
                                            RenderTextbox(
                                                CLAY_STRING(""),
                                                TEXTBOX_ID_DURATION_START,
                                                (NumberboxConfig){.isNumberbox = true, .min = 0, .max = FLOAT_MAX},
                                                false,
                                                6,
                                                "0.0");
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
                                            RenderTextbox(
                                                CLAY_STRING(""),
                                                TEXTBOX_ID_DURATION_END,
                                                (NumberboxConfig){.isNumberbox = true, .min = 0, .max = FLOAT_MAX},
                                                false,
                                                6,
                                                "");
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
                                                false,
                                                4,
                                                speedAudio[0] == '\0' ? "1.0" : speedAudio);
                                        }
                                    }
                                }
                            }
                        }
                        break;

                    case TAB_ID_AUDIO:
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
                                                false,
                                                4,
                                                "1.0");
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
                                                               {"Stereo", "stereo"},
                                                               {"Mono", "mono"},
                                                               DROPDOWN_OPTION_NULL,
                                                           });
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
                                                false,
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
                                                false,
                                                5,
                                                "0");
                                        }
                                    }
                                }
                            }
                        }
                        break;

                    case TAB_ID_SUBTITLES:
                        break;

                    case TAB_ID_DUMMY_LAST:
                        break;

                    default:
                        ERROR("Reached default case (TabbedBoxContent).");
                        break;
                    }
                }
            }

            CLAY({
                .layout = {
                    .childGap = fontData.charWidth * 2,
                    .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}},
            })
            {
                CLAY({
                    .layout = {
                        .childGap = 4,
                        .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}},
                })
                {
                    RenderTextbox(
                        CLAY_STRING("Output File:"),
                        TEXTBOX_ID_OUTPUT_PATH,
                        (NumberboxConfig){0},
                        false,
                        30,
                        "");

                    RenderButton(
                        CLAY_STRING("..."),
                        BUTTON_ID_BROWSE_OUTPUT,
                        false,
                        HandleBrowseButtonInteraction,
                        TEXTBOX_ID_OUTPUT_PATH,
                        defaultBoxPadding);
                }

                RenderButton(
                    CLAY_STRING("Convert"),
                    BUTTON_ID_CONVERT,
                    trim(getTextboxValue(TEXTBOX_ID_OUTPUT_PATH))[0] == '\0',
                    HandleConvertButtonInteraction,
                    0,
                    buttonPadding);
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