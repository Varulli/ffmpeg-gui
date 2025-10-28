#include "../inc/clay.h"
#include "../inc/raylib.h"
#include "../inc/nfd.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <float.h>

#define DROPDOWN_OPTION_NULL {CLAY_STRING(""), NULL}
#define DROPDOWN_OPTION_UNSELECTED {CLAY_STRING("-- None --"), ""}

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

// #define NUM_TEST 5
#define TEXTBOX_CHARS_MAX 255

#define CLAMP(val, min, max) (val < min) ? min : (val > max) ? max \
                                                             : val

typedef enum
{
    FONT_ID_BODY_16,
    FONT_ID_TEST_16,
    FONT_ID_DUMMY_LAST
} FontID;

typedef enum
{
    TEXTBOX_ID_INPUT_PATH,
    TEXTBOX_ID_FILTER_FPS,
    TEXTBOX_ID_DURATION_START,
    TEXTBOX_ID_DURATION_END,
    TEXTBOX_ID_SPEED,
    TEXTBOX_ID_OUTPUT_PATH,
    TEXTBOX_ID_DUMMY_LAST
} TextboxID;

typedef enum
{
    DROPDOWN_ID_TEST,
    DROPDOWN_ID_DUMMY_LAST
} DropdownID;

const Clay_Color COLOR_BG_MAIN = {50, 50, 50, 255};
const Clay_Color COLOR_BG_SECTION = {70, 70, 70, 255};
const Clay_Color COLOR_BG_TEXTBOX = {90, 90, 90, 255};
const Clay_Color COLOR_BG_TEXTBOX_DISABLED = {70, 70, 70, 255};
const Clay_Color COLOR_BG_BUTTON = {90, 90, 90, 255};
const Clay_Color COLOR_BG_DROPDOWN = {90, 90, 90, 255};
const Clay_Color COLOR_BG_BUTTON_HOVERED = {110, 110, 110, 255};
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
    char chars[TEXTBOX_CHARS_MAX + 1];
    const char *charsDefault;
    size_t length;
    size_t cursorPosition;
    NumberboxConfig numberboxConfig;
    bool isInit;
} TextboxBuffer;

