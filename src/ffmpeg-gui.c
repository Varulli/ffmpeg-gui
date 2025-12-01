#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer_raylib.c"
#include "layout_creator.c"

void DrawAllGlyphs(Font font, int startX, int startY, int scale, Color color)
{
    int x = startX;
    int y = startY;
    int spacing = font.baseSize * scale + 8; // space between glyphs

    for (int i = 0; i < font.glyphCount; i++)
    {
        GlyphInfo g = font.glyphs[i];

        // Draw background box for clarity
        DrawRectangle(x - 2, y - 2, spacing, spacing, BLACK);
        DrawRectangleLines(x - 2, y - 2, spacing, spacing, LIGHTGRAY);

        // Draw glyph from the texture atlas
        Rectangle src = font.recs[i];
        DrawTexturePro(font.texture, src,
                       (Rectangle){x, y, src.width * scale, src.height * scale},
                       (Vector2){0, 0}, 0, color);

        // Advance for next glyph
        x += spacing;
        if (x > GetScreenWidth() - spacing)
        {
            x = startX;
            y += spacing;
        }
    }
}

void HandleClayErrors(Clay_ErrorData errorData)
{
    ERROR("%s", errorData.errorText.chars);
}

Clay_RenderCommandArray CreateLayout(Clay_Context *context)
{
    Clay_SetCurrentContext(context);
    // Run once per frame
    Clay_SetLayoutDimensions((Clay_Dimensions){
        .width = GetScreenWidth(),
        .height = GetScreenHeight(),
    });
    Vector2 mousePosition = GetMousePosition();
    Vector2 scrollDelta = GetMouseWheelMoveV();
    Clay_SetPointerState(
        (Clay_Vector2){mousePosition.x, mousePosition.y},
        IsMouseButtonDown(0));
    Clay_UpdateScrollContainers(
        true,
        (Clay_Vector2){scrollDelta.x, scrollDelta.y},
        GetFrameTime());
    return LayoutCreator_CreateLayout();
}

int main(void)
{
    Clay_Raylib_Initialize(1000, 600, "ffmpeg GUI", FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT); // Extra parameters to this function are new since the video was published

    Font fonts[FONT_ID_DUMMY_LAST];
    fonts[FONT_ID_BODY] = LoadFontEx("resources/fonts/consolas.ttf", fontData.fontSizeMax, NULL, 0);
    fonts[FONT_ID_BOLD] = LoadFontEx("resources/fonts/consolas-bold.ttf", fontData.fontSizeMax, NULL, 0);
    int codepoints[] = {0x25B2, 0x25B3, 0x25BC, 0x25BD, 'A', 'B', 'C', 'D', 'E', 'F'};
    fonts[FONT_ID_SYMBOL] = LoadFontEx("resources/fonts/NotoSansJP-Regular.ttf", fontData.fontSizeMax, codepoints, sizeof(codepoints) / sizeof(codepoints[0]));
    for (size_t i = 0; i < FONT_ID_DUMMY_LAST; i++)
    {
        SetTextureFilter(fonts[i].texture, TEXTURE_FILTER_BILINEAR);
    }

    uint64_t clayRequiredMemory = Clay_MinMemorySize();

    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(clayRequiredMemory, malloc(clayRequiredMemory));
    Clay_Context *clayContext = Clay_Initialize(clayMemory, (Clay_Dimensions){.width = GetScreenWidth(), .height = GetScreenHeight()}, (Clay_ErrorHandler){HandleClayErrors}); // This final argument is new since the video was published
    Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);

    LayoutCreator_Initialize(fonts[FONT_ID_BODY]);

    Clay_SetDebugModeEnabled(true);

    while (!WindowShouldClose())
    {
        Clay_RenderCommandArray renderCommands = CreateLayout(clayContext);
        BeginDrawing();
        ClearBackground(BLACK);
        Clay_Raylib_Render(renderCommands, fonts);

        // DrawAllGlyphs(fonts[FONT_ID_BODY], 10, 10, 1, WHITE);
        // DrawAllGlyphs(fonts[FONT_ID_BOLD], 10, 210, 1, WHITE);
        // DrawAllGlyphs(fonts[FONT_ID_SYMBOL], 10, 10, 1, WHITE);

        EndDrawing();
    }

    LayoutCreator_Destroy();

    Clay_Raylib_Close();
}