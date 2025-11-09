#include "../inc/clay.h"
#include "../inc/raylib.h"
#include "../inc/nfd.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#define POPEN_READ_ONLY_STDERR "2>&1 1>nul"
#else
#define POPEN_READ_ONLY_STDERR "2>&1 1>/dev/null"
#endif

#define TEXTBOX_BUFFER_SIZE 256
#define FLOAT_MAX 1e5f

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

#define DROPDOWN_OPTION_NULL {CLAY_STRING(""), NULL}
#define DROPDOWN_OPTION_UNSELECTED {CLAY_STRING("-- None --"), ""}

#define LOG(format, ...) printf("\x1b[33mLOG: " format "\x1b[0m\n", ##__VA_ARGS__)
#define ERROR(format, ...) fprintf(stderr, "\x1b[31mERROR: " format "\x1b[0m\n", ##__VA_ARGS__)

#define CLAMP(val, min, max) (val < min) ? min : (val > max) ? max \
                                                             : val

typedef enum
{
    FONT_ID_BODY_16,
    FONT_ID_BOLD_16,
    FONT_ID_DUMMY_LAST
} FontID;

typedef enum
{
    TEXTBOX_ID_INPUT_PATH,
    TEXTBOX_ID_FPS,
    TEXTBOX_ID_DURATION_START,
    TEXTBOX_ID_DURATION_END,
    TEXTBOX_ID_SPEED,
    TEXTBOX_ID_OUTPUT_FOLDER,
    TEXTBOX_ID_OUTPUT_NAME,
    TEXTBOX_ID_SCALE_W,
    TEXTBOX_ID_SCALE_H,
    TEXTBOX_ID_CROP_W,
    TEXTBOX_ID_CROP_H,
    TEXTBOX_ID_CROP_X,
    TEXTBOX_ID_CROP_Y,
    TEXTBOX_ID_DUMMY_LAST
} TextboxID;

typedef enum
{
    DROPDOWN_ID_TEST,
    DROPDOWN_ID_DUMMY_LAST
} DropdownID;

typedef enum
{
    BUTTON_ID_CONVERT,
    BUTTON_ID_BROWSE_INPUT,
    BUTTON_ID_BROWSE_OUTPUT,
    BUTTON_ID_DUMMY_LAST
} ButtonID;

const Clay_Color COLOR_BG_MAIN = {50, 50, 50, 255};
const Clay_Color COLOR_BG_SECTION = {70, 70, 70, 255};
const Clay_Color COLOR_BG_TEXTBOX = {90, 90, 90, 255};
const Clay_Color COLOR_BG_TEXTBOX_DISABLED = {70, 70, 70, 255};
const Clay_Color COLOR_BG_BUTTON = {90, 90, 90, 255};
const Clay_Color COLOR_BG_BUTTON_HOVERED = {110, 110, 110, 255};
const Clay_Color COLOR_BG_BUTTON_DISABLED = {70, 70, 70, 255};
const Clay_Color COLOR_BG_DROPDOWN = {90, 90, 90, 255};
const Clay_Color COLOR_BG_DROPDOWN_HOVERED = {110, 110, 110, 255};

const Clay_Color COLOR_BORDER_TEXTBOX = {150, 150, 150, 255};
const Clay_Color COLOR_BORDER_TEXTBOX_FOCUSED = {200, 200, 200, 255};
const Clay_Color COLOR_BORDER_BUTTON = {150, 150, 150, 255};
const Clay_Color COLOR_BORDER_DROPDOWN = {150, 150, 150, 255};

const Clay_Color COLOR_TRANSPARENT = {0, 0, 0, 0};
const Clay_Color COLOR_WHITE = {255, 255, 255, 255};
const Clay_Color COLOR_BLACK = {0, 0, 0, 255};
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
    // bool isInit;
    // bool isDisabled;
} TextboxBuffer;

