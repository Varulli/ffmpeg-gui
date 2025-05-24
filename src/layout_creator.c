#include "../inc/clay.h"

typedef enum
{
    FONT_ID_BODY_16,
    FONT_ID_DUMMY_LAST
} FontID;

const Clay_Color COLOR_WHITE = {255, 255, 255, 255};

void LayoutCreator_Initialize()
{
}

Clay_RenderCommandArray LayoutCreator_CreateLayout()
{
    Clay_BeginLayout();

    Clay_Color windowBackgroundColor = {50, 50, 50, 255};
    Clay_Color sectionBackgroundColor = {70, 70, 70, 255};

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
        .backgroundColor = windowBackgroundColor,
        // .clip = {
        //     .vertical = true,
        //     .horizontal = true,
        //     .childOffset = Clay_GetScrollOffset(),
        // },
    })
    {
        CLAY({
            .id = CLAY_ID("LeftSectionContainer"),
            .layout = {
                // .sizing = {
                //     // .width = CLAY_SIZING_FIXED(150),
                //     .height = CLAY_SIZING_GROW(0),
                // },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 16,
            },
            .backgroundColor = sectionBackgroundColor,
            .cornerRadius = CLAY_CORNER_RADIUS(16),
            .clip = {
                .vertical = true,
                // .horizontal = true,
                .childOffset = Clay_GetScrollOffset(),
            },
        })
        {
            CLAY_TEXT(CLAY_STRING("Select File: "), CLAY_TEXT_CONFIG({
                                                        .fontId = FONT_ID_BODY_16,
                                                        .fontSize = 16,
                                                        .textColor = COLOR_WHITE,
                                                    }));
        }

        CLAY({
            .id = CLAY_ID("RightSectionContainer"),
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_GROW(0),
                },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 16,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = sectionBackgroundColor,
            .cornerRadius = CLAY_CORNER_RADIUS(16),
            .clip = {
                .vertical = true,
                .horizontal = true,
                .childOffset = Clay_GetScrollOffset(),
            },
        })
        {
            CLAY_TEXT(CLAY_STRING("Test"), CLAY_TEXT_CONFIG({
                                               .fontId = FONT_ID_BODY_16,
                                               .fontSize = 16,
                                               .textColor = COLOR_WHITE,
                                           }));
        }
    }

    return Clay_EndLayout();
}