typedef struct
{
    // bool *disabled;
    TextboxBuffer *textboxBuffers;
    bool hoveringTextbox;
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

// bool disabled[TEXTBOX_ID_DUMMY_LAST] = {0};
TextboxBuffer textboxBuffers[TEXTBOX_ID_DUMMY_LAST] = {0};

TextboxData textboxData = {
    // .disabled = disabled
    .textboxBuffers = textboxBuffers,
    .hoveringTextbox = false,
    .focusData = {
        .focusRegistered = false,
        .focusIndex = -1,
    },
};

size_t selectedOptions[DROPDOWN_ID_DUMMY_LAST] = {0};
const char *selectedValues[DROPDOWN_ID_DUMMY_LAST] = {0};

DropdownData dropdownData = {
    .selectedOptions = selectedOptions,
    .selectedValues = selectedValues,
    .hoveredOption = 0,
    .hoveredValue = NULL,
};

Clay_Padding defaultBoxPadding;
Clay_CornerRadius defaultCornerRadius;

const char *getTextboxValue(TextboxID textboxId)
{
    // printf("DEBUG: chars = %s\n", textboxData.textboxBuffers[textboxId].chars);
    // printf("DEBUG: charsDefault = %s\n", textboxData.textboxBuffers[textboxId].charsDefault);
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
    char cmd[1024] = {0};
    const char *str;
    strcat(cmd, "ffmpeg -i \"");
    if ((str = getTextboxValue(TEXTBOX_ID_INPUT_PATH)))
    {
        strcat(cmd, str);
    }
    strcat(cmd, "\" -vf \"");
    // strcat(cmd, textboxData.textboxBuffers[TEXTBOX_ID_FILTERS].chars);
    strcat(cmd, "\" \"");
    if ((str = getTextboxValue(TEXTBOX_ID_OUTPUT_PATH)))
    {
        strcat(cmd, str);
    }
    strcat(cmd, "\"");
    printf("DEBUG: cmd = %s\n", cmd);
    printf("DEBUG: dropdown (test) = %s\n", getDropdownValue(DROPDOWN_ID_TEST));
    return system(cmd);
}

void HandleTextboxInteraction(Clay_ElementId elementId,
                              Clay_PointerData pointerData,
                              intptr_t userData)
{
    if (pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        size_t index = (size_t)userData;

        textboxData.textboxBuffers[index].cursorPosition = textboxData.textboxBuffers[index].length;

        textboxData.focusData.focusRegistered = true;
        textboxData.focusData.focusIndex = index;
        textboxData.focusData.focusStartTime = GetTime();
    }
}

void HandleConvertButtonInteraction(Clay_ElementId elementId,
                                    Clay_PointerData pointerData,
                                    intptr_t userData)
{
    if (pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        int result = convert();

        if (result)
        {
            printf("DEBUG: result = %d\n", result);
        }
    }
}

void HandleBrowseButtonInteraction(Clay_ElementId elementId,
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

        case TEXTBOX_ID_OUTPUT_PATH:
            nfdpickfolderu8args_t pickFolderArgs = {0};
            result = NFD_PickFolderU8_With(&outPath, &pickFolderArgs);
            break;

        default:
            printf("DEBUG: invalid index = %zu (HandleBrowseButtonInteraction)\n", index);
            return;
            break;
        }

        if (result == NFD_OKAY)
        {
            TextboxBuffer *buffer = &textboxData.textboxBuffers[index];
            strncpy(buffer->chars, outPath, TEXTBOX_CHARS_MAX);
            buffer->length = strlen(buffer->chars);
            buffer->cursorPosition = buffer->length;
            NFD_FreePathU8(outPath);
        }
        else if (result == NFD_CANCEL)
        {
            // printf("DEBUG: Browse canceled (index = %zu)\n", index);
        }
        else
        {
            printf("DEBUG: Error: %s\n", NFD_GetError());
        }
    }
}

void HandleDropdownOptionInteraction(Clay_ElementId elementId,
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

void RenderTextbox(Clay_String label,
                   TextboxID textboxId,
                   NumberboxConfig numberboxConfig,
                   size_t maxCharsDisplayed,
                   Clay_String defaultValue)
{
    if (!textboxData.textboxBuffers[textboxId].isInit)
    {
        textboxData.textboxBuffers[textboxId].charsDefault = defaultValue.chars;
        textboxData.textboxBuffers[textboxId].numberboxConfig = numberboxConfig;
        textboxData.textboxBuffers[textboxId].isInit = true;
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
            CLAY_TEXT(label, TEXT_CONFIG_DEFAULT);
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
            // .backgroundColor = textboxData.disabled[textboxId] ? COLOR_BG_TEXTBOX_DISABLED : COLOR_BG_TEXTBOX,
            .backgroundColor = COLOR_BG_TEXTBOX,
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
                textboxData.hoveringTextbox = true;
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

                    CLAY_TEXT(textBeforeCursor, TEXT_CONFIG_DEFAULT);
                    CLAY_TEXT(textAfterCursor, TEXT_CONFIG_DEFAULT);
                }
            }
        }
    }
}

