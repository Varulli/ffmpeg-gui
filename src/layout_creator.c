#include "../inc/clay.h"
#include "../inc/raylib.h"

// #define TEXTBOX_DATA_INIT {.buffer = {0}, .cursorPosition = 0}

#define NUM_TEST 5
#define TEXTBOX_CHARS_MAX 255

typedef enum
{
    FONT_ID_BODY_16,
    FONT_ID_TEST_16,
    FONT_ID_DUMMY_LAST
} FontID;

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
} FocusData;

typedef struct
{
    char buffer[TEXTBOX_CHARS_MAX];
    size_t length;
    size_t cursorPosition;
} TextboxBuffer;

typedef struct
{
    TextboxBuffer *textboxBuffers;
    size_t nextTextboxIndex;
    bool hoveringTextBox;
    FocusData focusData;
} TextboxData;

TextboxBuffer textboxBuffers[] = {
    {.buffer = {'T', ' ', '1', '\0'}, .length = 3, .cursorPosition = 0}, // Test 1
    {.buffer = {'T', ' ', '2', '\0'}, .length = 3, .cursorPosition = 0}, // Test 2
    {.buffer = {'T', ' ', '3', '\0'}, .length = 3, .cursorPosition = 0}, // Test 3
};

TextboxData textboxData = {
    .textboxBuffers = textboxBuffers,
    .nextTextboxIndex = 0,
    .hoveringTextBox = false,
    .focusData = {
        .focusRegistered = false,
        .focusIndex = -1,
    },
};

void HandleTextboxInteraction(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData)
{
    size_t index = (size_t)userData;

    if (pointerInfo.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    {
        textboxData.focusData.focusRegistered = true;
        textboxData.focusData.focusIndex = index;
    }
}

void RenderTextBox(Clay_String label)
{
    size_t index = textboxData.nextTextboxIndex++;
    bool focused = (textboxData.focusData.focusIndex == index);

    CLAY({
        .id = CLAY_IDI("Textbox", index),
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

        CLAY({
            .layout = {
                .padding = {8, 8, 4, 4},
            },
            .backgroundColor = COLOR_BG_TEXTBOX,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
            .border = {
                .color = focused ? COLOR_BORDER_TEXTBOX_FOCUSED : COLOR_BORDER_TEXTBOX,
                .width = CLAY_BORDER_OUTSIDE(focused ? 2 : 1),
            },
        })
        {
            Clay_OnHover(HandleTextboxInteraction, index);
            if (Clay_Hovered())
            {
                textboxData.hoveringTextBox = true;
            }

            Clay_String content = {
                .isStaticallyAllocated = true,
                .length = textboxData.textboxBuffers[index].length,
                .chars = textboxData.textboxBuffers[index].buffer,
            };
            CLAY_TEXT(content, CLAY_TEXT_CONFIG({
                                   .fontId = FONT_ID_BODY_16,
                                   .fontSize = 16,
                                   .textColor = COLOR_WHITE,
                               }));
        }
    }
}

void LayoutCreator_Initialize()
{
}

Clay_RenderCommandArray LayoutCreator_CreateLayout()
{
    Clay_BeginLayout();

    bool leftClickPressed = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    textboxData.nextTextboxIndex = 0;
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

            RenderTextBox(CLAY_STRING("Test 1:"));
            RenderTextBox(CLAY_STRING("Test 2:"));
            RenderTextBox(CLAY_STRING("Test 3:"));
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

    if (leftClickPressed && !textboxData.focusData.focusRegistered)
    {
        textboxData.focusData.focusIndex = -1;
    }

    SetMouseCursor(textboxData.hoveringTextBox ? MOUSE_CURSOR_IBEAM : MOUSE_CURSOR_DEFAULT);

    return Clay_EndLayout();
}