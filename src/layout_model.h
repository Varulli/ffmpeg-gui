#ifndef LAYOUT_MODEL_H
#define LAYOUT_MODEL_H

#include <stdbool.h>
#include <stddef.h>

#define LAYOUT_TEXTBOX_BUFFER_SIZE 256
#define LAYOUT_MAX_TYPED_CHARS 64

typedef enum
{
    LAYOUT_TEXTBOX_ID_INPUT_PATH,
    LAYOUT_TEXTBOX_ID_FPS,
    LAYOUT_TEXTBOX_ID_DURATION_START_VIDEO,
    LAYOUT_TEXTBOX_ID_DURATION_END_VIDEO,
    LAYOUT_TEXTBOX_ID_SPEED_VIDEO,
    LAYOUT_TEXTBOX_ID_CROP_W,
    LAYOUT_TEXTBOX_ID_CROP_H,
    LAYOUT_TEXTBOX_ID_CROP_X,
    LAYOUT_TEXTBOX_ID_CROP_Y,
    LAYOUT_TEXTBOX_ID_SCALE_W,
    LAYOUT_TEXTBOX_ID_SCALE_H,
    LAYOUT_TEXTBOX_ID_VOLUME,
    LAYOUT_TEXTBOX_ID_DURATION_START_AUDIO,
    LAYOUT_TEXTBOX_ID_DURATION_END_AUDIO,
    LAYOUT_TEXTBOX_ID_SPEED_AUDIO,
    LAYOUT_TEXTBOX_ID_DELAY,
    LAYOUT_TEXTBOX_ID_SUBTITLES_SOURCE,
    LAYOUT_TEXTBOX_ID_OUTPUT_PATH,
    LAYOUT_TEXTBOX_ID_DUMMY_LAST
} LayoutTextboxID;

typedef enum
{
    LAYOUT_DROPDOWN_ID_OUTPUT_TYPE,
    LAYOUT_DROPDOWN_ID_LOUDNORM_ENABLE,
    LAYOUT_DROPDOWN_ID_CHANNEL_LAYOUT,
    LAYOUT_DROPDOWN_ID_SUBTITLES,
    LAYOUT_DROPDOWN_ID_DUMMY_LAST
} LayoutDropdownID;

typedef enum
{
    LAYOUT_STREAM_ID_VIDEO,
    LAYOUT_STREAM_ID_AUDIO,
    LAYOUT_STREAM_ID_SUBTITLES,
    LAYOUT_STREAM_ID_DUMMY_LAST,
} LayoutStreamID;

typedef enum
{
    LAYOUT_TAB_ID_DIMENSIONS,
    LAYOUT_TAB_ID_VIDEO,
    LAYOUT_TAB_ID_AUDIO,
    LAYOUT_TAB_ID_SUBTITLES,
    LAYOUT_TAB_ID_DUMMY_LAST,
} LayoutTabID;

typedef enum
{
    LAYOUT_KEY_BACKSPACE,
    LAYOUT_KEY_DELETE,
    LAYOUT_KEY_LEFT,
    LAYOUT_KEY_RIGHT,
    LAYOUT_KEY_TAB,
    LAYOUT_KEY_DUMMY_LAST,
} LayoutKey;

typedef struct
{
    bool isNumberbox;
    bool isInt;
    float min;
    float max;
} LayoutNumberboxConfig;

typedef struct
{
    char chars[LAYOUT_TEXTBOX_BUFFER_SIZE];
    const char *charsDefault;
    size_t length;
    size_t cursorPosition;
    LayoutNumberboxConfig numberboxConfig;
} LayoutTextboxBuffer;

typedef struct
{
    bool focusRegistered;
    int focusIndex;
    double focusStartTime;
} LayoutFocusData;

typedef struct
{
    LayoutTextboxBuffer textboxBuffers[LAYOUT_TEXTBOX_ID_DUMMY_LAST];
    bool isInit[LAYOUT_TEXTBOX_ID_DUMMY_LAST];
    bool isEnabled[LAYOUT_TEXTBOX_ID_DUMMY_LAST];
    int hoveredTextbox;
    LayoutFocusData focusData;
} LayoutTextboxData;