void RenderBrowseButton(TextboxID textboxId)
{
    CLAY({
        .layout = {
            .padding = defaultBoxPadding,
        },
        .backgroundColor = Clay_Hovered() ? COLOR_BG_BUTTON_HOVERED : COLOR_BG_BUTTON,
        .cornerRadius = CLAY_CORNER_RADIUS(8),
        .border = {
            .color = COLOR_BORDER_BUTTON,
            .width = CLAY_BORDER_OUTSIDE(1),
        },
    })
    {
        Clay_OnHover(HandleBrowseButtonInteraction, textboxId);

        CLAY_TEXT(CLAY_STRING("..."), TEXT_CONFIG_DEFAULT);
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
            CLAY_TEXT(label, TEXT_CONFIG_DEFAULT);
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

Clay_RenderCommandArray LayoutCreator_CreateLayout()
{
    Clay_BeginLayout();

    bool leftClickPressed = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    textboxData.hoveringTextbox = false;
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
            .clip = {
                .vertical = true,
                .childOffset = Clay_GetScrollOffset(),
            },
        })
        {
            CLAY({.layout = {.childGap = 4}})
            {
                RenderTextbox(CLAY_STRING("Input File:"),
                              TEXTBOX_ID_INPUT_PATH,
                              (NumberboxConfig){0},
                              30,
                              CLAY_STRING("default"));

                RenderBrowseButton(TEXTBOX_ID_INPUT_PATH);
            }

            RenderTextbox(CLAY_STRING("FPS:"),
                          TEXTBOX_ID_FILTER_FPS,
                          (NumberboxConfig){.isNumberbox = true, .isInt = true, .min = 1, .max = 60},
                          2,
                          CLAY_STRING("30"));

            CLAY({
                .layout = {
                    .childGap = textboxData.minDimensions.x,
                },
            })
            {
                RenderTextbox(CLAY_STRING("Duration:"),
                              TEXTBOX_ID_DURATION_START,
                              (NumberboxConfig){.isNumberbox = true, .min = 0, .max = FLT_MAX},
                              12,
                              CLAY_STRING("0.0"));

                RenderTextbox(CLAY_STRING("to"),
                              TEXTBOX_ID_DURATION_END,
                              (NumberboxConfig){.isNumberbox = true, .min = 0, .max = FLT_MAX},
                              2,
                              CLAY_STRING("0.0"));
            }

            RenderTextbox(CLAY_STRING("Speed:"),
                          TEXTBOX_ID_SPEED,
                          (NumberboxConfig){.isNumberbox = true, .min = 0.01, .max = 100},
                          4,
                          CLAY_STRING("1.0"));

            RenderDropdown(CLAY_STRING("Filters:"),
                           DROPDOWN_ID_TEST,
                           (DropdownOption[]){
                               DROPDOWN_OPTION_UNSELECTED,
                               {CLAY_STRING("Option 1"), "Option 1"},
                               {CLAY_STRING("Option 2 blah"), "Option 2"},
                               {CLAY_STRING("Option 3 yo"), "Option 3"},
                               DROPDOWN_OPTION_NULL,
                           });

            CLAY({.layout = {.childGap = 4}})
            {
                RenderTextbox(CLAY_STRING("Output Folder:"),
                              TEXTBOX_ID_OUTPUT_PATH,
                              (NumberboxConfig){0},
                              27,
                              CLAY_STRING(""));

                RenderBrowseButton(TEXTBOX_ID_OUTPUT_PATH);
            }

            CLAY(0)
            {
                Clay_OnHover(HandleConvertButtonInteraction, 0);

                CLAY_TEXT(CLAY_STRING("Convert"), TEXT_CONFIG_DEFAULT);
            }
        }

        // CLAY({
        //     .id = CLAY_ID("RightSectionContainer"),
        //     .layout = {
        //         .padding = CLAY_PADDING_ALL(16),
        //         .childGap = 16,
        //         .layoutDirection = CLAY_TOP_TO_BOTTOM,
        //     },
        //     .backgroundColor = COLOR_BG_SECTION,
        //     .cornerRadius = CLAY_CORNER_RADIUS(16),
        //     .clip = {
        //         .vertical = true,
        //         .horizontal = true,
        //         .childOffset = Clay_GetScrollOffset(),
        //     },
        // })
        // {
        //     CLAY_TEXT(CLAY_STRING("Preview"), defaultTextConfig);

        //     for (size_t i = 0; i < NUM_TEST; i++)
        //     {
        //         CLAY({
        //             .layout = {
        //                 .padding = CLAY_PADDING_ALL(16),
        //             },
        //             .backgroundColor = (i == textboxData.focusData.focusIndex) ? COLOR_GREEN : COLOR_RED,
        //             .cornerRadius = CLAY_CORNER_RADIUS(8),
        //         })
        //         {
        //             Clay_OnHover(HandleTextboxInteraction, i);
        //             if (Clay_Hovered())
        //             {
        //                 textboxData.hoveringTextbox = true;
        //             }
        //             CLAY_TEXT(CLAY_STRING(""), defaultTextConfig);
        //         }
        //     }
        // }
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

            if (((key >= charCodeLowerBound && key <= charCodeUpperBound) || (!buffer->numberboxConfig.isInt && key == '.' && strchr(buffer->chars, '.') == NULL)) && buffer->length < TEXTBOX_CHARS_MAX)
            {
                size_t i = ++buffer->length;
                for (i; i > buffer->cursorPosition; i--)
                {
                    buffer->chars[i] = buffer->chars[i - 1];
                }
                buffer->chars[buffer->cursorPosition] = key;
                buffer->cursorPosition++;

                if (buffer->numberboxConfig.isNumberbox && key != '.')
                {
                    float clampedVal = atof(buffer->chars);
                    clampedVal = CLAMP(clampedVal, buffer->numberboxConfig.min, buffer->numberboxConfig.max);

                    int charsWritten = snprintf(buffer->chars, TEXTBOX_CHARS_MAX, "%g", clampedVal);
                    if (charsWritten < 0)
                    {
                        exit(EXIT_FAILURE);
                    }

                    buffer->length = charsWritten;
                    buffer->cursorPosition = charsWritten;
                }
            }
        }

        bool nonZeroCursorPosition = buffer->cursorPosition > 0;
        bool nonMaxCursorPosition = buffer->cursorPosition < buffer->length;

        bool isCtrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        bool isShiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

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
        }
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
        }

        if (buffer->numberboxConfig.isNumberbox && buffer->length > 0)
        {
            if (buffer->length == 1 && buffer->chars[0] == '.')
            {
                strcpy(buffer->chars, "0.");
                buffer->length = 2;
                buffer->cursorPosition += 1;
            }

            float clampedVal = atof(buffer->chars);
            clampedVal = CLAMP(clampedVal, buffer->numberboxConfig.min, buffer->numberboxConfig.max);

            int charsWritten = snprintf(buffer->chars, TEXTBOX_CHARS_MAX, "%g", clampedVal);
            if (charsWritten < 0)
            {
                exit(EXIT_FAILURE);
            }

            buffer->length = charsWritten;
            if (buffer->cursorPosition > buffer->length)
            {
                buffer->cursorPosition = charsWritten;
            }
        }

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
        if (IsKeyPressed(KEY_TAB) || IsKeyPressedRepeat(KEY_TAB))
        {
            if (isShiftDown)
            {
                textboxData.focusData.focusIndex = (textboxData.focusData.focusIndex + TEXTBOX_ID_DUMMY_LAST - 1) % TEXTBOX_ID_DUMMY_LAST;
            }
            else
            {
                textboxData.focusData.focusIndex = (textboxData.focusData.focusIndex + 1) % TEXTBOX_ID_DUMMY_LAST;
            }
            buffer = &textboxData.textboxBuffers[textboxData.focusData.focusIndex];
            buffer->cursorPosition = buffer->length;
            textboxData.focusData.focusStartTime = GetTime();
        }
    }
    else
    {
        int key;
        while ((key = GetKeyPressed()))
        {
            if (key == KEY_TAB)
            {
                textboxData.focusData.focusIndex = 0;
                buffer = &textboxData.textboxBuffers[textboxData.focusData.focusIndex];
                buffer->cursorPosition = buffer->length;
                textboxData.focusData.focusStartTime = GetTime();
            }
        }
    }

    if (leftClickPressed && !textboxData.focusData.focusRegistered)
    {
        textboxData.focusData.focusIndex = -1;
    }

    SetMouseCursor(textboxData.hoveringTextbox ? MOUSE_CURSOR_IBEAM : MOUSE_CURSOR_DEFAULT);

    return Clay_EndLayout();
}