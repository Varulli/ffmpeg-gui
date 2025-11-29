#include "clay.h"
#include "raylib.h"
#include "nfd.h"
#include "cJSON.h"
#include <stdio.h>

#ifdef _WIN32
#include <processthreadsapi.h>
#include <errhandlingapi.h>
#else
#include <unistd.h>
#endif

#define TEXTBOX_BUFFER_SIZE 256
#define FLOAT_MAX 1e5f

#define CHAR_W 8
#define CHAR_H 16

#define TEXT_CONFIG_DEFAULT CLAY_TEXT_CONFIG({ \
    .fontId = FONT_ID_BODY_16,                 \
    .fontSize = 16,                            \
    .textColor = COLOR_WHITE,                  \
})
#define TEXT_CONFIG_FAINT CLAY_TEXT_CONFIG({ \
    .fontId = FONT_ID_BODY_16,               \
    .fontSize = 16,                          \
    .textColor = COLOR_LIGHTGRAY,            \
})
#define TEXT_CONFIG_BOLD CLAY_TEXT_CONFIG({ \
    .fontId = FONT_ID_BOLD_16,              \
    .fontSize = 16,                         \
    .textColor = COLOR_WHITE,               \
})
// #define TEXT_CONFIG_SYMBOL CLAY_TEXT_CONFIG({ \
//     .fontId = FONT_ID_SYMBOL_16,              \
//     .fontSize = 16,                           \
//     .textColor = COLOR_WHITE,                 \
// })

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
    FONT_ID_BODY_16,
    FONT_ID_BOLD_16,
    FONT_ID_SYMBOL_16,
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
    BUTTON_ID_CONVERT,
    BUTTON_ID_BROWSE_INPUT,
    BUTTON_ID_LOAD_INPUT,
    BUTTON_ID_BROWSE_OUTPUT,
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
    cJSON *streams;
} StreamData;

TextboxData textboxData = {
    .hoveredTextbox = -1,
    .focusData = {
        .focusRegistered = false,
        .focusIndex = -1,
    },
};

DropdownData dropdownData = {
    .hoveredOption = 0,
    .hoveredValue = NULL,
};

// ButtonData buttonData = {0};

TabData tabData = {
    .selectedTab = TAB_ID_DIMENSIONS,
};

ScrollData scrollData = {
    .scrolling = -1,
};

FormatData formatVideoData[FORMAT_VIDEO_ID_DUMMY_LAST] = {
    {.name = "GIF", .extensions = "gif"},
    {.name = "MKV", .extensions = "mkv"},
    {.name = "MOV", .extensions = "mov"},
    {.name = "MP4", .extensions = "mp4"},
    {.name = "WEBM", .extensions = "webm"},
};

FormatData formatAudioData[FORMAT_AUDIO_ID_DUMMY_LAST] = {
    {.name = "FLAC", .extensions = "flac"},
    {.name = "M4A", .extensions = "m4a"},
    {.name = "MP3", .extensions = "mp3"},
    {.name = "OGG", .extensions = "ogg,oga"},
    {.name = "OPUS", .extensions = "opus"},
    {.name = "WAV", .extensions = "wav,wave"},
};

FormatData formatImageData[FORMAT_IMAGE_ID_DUMMY_LAST] = {
    {.name = "JPEG", .extensions = "jpg,jpeg,jpe,jfif"},
    {.name = "PNG", .extensions = "png"},
    {.name = "TIFF", .extensions = "tiff,tif"},
    {.name = "WEBP", .extensions = "webp"},
};

StreamData streamData = {0};

Clay_Padding defaultBoxPadding = {
    CHAR_W,
    CHAR_W,
    CHAR_H / 4,
    CHAR_H / 4,
};

Clay_Padding buttonPadding = {
    CHAR_W * 3,
    CHAR_W * 3,
    CHAR_H / 4,
    CHAR_H / 4,
};

Clay_Padding dropdownPadding = {
    0,
    0,
    CHAR_H / 4,
    CHAR_H / 4,
};

Clay_ElementDeclaration styleFilters = {
    .layout = {
        .layoutDirection = CLAY_TOP_TO_BOTTOM,
        .sizing = {
            .width = CLAY_SIZING_GROW(0),
        },
        .childGap = 20,
    },
};