typedef struct
{
    size_t selectedOptions[LAYOUT_DROPDOWN_ID_DUMMY_LAST];
    const char *selectedValues[LAYOUT_DROPDOWN_ID_DUMMY_LAST];
    bool isInit[LAYOUT_DROPDOWN_ID_DUMMY_LAST];
    size_t hoveredOption;
    const char *hoveredValue;
} LayoutDropdownData;

typedef struct
{
    size_t selectedTab;
    bool isDisabled[LAYOUT_TAB_ID_DUMMY_LAST];
} LayoutTabData;

typedef struct
{
    char inputPath[LAYOUT_TEXTBOX_BUFFER_SIZE];
    size_t streamCounts[LAYOUT_STREAM_ID_DUMMY_LAST];
    char convertOutput[8192];
    size_t convertOutputLength;
} LayoutStreamData;

typedef struct
{
    char message[256];
    double createdAt;
} LayoutErrorPopup;

typedef struct
{
    LayoutErrorPopup popups[4];
    size_t count;
} LayoutErrorPopupData;

typedef struct
{
    size_t imageSize;
    size_t imageSizeMin;
    size_t imageSizeMax;
} LayoutPreviewImageData;

typedef struct
{
    LayoutTextboxData textboxData;
    LayoutDropdownData dropdownData;
    LayoutTabData tabData;
    LayoutStreamData streamData;
    LayoutPreviewImageData previewImageData;
    LayoutErrorPopupData errorPopupData;
} LayoutModel;

typedef struct
{
    int typedChars[LAYOUT_MAX_TYPED_CHARS];
    size_t typedCharCount;
    bool keyPressed[LAYOUT_KEY_DUMMY_LAST];
    bool keyRepeated[LAYOUT_KEY_DUMMY_LAST];
    bool ctrlDown;
    bool shiftDown;
    bool leftMouseReleased;
    double now;
} GuiInputFrame;

typedef struct
{
    bool (*fileExists)(const char *path, void *userData);
    bool (*directoryExists)(const char *path, void *userData);
    bool (*isFileNameValid)(const char *name, void *userData);
    int (*validateSubtitleFile)(const char *path, void *userData);
    void *userData;
} GuiPlatform;

typedef struct
{
    char **argv;
    size_t argc;
    char *filterGraph;
    char *outputPath;
} ConvertPlan;

void LayoutModel_Init(LayoutModel *model);
const char *LayoutModel_GetTextboxValue(const LayoutModel *model, LayoutTextboxID textboxId);
const char *LayoutModel_GetDropdownValue(const LayoutModel *model, LayoutDropdownID dropdownId);
void LayoutModel_ApplyTextboxInput(LayoutModel *model, const GuiInputFrame *input);
int LayoutModel_ParseStreamsJson(LayoutModel *model, const char *json, size_t len, char *error, size_t errorCap);
int LayoutModel_BuildConvertPlan(LayoutModel *model, const GuiPlatform *platform, ConvertPlan *out, char *error, size_t errorCap);
void LayoutModel_FreeConvertPlan(ConvertPlan *plan);
const char *LayoutModel_SanitizeErrorMessage(const char *message);
void LayoutModel_SelectDropdownHoveredOption(LayoutModel *model, LayoutDropdownID dropdownId);
void LayoutModel_SelectTab(LayoutModel *model, LayoutTabID tabId);
void LayoutModel_PreviewSizeDown(LayoutModel *model);
void LayoutModel_PreviewSizeUp(LayoutModel *model);
void LayoutModel_RemoveErrorPopup(LayoutModel *model, size_t index);

bool LayoutModel_CharMatchesAny(char c, const char *matchString);
bool LayoutModel_ExtensionEqualsIgnoreCase(const char *a, const char *b);
const char *LayoutModel_Trim(const char *str);

#endif