typedef struct
{
    TextboxBuffer *textboxBuffers;
    bool *isInit;
    bool *isDisabled;
    int hoveredTextbox;
    FocusData focusData;
    Vector2 minDimensions;
} TextboxData;

typedef struct
{
    Clay_String label;
    const char *value;
} DropdownOption;

typedef struct
{
    size_t *selectedOptions;
    const char **selectedValues;
    size_t hoveredOption;
    const char *hoveredValue;
} DropdownData;

typedef struct
{
    bool *isDisabled;
} ButtonData;

TextboxBuffer textboxBuffers[TEXTBOX_ID_DUMMY_LAST] = {0};
bool textboxIsInit[TEXTBOX_ID_DUMMY_LAST] = {0};
bool textboxIsDisabled[TEXTBOX_ID_DUMMY_LAST] = {0};

TextboxData textboxData = {
    .textboxBuffers = textboxBuffers,
    .isInit = textboxIsInit,
    .isDisabled = textboxIsDisabled,
    .hoveredTextbox = -1,
    .focusData = {
        .focusRegistered = false,
        .focusIndex = -1,
    },
};

size_t dropdownSelectedOptions[DROPDOWN_ID_DUMMY_LAST] = {0};
const char *dropdownSelectedValues[DROPDOWN_ID_DUMMY_LAST] = {0};

DropdownData dropdownData = {
    .selectedOptions = dropdownSelectedOptions,
    .selectedValues = dropdownSelectedValues,
    .hoveredOption = 0,
    .hoveredValue = NULL,
};

bool buttonIsDisabled[BUTTON_ID_DUMMY_LAST] = {0};

ButtonData buttonData = {
    .isDisabled = buttonIsDisabled,
};

Clay_Padding defaultBoxPadding;
Clay_CornerRadius defaultCornerRadius;

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

int convert()
{
    char cmd[2048] = {0};
    char *p = cmd;
    const char *str;

    // Input path
    p += snprintf(p, sizeof(cmd), "ffmpeg -i \"");
    if ((str = getTextboxValue(TEXTBOX_ID_INPUT_PATH))[0])
    {
        p += snprintf(p, sizeof(cmd), str);
    }
    else
    {
        LOG("no input");
        return 1;
    }

    p += snprintf(p, sizeof(cmd), "\" -vf \"");

    // FPS
    p += snprintf(p, sizeof(cmd), "fps=");
    p += snprintf(p, sizeof(cmd), getTextboxValue(TEXTBOX_ID_FPS));

    // Duration
    p += snprintf(p, sizeof(cmd), ",trim=");
    p += snprintf(p, sizeof(cmd), getTextboxValue(TEXTBOX_ID_DURATION_START));
    p += snprintf(p, sizeof(cmd), ":");
    p += snprintf(p, sizeof(cmd), getTextboxValue(TEXTBOX_ID_DURATION_END));

    // Speed
    p += snprintf(p, sizeof(cmd), ",setpts=(PTS-STARTPTS)/");
    p += snprintf(p, sizeof(cmd), getTextboxValue(TEXTBOX_ID_SPEED));

    // Scale
    p += snprintf(p, sizeof(cmd), ",scale=");
    p += snprintf(p, sizeof(cmd), getTextboxValue(TEXTBOX_ID_SCALE_W));
    p += snprintf(p, sizeof(cmd), ":");
    p += snprintf(p, sizeof(cmd), getTextboxValue(TEXTBOX_ID_SCALE_H));

    // Crop
    p += snprintf(p, sizeof(cmd), ",crop=");
    p += snprintf(p, sizeof(cmd), getTextboxValue(TEXTBOX_ID_CROP_W));
    p += snprintf(p, sizeof(cmd), ":");
    p += snprintf(p, sizeof(cmd), getTextboxValue(TEXTBOX_ID_CROP_H));
    p += snprintf(p, sizeof(cmd), ":");
    p += snprintf(p, sizeof(cmd), getTextboxValue(TEXTBOX_ID_CROP_X));
    p += snprintf(p, sizeof(cmd), ":");
    p += snprintf(p, sizeof(cmd), getTextboxValue(TEXTBOX_ID_CROP_Y));

    // Output path
    p += snprintf(p, sizeof(cmd), "\" \"");
    if ((str = getTextboxValue(TEXTBOX_ID_OUTPUT_FOLDER))[0])
    {
        p += snprintf(p, sizeof(cmd), str);
    }
    else
    {
        LOG("no output");
        return 1;
    }

    if (strchr(str, '\\') != NULL)
    {
        p += snprintf(p, sizeof(cmd), "\\");
    }
    else
    {
        p += snprintf(p, sizeof(cmd), "/");
    }
    p += snprintf(p, sizeof(cmd), getTextboxValue(TEXTBOX_ID_OUTPUT_NAME));
    p += snprintf(p, sizeof(cmd), "\" " POPEN_READ_ONLY_STDERR);

    if (p - cmd >= sizeof(cmd))
    {
        ERROR("cmd overflow");
    }

    LOG("cmd = \"%s\"", cmd);

    return -1;
}