Clay_ElementDeclaration styleFilterGroup = {
    .layout = {
        .layoutDirection = CLAY_TOP_TO_BOTTOM,
        .sizing = {
            .width = CLAY_SIZING_GROW(0, 250),
        },
        .childGap = 12,
    },
    .border = {
        .color = COLOR_GRAY,
        .width = (Clay_BorderWidth){0, 0, 0, 0, 1},
    },
};

Clay_ElementDeclaration styleFilterItemGroup = {
    .layout = {
        .layoutDirection = CLAY_TOP_TO_BOTTOM,
        .sizing = {
            .width = CLAY_SIZING_GROW(0),
        },
        .childGap = 8,
    },
};

Clay_ElementDeclaration styleFilterItem = {
    .layout = {
        .sizing = {
            .width = CLAY_SIZING_GROW(0),
        },
        .childGap = 8,
    },
};

Clay_ElementDeclaration styleFilterItemLabel = {
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

Clay_ElementDeclaration styleFilterItemField = {
    .layout = {
        .sizing = {
            .width = CLAY_SIZING_GROW(0),
        },
    },
};

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
            "ffprobe -v error -show_streams -of json \"%s\"", getTextboxValue(TEXTBOX_ID_INPUT_PATH));

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
            pclose(fp);
            return;
        }

        // cJSON_PrintPreallocated(json, buffer, sizeof(buffer) - 5, cJSON_True);
        // LOG("JSON:\n%s", buffer);

        cJSON *streams = cJSON_GetObjectItemCaseSensitive(json, "streams");
        if (streams == NULL)
        {
            ERROR("Failed to get streams object from JSON.");
            cJSON_Delete(json);
            pclose(fp);
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
        LOG("v: %zu, a: %zu, s: %zu", streamData.streamCounts[STREAM_ID_VIDEO], streamData.streamCounts[STREAM_ID_AUDIO], streamData.streamCounts[STREAM_ID_SUBTITLES]);

        memcpy(&streamData.inputPath, getTextboxValue(TEXTBOX_ID_INPUT_PATH), TEXTBOX_BUFFER_SIZE);

        cJSON_Delete(json);
        pclose(fp);

        dropdownData.isInit[DROPDOWN_ID_OUTPUT_TYPE] = false;
        LOG("%zu", tabData.selectedTab);
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

void HandleThumbInteraction(
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
    }
    else if (pointerData.state == CLAY_POINTER_DATA_RELEASED)
    {
        scrollData.scrolling = -1;
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
        textboxData.textboxBuffers[textboxId].charsDefault = charsDefault;
        textboxData.textboxBuffers[textboxId].numberboxConfig = numberboxConfig;
        // textboxData.isDisabled[textboxId] = isDisabled;
        textboxData.isInit[textboxId] = true;
    }

    bool focused = (textboxData.focusData.focusIndex == textboxId);

    CLAY({
        .id = CLAY_IDI("Textbox", textboxId),
        .layout = {
            .childGap = CHAR_W,
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
                                .min = CHAR_W * (maxCharsDisplayed + 2),
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

    size_t maxLength = 0;
    // size_t maxLength = strlen(options[dropdownData.selectedOptions[dropdownId]].label) + 2;
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
            .childGap = CHAR_W,
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

        // Clay_Sizing dropdownSizing = {.width = {
        //                                   .size = {
        //                                       .minMax = {.min = CHAR_W * (maxLength + 2)},
        //                                   },
        //                                   .type = CLAY__SIZING_TYPE_FIXED,
        //                               }};
        Clay_Sizing dropdownSizing = {.width = CLAY_SIZING_FIXED(CHAR_W * (maxLength + 6))};

        // CLAY({
        //     .layout = {.sizing = dropdownSizing},
        //     .backgroundColor = COLOR_BG_DROPDOWN,
        //     .cornerRadius = buttonCornerRadius,
        //     // .border = {
        //     //     .color = COLOR_BORDER_DROPDOWN,
        //     //     .width = buttonBorderWidth,
        //     // },
        // })
        // {
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
            CLAY_TEXT(str, TEXT_CONFIG_DEFAULT);

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

void RenderScrollBar(ScrollID scrollId)
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
        CLAY({
            .layout = {
                .sizing = {
                    .width = 16,
                    .height = (scrollData.data[scrollId].scrollContainerDimensions.height / scrollData.data[scrollId].contentDimensions.height) * scrollData.data[scrollId].scrollContainerDimensions.height,
                },
            },
            .backgroundColor = scrollData.scrolling == scrollId || Clay_Hovered() ? COLOR_BG_THUMB_HOVERED : COLOR_BG_THUMB,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
            .floating = {
                .offset = (Clay_Vector2){0, -scrollData.data[scrollId].scrollPosition->y * scrollData.data[scrollId].scrollContainerDimensions.height / scrollData.data[scrollId].contentDimensions.height},
                .attachTo = CLAY_ATTACH_TO_PARENT,
            },
        })
        {
            Clay_OnHover(HandleThumbInteraction, scrollId);
        }

        if (scrollData.scrolling == scrollId)
        {
            CLAY({
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_GROW(0),
                    },
                },
                // .backgroundColor = (Clay_Color){255, 255, 255, 30},
                .floating = {
                    .attachTo = CLAY_ATTACH_TO_ROOT,
                },
            })
            {
                Clay_OnHover(HandleThumbInteraction, scrollId);
            }
        }
    }
}

