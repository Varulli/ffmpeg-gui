#include "../inc/clay.h"
#include "../inc/raylib.h"
#include "../inc/nfd.h"
#include <stdlib.h>
#include <string.h>

#define DROPDOWN_OPTION_NULL {CLAY_STRING(""), NULL}
#define DROPDOWN_OPTION_UNSELECTED {CLAY_STRING("-- None --"), ""}

#define NUM_TEST 5
#define TEXTBOX_CHARS_MAX 255

// #define CLAMP(val, min, max) (val < min) ? min : (val > max) ? max \
//                                                              : val

typedef enum
{
    FONT_ID_BODY_16,
    FONT_ID_TEST_16,
    FONT_ID_DUMMY_LAST
} FontID;

typedef enum
{
    TEXTBOX_ID_INPUT_PATH,
    TEXTBOX_ID_FILTERS,
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
const Clay_Color COLOR_BG_BUTTON = {90, 90, 90, 255};
const Clay_Color COLOR_BG_DROPDOWN = {90, 90, 90, 255};
const Clay_Color COLOR_BG_BUTTON_HOVERED = {110, 110, 110, 255};
const Clay_Color COLOR_BG_DROPDOWN_HOVERED = {110, 110, 110, 255};
const Clay_Color COLOR_BORDER_TEXTBOX = {150, 150, 150, 255};
const Clay_Color COLOR_BORDER_TEXTBOX_FOCUSED = {200, 200, 200, 255};
const Clay_Color COLOR_BORDER_BUTTON = {150, 150, 150, 255};
const Clay_Color COLOR_BORDER_DROPDOWN = {150, 150, 150, 255};
const Clay_Color COLOR_WHITE = {255, 255, 255, 255};
const Clay_Color COLOR_RED = {255, 0, 0, 255};
const Clay_Color COLOR_GREEN = {0, 255, 0, 255};

typedef struct
{
    bool focusRegistered;
    int focusIndex;
    double focusStartTime;
} FocusData;

typedef struct
{
    char chars[TEXTBOX_CHARS_MAX];
    size_t length;
    size_t cursorPosition;
} TextboxBuffer;

typedef struct
{
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

TextboxBuffer textboxBuffers[TEXTBOX_ID_DUMMY_LAST] = {0};

TextboxData textboxData = {
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

Clay_TextElementConfig *defaultTextConfig;
Clay_Padding defaultBoxPadding;
Clay_CornerRadius defaultCornerRadius;

// bool once = true;

int convert()
{
    char cmd[1024] = {0};
    strcat(cmd, "ffmpeg -i \"");
    strcat(cmd, textboxData.textboxBuffers[TEXTBOX_ID_INPUT_PATH].chars);
    strcat(cmd, "\" -vf \"");
    strcat(cmd, textboxData.textboxBuffers[TEXTBOX_ID_FILTERS].chars);
    strcat(cmd, "\" \"");
    strcat(cmd, textboxData.textboxBuffers[TEXTBOX_ID_OUTPUT_PATH].chars);
    strcat(cmd, "\"");
    printf("DEBUG: cmd = %s\n", cmd);
    printf("DEBUG: dropdown (test) = %s\n", dropdownData.selectedValues[DROPDOWN_ID_TEST]);
    return system(cmd);
}

void HandleTextboxInteraction(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData)
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

void HandleConvertButtonInteraction(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData)
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

void HandleBrowseButtonInteraction(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData)
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

void HandleDropdownOptionInteraction(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData)
{
    if (pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        size_t index = (size_t)userData;

        dropdownData.selectedOptions[index] = dropdownData.hoveredOption;
        dropdownData.selectedValues[index] = dropdownData.hoveredValue;
    }
}

void RenderTextbox(Clay_String label, TextboxID textboxId, size_t maxCharsDisplayed)
{
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
            CLAY_TEXT(label, defaultTextConfig);
        }

        uint16_t borderWidth = focused ? 2 : 1;
        CLAY({
            .layout = {
                .sizing = {
                    .width = {.size = {.minMax = {.min = textboxData.minDimensions.x * (maxCharsDisplayed + 2)}}},
                },
                .padding = defaultBoxPadding,
            },
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
                    .color = offInterval ? COLOR_BG_TEXTBOX : COLOR_WHITE,
                    .width = focused ? (Clay_BorderWidth){0, 0, 0, 0, 2} : (Clay_BorderWidth){0},
                },
            })
            {
                TextboxBuffer *buffer = &textboxData.textboxBuffers[textboxId];

                if (buffer->length == 0)
                {
                    CLAY_TEXT(CLAY_STRING(""), defaultTextConfig);
                    CLAY_TEXT(CLAY_STRING(" "), defaultTextConfig);
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

                    CLAY_TEXT(textBeforeCursor, defaultTextConfig);
                    CLAY_TEXT(textAfterCursor, defaultTextConfig);
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

        CLAY_TEXT(CLAY_STRING("..."), defaultTextConfig);
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
            CLAY_TEXT(label, defaultTextConfig);
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
                        .size = {.minMax = {.min = textboxData.minDimensions.x * (maxLength + 2)}},
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
                CLAY_TEXT(options[dropdownData.selectedOptions[dropdownId]].label, defaultTextConfig);
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
                                            .size = {.minMax = {.min = textboxData.minDimensions.x * (maxLength + 2)}},
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

                                CLAY_TEXT(options[i].label, defaultTextConfig);
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
    defaultTextConfig = CLAY_TEXT_CONFIG({
        .fontId = FONT_ID_BODY_16,
        .fontSize = 16,
        .textColor = {255, 255, 255, 255},
    });

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
                .childGap = 16,
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
            CLAY_TEXT(CLAY_STRING("Select File: "), defaultTextConfig);

            CLAY({.layout = {.childGap = 4}})
            {
                RenderTextbox(CLAY_STRING("Input File:"), TEXTBOX_ID_INPUT_PATH, 30);
                RenderBrowseButton(TEXTBOX_ID_INPUT_PATH);
            }

            RenderDropdown(CLAY_STRING("Filters:"), DROPDOWN_ID_TEST, (DropdownOption[]){
                                                                          DROPDOWN_OPTION_UNSELECTED,
                                                                          {CLAY_STRING("Option 1"), "Option 1"},
                                                                          {CLAY_STRING("Option 2 blah"), "Option 2"},
                                                                          {CLAY_STRING("Option 3 yo"), "Option 3"},
                                                                          DROPDOWN_OPTION_NULL,
                                                                      });

            CLAY({.layout = {.childGap = 4}})
            {
                RenderTextbox(CLAY_STRING("Output Folder:"), TEXTBOX_ID_OUTPUT_PATH, 27);
                RenderBrowseButton(TEXTBOX_ID_OUTPUT_PATH);
            }

            CLAY(0)
            {
                Clay_OnHover(HandleConvertButtonInteraction, 0);

                CLAY_TEXT(CLAY_STRING("Convert"), defaultTextConfig);
            }
        }

        CLAY({
            .id = CLAY_ID("RightSectionContainer"),
            .layout = {
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 16,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = COLOR_BG_SECTION,
            .cornerRadius = CLAY_CORNER_RADIUS(16),
            .clip = {
                .vertical = true,
                .horizontal = true,
                .childOffset = Clay_GetScrollOffset(),
            },
        })
        {
            CLAY_TEXT(CLAY_STRING("Preview"), defaultTextConfig);

            for (size_t i = 0; i < NUM_TEST; i++)
            {
                CLAY({
                    .layout = {
                        .padding = CLAY_PADDING_ALL(16),
                    },
                    .backgroundColor = (i == textboxData.focusData.focusIndex) ? COLOR_GREEN : COLOR_RED,
                    .cornerRadius = CLAY_CORNER_RADIUS(8),
                })
                {
                    Clay_OnHover(HandleTextboxInteraction, i);
                    if (Clay_Hovered())
                    {
                        textboxData.hoveringTextbox = true;
                    }
                    CLAY_TEXT(CLAY_STRING(""), defaultTextConfig);
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
            if (key >= 32 && key <= 126 && buffer->length < TEXTBOX_CHARS_MAX)
            {
                size_t i = ++buffer->length;
                for (i; i > buffer->cursorPosition; i--)
                {
                    buffer->chars[i] = buffer->chars[i - 1];
                }
                buffer->chars[buffer->cursorPosition] = key;
                buffer->cursorPosition++;
            }
        }

        while ((key = GetKeyPressed()))
        {
            bool nonZeroCursorPosition = buffer->cursorPosition > 0;
            bool nonMaxCursorPosition = buffer->cursorPosition < buffer->length;

            if (key == KEY_BACKSPACE && nonZeroCursorPosition)
            {
                int offset = 0;

                if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
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
            if (key == KEY_DELETE && nonMaxCursorPosition)
            {
                int offset = 0;

                if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
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
            if (key == KEY_LEFT && nonZeroCursorPosition)
            {
                if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
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
            if (key == KEY_RIGHT && nonMaxCursorPosition)
            {
                if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
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
            if (key == KEY_TAB)
            {
                if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
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