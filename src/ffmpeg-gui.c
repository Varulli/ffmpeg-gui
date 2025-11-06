#define CLAY_IMPLEMENTATION
#include "../inc/clay.h"
#include "clay_renderer_raylib.c"
#include "layout_creator.c"

void HandleClayErrors(Clay_ErrorData errorData)
{
    printf("%s", errorData.errorText.chars);
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
    fonts[FONT_ID_BODY_16] = LoadFontEx("resources/consolas.ttf", 16, NULL, 400);
    fonts[FONT_ID_BOLD_16] = LoadFontEx("resources/consolas-bold.ttf", 16, NULL, 400);
    for (size_t i = 0; i < FONT_ID_DUMMY_LAST; i++)
    {
        SetTextureFilter(fonts[i].texture, TEXTURE_FILTER_BILINEAR);
    }

    uint64_t clayRequiredMemory = Clay_MinMemorySize();

    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(clayRequiredMemory, malloc(clayRequiredMemory));
    Clay_Context *clayContext = Clay_Initialize(clayMemory, (Clay_Dimensions){.width = GetScreenWidth(), .height = GetScreenHeight()}, (Clay_ErrorHandler){HandleClayErrors}); // This final argument is new since the video was published
    Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);

    LayoutCreator_Initialize(fonts[FONT_ID_BODY_16]);

    // Clay_SetDebugModeEnabled(true);

    while (!WindowShouldClose())
    {
        Clay_RenderCommandArray renderCommands = CreateLayout(clayContext);
        BeginDrawing();
        ClearBackground(BLACK);
        Clay_Raylib_Render(renderCommands, fonts);
        EndDrawing();
    }

    LayoutCreator_Destroy();

    Clay_Raylib_Close();
}