void LayoutCreator_Initialize(Font defaultFont)
{
    for (size_t i = 0; i < TAB_ID_DUMMY_LAST; i++)
    {
        tabData.isDisabled[i] = true;
    }

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
    bool scrollingWindow = scrollData.data[SCROLL_ID_WINDOW].found && scrollData.data[SCROLL_ID_WINDOW].scrollContainerDimensions.height != scrollData.data[SCROLL_ID_WINDOW].contentDimensions.height;
    // bool scrollingTabbedBoxContent = scrollData.data[SCROLL_ID_TABBEDBOXCONTENT].found && scrollData.data[SCROLL_ID_TABBEDBOXCONTENT].scrollContainerDimensions.height != scrollData.data[SCROLL_ID_TABBEDBOXCONTENT].contentDimensions.height;

    CLAY({
        .id = CLAY_ID("WindowContainer"),
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_GROW(0),
            },
            .padding = scrollingWindow ? (Clay_Padding){16, 32, 16, 16} : CLAY_PADDING_ALL(16),
        },
        .backgroundColor = COLOR_BG_MAIN,
        .clip = {
            .vertical = true,
            .childOffset = Clay_GetScrollOffset(),
        },
    })
    {
        if (scrollingWindow)
        {
            RenderScrollBar(SCROLL_ID_WINDOW);
        }

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
                .childGap = CHAR_H,
            },
            .backgroundColor = COLOR_BG_SECTION,
            .cornerRadius = CLAY_CORNER_RADIUS(16),
        })
        {
            CLAY({
                .layout = {
                    .childGap = CHAR_W * 2,
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

            CLAY({.layout = {.childGap = CHAR_W}})
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
                        .padding = CLAY_PADDING_ALL(16),
                        .childGap = 16,
                    },
                    .backgroundColor = COLOR_BG_TAB_SELECTED,
                    .cornerRadius = (Clay_CornerRadius){0, 0, 16, 16},
                    .border = {
                        .color = COLOR_BORDER_TAB,
                        .width = (Clay_BorderWidth){1, 1, 0, 1, 0},
                    },
                    // .clip = {
                    //     .vertical = true,
                    //     .horizontal = true,
                    //     .childOffset = Clay_GetScrollOffset(),
                    // },
                })
                {
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
                                                4,
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
                                                TEXTBOX_ID_SCALE_H,
                                                (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 0, .max = FLOAT_MAX},
                                                false,
                                                4,
                                                "in_h");
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
                                                4,
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
                                                4,
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
                                                4,
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
                                                4,
                                                "0");
                                        }
                                    }
                                }
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
                                            RenderTextbox(
                                                CLAY_STRING(""),
                                                TEXTBOX_ID_SPEED_VIDEO,
                                                (NumberboxConfig){.isNumberbox = true, .min = 0.01, .max = 100},
                                                false,
                                                4,
                                                "1.0");
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
                                            RenderTextbox(
                                                CLAY_STRING(""),
                                                TEXTBOX_ID_SPEED_AUDIO,
                                                (NumberboxConfig){.isNumberbox = true, .min = 0.01, .max = 100},
                                                false,
                                                4,
                                                "1.0");
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
                    .childGap = CHAR_W * 2,
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
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
    {
        HandleThumbInteraction(CLAY_ID(""), (Clay_PointerData){0}, SCROLL_ID_DUMMY_LAST);
    }
    else if (IsMouseButtonUp)
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