void HandleTextboxInteraction(Clay_ElementId elementId,
                              Clay_PointerData pointerData,
                              intptr_t userData)
{
    size_t index = (size_t)userData;
    if (!textboxData.isDisabled[index] && pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        textboxData.textboxBuffers[index].cursorPosition = textboxData.textboxBuffers[index].length;

        textboxData.focusData.focusRegistered = true;
        textboxData.focusData.focusIndex = index;
        textboxData.focusData.focusStartTime = GetTime();
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
            ERROR("result = %d", result);
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
            nfdu8filteritem_t filters[] = {
                {"Videos", "mp4,mov,mkv,webm,flv,mpeg"},
                {"GIFs", "gif"},
            };
            nfdopendialogu8args_t openDialogArgs = {0};
            openDialogArgs.filterList = filters;
            openDialogArgs.filterCount = 2;
            result = NFD_OpenDialogU8_With(&outPath, &openDialogArgs);
            break;

        case TEXTBOX_ID_OUTPUT_FOLDER:
            nfdpickfolderu8args_t pickFolderArgs = {0};
            result = NFD_PickFolderU8_With(&outPath, &pickFolderArgs);
            break;

        default:
            ERROR("invalid index = %zu (HandleBrowseButtonInteraction)", index);
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
            // LOG("Browse canceled (index = %zu)", index);
        }
        else
        {
            LOG("Error: %s", NFD_GetError());
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

void RenderTextbox(
    Clay_String label,
    TextboxID textboxId,
    NumberboxConfig numberboxConfig,
    bool isDisabled,
    size_t maxCharsDisplayed,
    Clay_String defaultValue)
{
    if (!textboxData.isInit[textboxId])
    {
        textboxData.textboxBuffers[textboxId].charsDefault = defaultValue.chars;
        textboxData.textboxBuffers[textboxId].numberboxConfig = numberboxConfig;
        textboxData.isDisabled[textboxId] = isDisabled;
        textboxData.isInit[textboxId] = true;
    }

    bool focused = (textboxData.focusData.focusIndex == textboxId);

    CLAY({
        .id = CLAY_IDI("Textbox", textboxId),
        .layout = {
            .childGap = textboxData.minDimensions.x,
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
                                .min = textboxData.minDimensions.x * (maxCharsDisplayed + 2),
                            },
                        },
                    },
                },
                .padding = defaultBoxPadding,
            },
            .backgroundColor = textboxData.isDisabled[textboxId] ? COLOR_BG_TEXTBOX_DISABLED : COLOR_BG_TEXTBOX,
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
                    if (defaultValue.length > 0)
                    {
                        CLAY_TEXT(defaultValue, TEXT_CONFIG_FAINT);
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

                    if (textboxData.isDisabled[textboxId])
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

void RenderButton(
    Clay_String label,
    ButtonID buttonId,
    void (*onHoverFunction)(
        Clay_ElementId elementId,
        Clay_PointerData pointerData,
        intptr_t userData),
    intptr_t userData)
{
    CLAY({
        .layout = {
            .padding = defaultBoxPadding,
        },
        .backgroundColor = buttonData.isDisabled[buttonId] ? COLOR_BG_BUTTON_DISABLED : Clay_Hovered() ? COLOR_BG_BUTTON_HOVERED
                                                                                                       : COLOR_BG_BUTTON,
        .cornerRadius = CLAY_CORNER_RADIUS(8),
        .border = {
            .color = COLOR_BORDER_BUTTON,
            .width = CLAY_BORDER_OUTSIDE(1),
        },
    })
    {
        Clay_OnHover(onHoverFunction, userData);

        CLAY_TEXT(label, TEXT_CONFIG_BOLD);
    }
}

void RenderDropdown(Clay_String label, DropdownID dropdownId, DropdownOption *options)
{
    bool dropdownHovered = Clay_PointerOver(CLAY_IDI("DropdownButton", dropdownId)) ||
                           Clay_PointerOver(CLAY_IDI("DropdownOptions", dropdownId));

    size_t maxLength = 0;
    for (size_t i = 0; options[i].value != NULL; i++)
    {
        if (options[i].label.length > maxLength)
        {
            maxLength = options[i].label.length;
        }
    }

    CLAY({
        .id = CLAY_IDI("Dropdown", dropdownId),
        .layout = {
            .childGap = textboxData.minDimensions.x,
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
        if (dropdownHovered)
        {
            buttonCornerRadius = (Clay_CornerRadius){8, 8, 0, 0};
            buttonBorderWidth = (Clay_BorderWidth){1, 1, 1, 0, 0};
        }
        else
        {
            buttonCornerRadius = CLAY_CORNER_RADIUS(8);
            buttonBorderWidth = (Clay_BorderWidth)CLAY_BORDER_OUTSIDE(1);
        }

        CLAY({
            .layout = {
                .sizing = {
                    .width = {
                        .size = {
                            .minMax = {
                                .min = textboxData.minDimensions.x * (maxLength + 2),
                            },
                        },
                        .type = CLAY__SIZING_TYPE_FIXED,
                    },
                },
            },
            .backgroundColor = COLOR_BG_DROPDOWN,
            .cornerRadius = buttonCornerRadius,
            .border = {
                .color = COLOR_BORDER_DROPDOWN,
                .width = buttonBorderWidth,
            },
        })
        {
            CLAY({
                .id = CLAY_IDI("DropdownButton", dropdownId),
                .layout = {
                    .sizing = CLAY_SIZING_GROW(0),
                    .padding = defaultBoxPadding,
                    .childAlignment = {.x = CLAY_ALIGN_X_CENTER},
                },
            })
            {
                CLAY_TEXT(options[dropdownData.selectedOptions[dropdownId]].label, TEXT_CONFIG_DEFAULT);
            }

            if (dropdownHovered)
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
                                    .sizing = {
                                        .width = {
                                            .size = {
                                                .minMax = {
                                                    .min = textboxData.minDimensions.x * (maxLength + 2),
                                                },
                                            },
                                            .type = CLAY__SIZING_TYPE_FIXED,
                                        },
                                    },
                                    .padding = defaultBoxPadding,
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

                                CLAY_TEXT(options[i].label, TEXT_CONFIG_DEFAULT);
                            }
                        }
                    }
                }
            }
        }
    }
}

