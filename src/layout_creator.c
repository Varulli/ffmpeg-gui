#include "../inc/clay.h"
#include "../inc/raylib.h"
#include "../inc/nfd.h"
#include <stdlib.h>
#include <string.h>

// #define TEXTBOX_DATA_INIT {.buffer = {0}, .cursorPosition = 0}

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
    TEXTBOX_ID_INPUT_FILE,
    TEXTBOX_ID_FILTERS,
    TEXTBOX_ID_OUTPUT_LOCATION,
    TEXTBOX_ID_DUMMY_LAST
} TextboxID;

const Clay_Color COLOR_BG_MAIN = {50, 50, 50, 255};
const Clay_Color COLOR_BG_SECTION = {70, 70, 70, 255};
const Clay_Color COLOR_BG_TEXTBOX = {90, 90, 90, 255};
const Clay_Color COLOR_BORDER_TEXTBOX = {150, 150, 150, 255};
const Clay_Color COLOR_BORDER_TEXTBOX_FOCUSED = {200, 200, 200, 255};
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
    bool hoveringTextBox;
    FocusData focusData;
    Vector2 minDimensions;
} TextboxData;

TextboxBuffer textboxBuffers[TEXTBOX_ID_DUMMY_LAST] = {0};

TextboxData textboxData = {
    .textboxBuffers = textboxBuffers,
    .hoveringTextBox = false,
    .focusData = {
        .focusRegistered = false,
        .focusIndex = -1,
    },
};

int convert()
{
    char cmd[1024] = {0};
    strcat(cmd, "ffmpeg -i ");
    strcat(cmd, textboxData.textboxBuffers[0].chars);
    strcat(cmd, " -vf \"");
    strcat(cmd, textboxData.textboxBuffers[1].chars);
    strcat(cmd, "\" ");
    strcat(cmd, textboxData.textboxBuffers[2].chars);
    printf("DEBUG: cmd = %s\n", cmd);
    return system(cmd);
}

void HandleTextboxInteraction(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData)
{
    size_t index = (size_t)userData;

    if (pointerInfo.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        textboxData.textboxBuffers[index].cursorPosition = textboxData.textboxBuffers[index].length;

        textboxData.focusData.focusRegistered = true;
        textboxData.focusData.focusIndex = index;
        textboxData.focusData.focusStartTime = GetTime();
    }
}

void HandleConvertButtonInteraction(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData)
{
    if (pointerInfo.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        int result = convert();

        if (result)
        {
            printf("DEBUG: result = %d\n", result);
        }
    }
}

