#include "layout_model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_failures = 0;

#define EXPECT_TRUE(expr)                                                                 \
    do                                                                                    \
    {                                                                                     \
        if (!(expr))                                                                      \
        {                                                                                 \
            printf("FAIL %s:%d: expected true: %s\n", __FILE__, __LINE__, #expr);       \
            g_failures++;                                                                 \
        }                                                                                 \
    } while (0)

#define EXPECT_EQ_SIZE(expected, actual)                                                   \
    do                                                                                    \
    {                                                                                     \
        size_t e_ = (size_t)(expected);                                                    \
        size_t a_ = (size_t)(actual);                                                      \
        if (e_ != a_)                                                                      \
        {                                                                                 \
            printf("FAIL %s:%d: expected %zu, got %zu\n", __FILE__, __LINE__, e_, a_);  \
            g_failures++;                                                                 \
        }                                                                                 \
    } while (0)

#define EXPECT_EQ_INT(expected, actual)                                                    \
    do                                                                                    \
    {                                                                                     \
        int e_ = (int)(expected);                                                         \
        int a_ = (int)(actual);                                                           \
        if (e_ != a_)                                                                      \
        {                                                                                 \
            printf("FAIL %s:%d: expected %d, got %d\n", __FILE__, __LINE__, e_, a_);    \
            g_failures++;                                                                 \
        }                                                                                 \
    } while (0)

#define EXPECT_STREQ(expected, actual)                                                     \
    do                                                                                    \
    {                                                                                     \
        const char *e_ = (expected);                                                       \
        const char *a_ = (actual);                                                         \
        if (strcmp(e_, a_) != 0)                                                           \
        {                                                                                 \
            printf("FAIL %s:%d: expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, e_, a_); \
            g_failures++;                                                                 \
        }                                                                                 \
    } while (0)

static void set_textbox(LayoutModel *model, LayoutTextboxID id, const char *value)
{
    LayoutTextboxBuffer *buffer = &model->textboxData.textboxBuffers[id];
    snprintf(buffer->chars, sizeof(buffer->chars), "%s", value);
    buffer->length = strlen(buffer->chars);
    buffer->cursorPosition = buffer->length;
}

static void set_defaults(LayoutModel *model)
{
    set_textbox(model, LAYOUT_TEXTBOX_ID_FPS, "30");
    set_textbox(model, LAYOUT_TEXTBOX_ID_DURATION_START_VIDEO, "0");
    set_textbox(model, LAYOUT_TEXTBOX_ID_DURATION_END_VIDEO, "end");
    set_textbox(model, LAYOUT_TEXTBOX_ID_SPEED_VIDEO, "1");
    set_textbox(model, LAYOUT_TEXTBOX_ID_CROP_W, "0");
    set_textbox(model, LAYOUT_TEXTBOX_ID_CROP_H, "0");
    set_textbox(model, LAYOUT_TEXTBOX_ID_CROP_X, "0");
    set_textbox(model, LAYOUT_TEXTBOX_ID_CROP_Y, "0");
    set_textbox(model, LAYOUT_TEXTBOX_ID_SCALE_W, "0");
    set_textbox(model, LAYOUT_TEXTBOX_ID_SCALE_H, "0");
    set_textbox(model, LAYOUT_TEXTBOX_ID_DURATION_START_AUDIO, "0");
    set_textbox(model, LAYOUT_TEXTBOX_ID_DURATION_END_AUDIO, "end");
    set_textbox(model, LAYOUT_TEXTBOX_ID_SPEED_AUDIO, "1");
    set_textbox(model, LAYOUT_TEXTBOX_ID_DELAY, "0");
}

static bool file_exists(const char *path, void *userData)
{
    (void)userData;
    return strcmp(path, "/in.mp4") == 0 || strcmp(path, "/in.gif") == 0 || strcmp(path, "/subs.srt") == 0;
}

static bool dir_exists(const char *path, void *userData)
{
    (void)userData;
    return strcmp(path, "/tmp") == 0 || strcmp(path, ".") == 0;
}

static bool valid_name(const char *name, void *userData)
{
    (void)userData;
    return name != NULL && name[0] != '\0' && strchr(name, '?') == NULL;
}

static int valid_subtitle(const char *path, void *userData)
{
    (void)userData;
    return strcmp(path, "/subs.srt") == 0 ? 0 : 1;
}

static GuiPlatform test_platform(void)
{
    GuiPlatform platform = {
        .fileExists = file_exists,
        .directoryExists = dir_exists,
        .isFileNameValid = valid_name,
        .validateSubtitleFile = valid_subtitle,
        .userData = NULL,
    };
    return platform;
}

static void test_textbox_insert_delete_and_words(void)
{
    LayoutModel model;
    LayoutModel_Init(&model);
    model.textboxData.focusData.focusIndex = LAYOUT_TEXTBOX_ID_INPUT_PATH;

    GuiInputFrame input = {0};
    input.typedChars[0] = 'a';
    input.typedChars[1] = 'b';
    input.typedChars[2] = 'c';
    input.typedCharCount = 3;
    input.now = 1.0;
    LayoutModel_ApplyTextboxInput(&model, &input);
    EXPECT_STREQ("abc", model.textboxData.textboxBuffers[LAYOUT_TEXTBOX_ID_INPUT_PATH].chars);
    EXPECT_EQ_SIZE(3, model.textboxData.textboxBuffers[LAYOUT_TEXTBOX_ID_INPUT_PATH].cursorPosition);

    memset(&input, 0, sizeof(input));
    input.keyPressed[LAYOUT_KEY_LEFT] = true;
    input.now = 2.0;
    LayoutModel_ApplyTextboxInput(&model, &input);
    EXPECT_EQ_SIZE(2, model.textboxData.textboxBuffers[LAYOUT_TEXTBOX_ID_INPUT_PATH].cursorPosition);

    memset(&input, 0, sizeof(input));
    input.keyPressed[LAYOUT_KEY_DELETE] = true;
    input.now = 3.0;
    LayoutModel_ApplyTextboxInput(&model, &input);
    EXPECT_STREQ("ab", model.textboxData.textboxBuffers[LAYOUT_TEXTBOX_ID_INPUT_PATH].chars);

    set_textbox(&model, LAYOUT_TEXTBOX_ID_INPUT_PATH, "one/two three");
    memset(&input, 0, sizeof(input));
    input.keyPressed[LAYOUT_KEY_BACKSPACE] = true;
    input.ctrlDown = true;
    input.now = 4.0;
    LayoutModel_ApplyTextboxInput(&model, &input);
    EXPECT_STREQ("one/two ", model.textboxData.textboxBuffers[LAYOUT_TEXTBOX_ID_INPUT_PATH].chars);
}

static void test_numberbox_clamps_and_decimal(void)
{
    LayoutModel model;
    LayoutModel_Init(&model);
    LayoutTextboxBuffer *buffer = &model.textboxData.textboxBuffers[LAYOUT_TEXTBOX_ID_FPS];
    buffer->numberboxConfig = (LayoutNumberboxConfig){.isNumberbox = true, .isInt = false, .min = 0, .max = 120};
    model.textboxData.focusData.focusIndex = LAYOUT_TEXTBOX_ID_FPS;

    GuiInputFrame input = {0};
    input.typedChars[0] = '.';
    input.typedCharCount = 1;
    input.now = 1.0;
    LayoutModel_ApplyTextboxInput(&model, &input);
    EXPECT_STREQ("0.", buffer->chars);

    memset(&input, 0, sizeof(input));
    input.typedChars[0] = '9';
    input.typedChars[1] = '9';
    input.typedChars[2] = '9';
    input.typedCharCount = 3;
    input.now = 2.0;
    LayoutModel_ApplyTextboxInput(&model, &input);
    EXPECT_STREQ("0.999", buffer->chars);

    memset(&input, 0, sizeof(input));
    input.typedChars[0] = 'x';
    input.typedCharCount = 1;
    LayoutModel_ApplyTextboxInput(&model, &input);
    EXPECT_STREQ("0.999", buffer->chars);
}

static void test_tab_focus_cycle(void)
{
    LayoutModel model;
    LayoutModel_Init(&model);
    set_textbox(&model, LAYOUT_TEXTBOX_ID_FPS, "30");
    model.textboxData.focusData.focusIndex = LAYOUT_TEXTBOX_ID_INPUT_PATH;

    GuiInputFrame input = {0};
    input.keyPressed[LAYOUT_KEY_TAB] = true;
    input.now = 1.0;
    LayoutModel_ApplyTextboxInput(&model, &input);
    EXPECT_EQ_INT(LAYOUT_TEXTBOX_ID_FPS, model.textboxData.focusData.focusIndex);
    EXPECT_EQ_SIZE(2, model.textboxData.textboxBuffers[LAYOUT_TEXTBOX_ID_FPS].cursorPosition);

    memset(&input, 0, sizeof(input));
    input.keyPressed[LAYOUT_KEY_TAB] = true;
    input.shiftDown = true;
    input.now = 2.0;
    LayoutModel_ApplyTextboxInput(&model, &input);
    EXPECT_EQ_INT(LAYOUT_TEXTBOX_ID_INPUT_PATH, model.textboxData.focusData.focusIndex);
}

static void test_parse_streams_json(void)
{
    LayoutModel model;
    char error[128];
    LayoutModel_Init(&model);
    const char *json = "{\"streams\":[{\"codec_type\":\"video\"},{\"codec_type\":\"audio\"},{\"codec_type\":\"subtitle\"},{\"codec_type\":\"audio\"}]}";
    EXPECT_EQ_INT(0, LayoutModel_ParseStreamsJson(&model, json, strlen(json), error, sizeof(error)));
    EXPECT_EQ_SIZE(1, model.streamData.streamCounts[LAYOUT_STREAM_ID_VIDEO]);
    EXPECT_EQ_SIZE(2, model.streamData.streamCounts[LAYOUT_STREAM_ID_AUDIO]);
    EXPECT_EQ_SIZE(1, model.streamData.streamCounts[LAYOUT_STREAM_ID_SUBTITLES]);

    EXPECT_TRUE(LayoutModel_ParseStreamsJson(&model, "{\"format\":{}}", strlen("{\"format\":{}}"), error, sizeof(error)) != 0);
}

static void test_convert_plan_video_audio_subtitles(void)
{
    LayoutModel model;
    ConvertPlan plan;
    char error[256];
    LayoutModel_Init(&model);
    set_defaults(&model);
    snprintf(model.streamData.inputPath, sizeof(model.streamData.inputPath), "%s", "/in.mp4");
    model.streamData.streamCounts[LAYOUT_STREAM_ID_VIDEO] = 1;
    model.streamData.streamCounts[LAYOUT_STREAM_ID_AUDIO] = 1;
    model.dropdownData.selectedValues[LAYOUT_DROPDOWN_ID_OUTPUT_TYPE] = "v";
    model.dropdownData.selectedValues[LAYOUT_DROPDOWN_ID_SUBTITLES] = "burn";
    model.dropdownData.selectedValues[LAYOUT_DROPDOWN_ID_LOUDNORM_ENABLE] = "1";
    model.dropdownData.selectedValues[LAYOUT_DROPDOWN_ID_CHANNEL_LAYOUT] = "stereo";
    set_textbox(&model, LAYOUT_TEXTBOX_ID_OUTPUT_PATH, "/tmp/out.mp4");
    set_textbox(&model, LAYOUT_TEXTBOX_ID_SUBTITLES_SOURCE, "/subs.srt");

    GuiPlatform platform = test_platform();
    EXPECT_EQ_INT(0, LayoutModel_BuildConvertPlan(&model, &platform, &plan, error, sizeof(error)));
    EXPECT_STREQ("/tmp/out.mp4", plan.outputPath);
    EXPECT_TRUE(strstr(plan.filterGraph, "subtitles='/subs.srt'") != NULL);
    EXPECT_TRUE(strstr(plan.filterGraph, "loudnorm") != NULL);
    EXPECT_TRUE(strstr(plan.filterGraph, "channel_layouts=stereo") != NULL);
    EXPECT_STREQ("ffmpeg", plan.argv[0]);
    EXPECT_STREQ("-filter_complex", plan.argv[8]);
    EXPECT_STREQ(plan.filterGraph, plan.argv[9]);
    EXPECT_STREQ("/tmp/out.mp4", plan.argv[plan.argc - 1]);
    LayoutModel_FreeConvertPlan(&plan);
}

static void test_convert_plan_errors_and_image(void)
{
    LayoutModel model;
    ConvertPlan plan;
    char error[256];
    GuiPlatform platform = test_platform();
    LayoutModel_Init(&model);
    set_defaults(&model);
    snprintf(model.streamData.inputPath, sizeof(model.streamData.inputPath), "%s", "/missing.mp4");
    model.dropdownData.selectedValues[LAYOUT_DROPDOWN_ID_OUTPUT_TYPE] = "i";
    set_textbox(&model, LAYOUT_TEXTBOX_ID_OUTPUT_PATH, "/tmp/frame.bad");
    EXPECT_TRUE(LayoutModel_BuildConvertPlan(&model, &platform, &plan, error, sizeof(error)) != 0);
    EXPECT_STREQ("The selected input file does not exist.", error);

    snprintf(model.streamData.inputPath, sizeof(model.streamData.inputPath), "%s", "/in.mp4");
    model.streamData.streamCounts[LAYOUT_STREAM_ID_VIDEO] = 1;
    EXPECT_EQ_INT(0, LayoutModel_BuildConvertPlan(&model, &platform, &plan, error, sizeof(error)));
    EXPECT_STREQ("/tmp/frame.jpg", plan.outputPath);
    EXPECT_TRUE(strstr(plan.filterGraph, "[out_v]") != NULL);
    EXPECT_STREQ("-vframes", plan.argv[plan.argc - 3]);
    LayoutModel_FreeConvertPlan(&plan);
}

static void test_utilities_and_interactions(void)
{
    LayoutModel model;
    LayoutModel_Init(&model);

    EXPECT_STREQ("The selected output height must be divisible by 2 for this conversion.",
                 LayoutModel_SanitizeErrorMessage("\x1b[31mheight not divisible by 2\nrest"));
    EXPECT_STREQ("hello", LayoutModel_Trim("   hello"));
    EXPECT_TRUE(LayoutModel_ExtensionEqualsIgnoreCase(".MP4", ".mp4"));

    model.dropdownData.hoveredOption = 2;
    model.dropdownData.hoveredValue = "a";
    LayoutModel_SelectDropdownHoveredOption(&model, LAYOUT_DROPDOWN_ID_OUTPUT_TYPE);
    EXPECT_EQ_SIZE(2, model.dropdownData.selectedOptions[LAYOUT_DROPDOWN_ID_OUTPUT_TYPE]);
    EXPECT_STREQ("a", model.dropdownData.selectedValues[LAYOUT_DROPDOWN_ID_OUTPUT_TYPE]);

    model.tabData.isDisabled[LAYOUT_TAB_ID_AUDIO] = true;
    LayoutModel_SelectTab(&model, LAYOUT_TAB_ID_AUDIO);
    EXPECT_EQ_SIZE(LAYOUT_TAB_ID_DIMENSIONS, model.tabData.selectedTab);
    LayoutModel_SelectTab(&model, LAYOUT_TAB_ID_VIDEO);
    EXPECT_EQ_SIZE(LAYOUT_TAB_ID_VIDEO, model.tabData.selectedTab);

    model.previewImageData.imageSize = model.previewImageData.imageSizeMin;
    LayoutModel_PreviewSizeDown(&model);
    EXPECT_EQ_SIZE(model.previewImageData.imageSizeMin, model.previewImageData.imageSize);
    model.previewImageData.imageSize = model.previewImageData.imageSizeMax;
    LayoutModel_PreviewSizeUp(&model);
    EXPECT_EQ_SIZE(model.previewImageData.imageSizeMax, model.previewImageData.imageSize);

    model.errorPopupData.count = 3;
    snprintf(model.errorPopupData.popups[0].message, sizeof(model.errorPopupData.popups[0].message), "one");
    snprintf(model.errorPopupData.popups[1].message, sizeof(model.errorPopupData.popups[1].message), "two");
    snprintf(model.errorPopupData.popups[2].message, sizeof(model.errorPopupData.popups[2].message), "three");
    LayoutModel_RemoveErrorPopup(&model, 1);
    EXPECT_EQ_SIZE(2, model.errorPopupData.count);
    EXPECT_STREQ("three", model.errorPopupData.popups[1].message);
}

int main(void)
{
    test_textbox_insert_delete_and_words();
    test_numberbox_clamps_and_decimal();
    test_tab_focus_cycle();
    test_parse_streams_json();
    test_convert_plan_video_audio_subtitles();
    test_convert_plan_errors_and_image();
    test_utilities_and_interactions();

    if (g_failures != 0)
    {
        printf("%d test expectation(s) failed\n", g_failures);
        return 1;
    }
    printf("layout_model tests passed\n");
    return 0;
}