void LayoutCreator_Initialize(Font defaultFont)
{
    textboxData.minDimensions = MeasureTextEx(defaultFont, "1", 16, 0);

    defaultBoxPadding = (Clay_Padding){
        textboxData.minDimensions.x,
        textboxData.minDimensions.x,
        textboxData.minDimensions.y / 4,
        textboxData.minDimensions.y / 4,
    };

    defaultCornerRadius = CLAY_CORNER_RADIUS(textboxData.minDimensions.x / 2);

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

Clay_RenderCommandArray LayoutCreator_CreateLayout()
{
    Clay_BeginLayout();

    textboxData.hoveredTextbox = -1;
    textboxData.focusData.focusRegistered = false;

    CLAY({
        .id = CLAY_ID("WindowContainer"),
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_GROW(0),
            },
            .padding = CLAY_PADDING_ALL(16),
            .childGap = 16,
        },
        .backgroundColor = COLOR_BG_MAIN,
    })
    {
        CLAY({
            .id = CLAY_ID("LeftSectionContainer"),
            .layout = {
                .padding = CLAY_PADDING_ALL(16),
                .childGap = textboxData.minDimensions.y,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = COLOR_BG_SECTION,
            .cornerRadius = CLAY_CORNER_RADIUS(16),
            // .clip = {
            //     .vertical = true,
            //     .childOffset = Clay_GetScrollOffset(),
            // },
        })
        {
            CLAY({.layout = {.childGap = 4}})
            {
                RenderTextbox(
                    CLAY_STRING("Input File:"),
                    TEXTBOX_ID_INPUT_PATH,
                    (NumberboxConfig){0},
                    false,
                    30,
                    CLAY_STRING(""));

                RenderButton(
                    CLAY_STRING("..."),
                    BUTTON_ID_BROWSE_INPUT,
                    HandleBrowseButtonInteraction,
                    TEXTBOX_ID_INPUT_PATH);
            }

            RenderTextbox(
                CLAY_STRING("FPS:"),
                TEXTBOX_ID_FPS,
                (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 1, .max = 60},
                false,
                2,
                CLAY_STRING("30"));

            CLAY({
                .layout = {
                    .childGap = textboxData.minDimensions.x,
                },
            })
            {
                RenderTextbox(
                    CLAY_STRING("Duration:"),
                    TEXTBOX_ID_DURATION_START,
                    (NumberboxConfig){.isNumberbox = true, .min = 0, .max = FLOAT_MAX},
                    false,
                    6,
                    CLAY_STRING(""));

                RenderTextbox(
                    CLAY_STRING("to"),
                    TEXTBOX_ID_DURATION_END,
                    (NumberboxConfig){.isNumberbox = true, .min = 0, .max = FLOAT_MAX},
                    false,
                    6,
                    CLAY_STRING(""));
            }

            RenderTextbox(
                CLAY_STRING("Speed:"),
                TEXTBOX_ID_SPEED,
                (NumberboxConfig){.isNumberbox = true, .min = 0.01, .max = 100},
                false,
                4,
                CLAY_STRING("1.0"));

            CLAY({.layout = {.childGap = 4}})
            {
                RenderTextbox(
                    CLAY_STRING("Output Folder:"),
                    TEXTBOX_ID_OUTPUT_FOLDER,
                    (NumberboxConfig){0},
                    false,
                    27,
                    CLAY_STRING(""));

                RenderButton(
                    CLAY_STRING("..."),
                    BUTTON_ID_BROWSE_OUTPUT,
                    HandleBrowseButtonInteraction,
                    TEXTBOX_ID_OUTPUT_FOLDER);
            }

            RenderTextbox(
                CLAY_STRING("File Name:"),
                TEXTBOX_ID_OUTPUT_NAME,
                (NumberboxConfig){0},
                false,
                25,
                CLAY_STRING("converted-file"));

            RenderButton(
                CLAY_STRING(" Convert "),
                BUTTON_ID_CONVERT,
                HandleConvertButtonInteraction,
                0);
        }

        CLAY({
            .id = CLAY_ID("RightSectionContainer"),
            .layout = {
                .padding = CLAY_PADDING_ALL(16),
                .childGap = textboxData.minDimensions.y,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = COLOR_BG_SECTION,
            .cornerRadius = CLAY_CORNER_RADIUS(16),
            // .clip = {
            //     .vertical = true,
            //     .horizontal = true,
            //     .childOffset = Clay_GetScrollOffset(),
            // },
        })
        {
            // CLAY_TEXT(CLAY_STRING("Preview"), TEXT_CONFIG_BOLD);

            CLAY({
                .layout = {
                    .childGap = 12,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .border = {
                    .color = COLOR_GRAY,
                    .width = (Clay_BorderWidth){0, 0, 0, 0, 1},
                },
            })
            {
                CLAY_TEXT(CLAY_STRING("Scale"), TEXT_CONFIG_BOLD);

                CLAY(0)
                {
                    RenderTextbox(
                        CLAY_STRING("    Width:"),
                        TEXTBOX_ID_SCALE_W,
                        (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 0, .max = FLOAT_MAX},
                        false,
                        4,
                        CLAY_STRING("in_w"));

                    RenderTextbox(
                        CLAY_STRING("    Height:"),
                        TEXTBOX_ID_SCALE_H,
                        (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 0, .max = FLOAT_MAX},
                        false,
                        4,
                        CLAY_STRING("in_h"));
                }
            }

            CLAY({
                .layout = {
                    .childGap = 12,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .border = {
                    .color = COLOR_GRAY,
                    .width = (Clay_BorderWidth){0, 0, 0, 0, 1},
                },
            })
            {
                CLAY_TEXT(CLAY_STRING("Crop"), TEXT_CONFIG_BOLD);

                CLAY({
                    .layout = {
                        .childGap = 12,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                })
                {
                    CLAY(0)
                    {
                        RenderTextbox(
                            CLAY_STRING("    Width:"),
                            TEXTBOX_ID_CROP_W,
                            (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 0, .max = FLOAT_MAX},
                            false,
                            4,
                            CLAY_STRING("in_w"));

                        RenderTextbox(
                            CLAY_STRING("    Height:"),
                            TEXTBOX_ID_CROP_H,
                            (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 0, .max = FLOAT_MAX},
                            false,
                            4,
                            CLAY_STRING("in_h"));
                    }

                    CLAY(0)
                    {
                        RenderTextbox(
                            CLAY_STRING(" x-offset:"),
                            TEXTBOX_ID_CROP_X,
                            (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 0, .max = FLOAT_MAX},
                            false,
                            4,
                            CLAY_STRING("0"));

                        RenderTextbox(
                            CLAY_STRING("  y-offset:"),
                            TEXTBOX_ID_CROP_Y,
                            (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 0, .max = FLOAT_MAX},
                            false,
                            4,
                            CLAY_STRING("0"));
                    }
                }
            }
        }
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
        if ((IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE)) && nonMaxCursorPosition)
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
        if ((IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) && nonZeroCursorPosition)
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
        if ((IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) && nonMaxCursorPosition)
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
        if (IsKeyPressed(KEY_TAB) || IsKeyPressedRepeat(KEY_TAB))
        {
            if (isShiftDown)
            {
                do
                {
                    textboxData.focusData.focusIndex = (textboxData.focusData.focusIndex + TEXTBOX_ID_DUMMY_LAST - 1) % TEXTBOX_ID_DUMMY_LAST;
                } while (textboxData.isDisabled[textboxData.focusData.focusIndex]);
            }
            else
            {
                do
                {
                    textboxData.focusData.focusIndex = (textboxData.focusData.focusIndex + 1) % TEXTBOX_ID_DUMMY_LAST;
                } while (textboxData.isDisabled[textboxData.focusData.focusIndex]);
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
            do
            {
                textboxData.focusData.focusIndex = (textboxData.focusData.focusIndex + 1) % TEXTBOX_ID_DUMMY_LAST;
            } while (textboxData.isDisabled[textboxData.focusData.focusIndex]);
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

    SetMouseCursor(textboxData.hoveredTextbox >= 0 && !textboxData.isDisabled[textboxData.focusData.focusIndex] ? MOUSE_CURSOR_IBEAM : MOUSE_CURSOR_DEFAULT);

    return Clay_EndLayout();
}