void RenderTextBox(Clay_String label, TextboxID textboxId)
{
    bool focused = (textboxData.focusData.focusIndex == textboxId);

    CLAY({
        .id = CLAY_IDI("Textbox", textboxId),
        .layout = {
            .childGap = MeasureText(" ", 16),
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
        },
    })
    {
        if (label.length > 0)
        {
            CLAY_TEXT(label, CLAY_TEXT_CONFIG({
                                 .fontId = FONT_ID_BODY_16,
                                 .fontSize = 16,
                                 .textColor = COLOR_WHITE,
                             }));
        }

        uint16_t borderWidth = focused ? 2 : 1;
        CLAY({
            .layout = {
                .sizing = {
                    .width = {.size = {.minMax = {.min = textboxData.minDimensions.x}}},
                    .height = {.size = {.minMax = {.min = textboxData.minDimensions.y + 8}}},
                },
                .padding = {8, 8, 4, 4},
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
                textboxData.hoveringTextBox = true;
            }

            bool offInterval = (int)(floor((GetTime() - textboxData.focusData.focusStartTime) * 2)) % 2;
            CLAY({
                .border = {
                    .color = offInterval ? COLOR_BG_TEXTBOX : COLOR_WHITE,
                    .width = focused ? (Clay_BorderWidth){0, 0, 0, 0, 2} : (Clay_BorderWidth){0},
                },
            })
            {
                Clay_TextElementConfig *textboxTextConfig = CLAY_TEXT_CONFIG({
                    .fontId = FONT_ID_BODY_16,
                    .fontSize = 16,
                    .textColor = COLOR_WHITE,
                });
                TextboxBuffer *buffer = &textboxData.textboxBuffers[textboxId];

                if (buffer->length == 0)
                {
                    CLAY_TEXT(CLAY_STRING(""), textboxTextConfig);
                    CLAY_TEXT(CLAY_STRING(" "), textboxTextConfig);
                }
                else
                {
                    Clay_String textBeforeCursor = {
                        .isStaticallyAllocated = true,
                        .length = buffer->cursorPosition,
                        .chars = buffer->chars,
                    };
                    Clay_String textAfterCursor = {
                        .isStaticallyAllocated = true,
                        .length = buffer->length - buffer->cursorPosition,
                        .chars = buffer->chars + buffer->cursorPosition,
                    };

                    CLAY_TEXT(textBeforeCursor, textboxTextConfig);
                    CLAY_TEXT(textAfterCursor, textboxTextConfig);
                }
            }
        }
    }
}

void RenderBrowseFilesButton()
{
}

void LayoutCreator_Initialize()
{
}

Clay_RenderCommandArray LayoutCreator_CreateLayout()
{
    Clay_BeginLayout();

    bool leftClickPressed = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    textboxData.hoveringTextBox = false;
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
            CLAY_TEXT(CLAY_STRING("Select File: "), CLAY_TEXT_CONFIG({
                                                        .fontId = FONT_ID_BODY_16,
                                                        .fontSize = 16,
                                                        .textColor = COLOR_WHITE,
                                                    }));

            RenderTextBox(CLAY_STRING("Input File:"), TEXTBOX_ID_INPUT_FILE);
            RenderTextBox(CLAY_STRING("Filters:"), TEXTBOX_ID_FILTERS);
            RenderTextBox(CLAY_STRING("Output Location:"), TEXTBOX_ID_OUTPUT_LOCATION);

            CLAY(0)
            {
                Clay_OnHover(HandleConvertButtonInteraction, 0);

                CLAY_TEXT(CLAY_STRING("Convert"), CLAY_TEXT_CONFIG({
                                                      .fontId = FONT_ID_BODY_16,
                                                      .fontSize = 16,
                                                      .textColor = COLOR_WHITE,
                                                  }));
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
            CLAY_TEXT(CLAY_STRING("Preview"), CLAY_TEXT_CONFIG({
                                                  .fontId = FONT_ID_BODY_16,
                                                  .fontSize = 16,
                                                  .textColor = COLOR_WHITE,
                                              }));

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
                        textboxData.hoveringTextBox = true;
                    }
                    CLAY_TEXT(CLAY_STRING(""), CLAY_TEXT_CONFIG({
                                                   .fontId = FONT_ID_BODY_16,
                                                   .fontSize = 16,
                                                   .textColor = COLOR_WHITE,
                                               }));
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
                int offset = 1;

                if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) &&
                    buffer->length > 1)
                {
                    if (buffer->chars[buffer->cursorPosition - 1] == ' ')
                    {
                        while ((int)buffer->cursorPosition - 1 - offset >= 0 &&
                               buffer->chars[buffer->cursorPosition - 1 - offset] == ' ')
                        {
                            offset++;
                        }
                    }
                    else
                    {
                        while ((int)buffer->cursorPosition - 1 - offset >= 0 &&
                               buffer->chars[buffer->cursorPosition - 1 - offset] != ' ')
                        {
                            offset++;
                        }
                    }
                }

                for (size_t i = buffer->cursorPosition; i <= buffer->length; i++)
                {
                    buffer->chars[i - offset] = buffer->chars[i];
                }
                buffer->cursorPosition -= offset;
                buffer->length -= offset;
                textboxData.focusData.focusStartTime = GetTime();
            }
            if (key == KEY_LEFT && nonZeroCursorPosition)
            {
                if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
                {
                    if (buffer->chars[buffer->cursorPosition - 1] == ' ')
                    {
                        while (buffer->cursorPosition > 0 && buffer->chars[buffer->cursorPosition - 1] == ' ')
                        {
                            buffer->cursorPosition--;
                        }
                    }
                    else
                    {
                        while (buffer->cursorPosition > 0 && buffer->chars[buffer->cursorPosition - 1] != ' ')
                        {
                            buffer->cursorPosition--;
                        }
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
                    if (buffer->chars[buffer->cursorPosition] == ' ')
                    {
                        while (buffer->cursorPosition < buffer->length && buffer->chars[buffer->cursorPosition] == ' ')
                        {
                            buffer->cursorPosition++;
                        }
                    }
                    else
                    {
                        while (buffer->cursorPosition < buffer->length && buffer->chars[buffer->cursorPosition] != ' ')
                        {
                            buffer->cursorPosition++;
                        }
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

    SetMouseCursor(textboxData.hoveringTextBox ? MOUSE_CURSOR_IBEAM : MOUSE_CURSOR_DEFAULT);

    return Clay_EndLayout();
}