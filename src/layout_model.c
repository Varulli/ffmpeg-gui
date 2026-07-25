#include "layout_model.h"
#include "cJSON.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LAYOUT_CLAMP(val, min, max) ((val) < (min) ? (min) : (val) > (max) ? (max) \
                                                                           : (val))

typedef struct
{
    char *buffer;
    size_t len;
    size_t cap;
} LayoutStringBuilder;

typedef struct
{
    char **v;
    size_t len;
    size_t cap;
} LayoutArgvBuilder;

static char *layout_strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *out = malloc(n);
    if (out != NULL)
    {
        memcpy(out, s, n);
    }
    return out;
}

static char *layout_argprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0)
    {
        return NULL;
    }

    char *buffer = malloc((size_t)n + 1);
    if (buffer == NULL)
    {
        return NULL;
    }

    va_start(ap, fmt);
    vsnprintf(buffer, (size_t)n + 1, fmt, ap);
    va_end(ap);
    return buffer;
}

static int set_error(char *error, size_t errorCap, const char *fmt, ...)
{
    if (error != NULL && errorCap > 0)
    {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(error, errorCap, fmt, ap);
        va_end(ap);
    }
    return 1;
}

static void append_convert_error_output(LayoutModel *model, const char *data, size_t dataLength)
{
    if (model == NULL || data == NULL || dataLength == 0)
    {
        return;
    }

    size_t available = sizeof(model->streamData.convertOutput) - model->streamData.convertOutputLength - 1;
    if (available == 0)
    {
        return;
    }

    size_t copyLength = dataLength < available ? dataLength : available;
    memcpy(model->streamData.convertOutput + model->streamData.convertOutputLength, data, copyLength);
    model->streamData.convertOutputLength += copyLength;
    model->streamData.convertOutput[model->streamData.convertOutputLength] = '\0';
}

static void append_convert_error_line(LayoutModel *model, const char *line)
{
    append_convert_error_output(model, line, strlen(line));
    append_convert_error_output(model, "\n", 1);
}

static void sb_init(LayoutStringBuilder *sb)
{
    sb->buffer = NULL;
    sb->len = 0;
    sb->cap = 0;
}

static bool sb_reserve(LayoutStringBuilder *sb, size_t extra)
{
    if (sb->len + extra + 1 <= sb->cap)
    {
        return true;
    }

    size_t newCap = sb->cap ? sb->cap * 2 : 128;
    while (newCap < sb->len + extra + 1)
    {
        newCap *= 2;
    }

    char *tmp = realloc(sb->buffer, newCap);
    if (tmp == NULL)
    {
        return false;
    }
    sb->buffer = tmp;
    sb->cap = newCap;
    return true;
}

static bool sb_append(LayoutStringBuilder *sb, const char *s)
{
    size_t n = strlen(s);
    if (!sb_reserve(sb, n))
    {
        return false;
    }
    memcpy(sb->buffer + sb->len, s, n);
    sb->len += n;
    sb->buffer[sb->len] = '\0';
    return true;
}

static bool sb_appendf(LayoutStringBuilder *sb, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0 || !sb_reserve(sb, (size_t)n))
    {
        return false;
    }

    va_start(ap, fmt);
    vsnprintf(sb->buffer + sb->len, (size_t)n + 1, fmt, ap);
    va_end(ap);
    sb->len += (size_t)n;
    return true;
}

static void argv_init(LayoutArgvBuilder *a)
{
    a->v = NULL;
    a->len = 0;
    a->cap = 0;
}

static bool argv_push_owned(LayoutArgvBuilder *a, char *arg)
{
    if (arg == NULL)
    {
        return false;
    }
    if (a->len + 1 >= a->cap)
    {
        size_t newCap = a->cap ? a->cap * 2 : 8;
        char **tmp = realloc(a->v, newCap * sizeof(char *));
        if (tmp == NULL)
        {
            free(arg);
            return false;
        }
        a->v = tmp;
        a->cap = newCap;
    }
    a->v[a->len++] = arg;
    a->v[a->len] = NULL;
    return true;
}

static bool argv_push(LayoutArgvBuilder *a, const char *arg)
{
    return argv_push_owned(a, layout_strdup(arg));
}

static void argv_free(LayoutArgvBuilder *a)
{
    if (a == NULL)
    {
        return;
    }
    for (size_t i = 0; i < a->len; i++)
    {
        free(a->v[i]);
    }
    free(a->v);
    a->v = NULL;
    a->len = 0;
    a->cap = 0;
}

bool LayoutModel_CharMatchesAny(char c, const char *matchString)
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

static int clamp_float_as_string(char *floatAsString, float min, float max)
{
    float clampedVal = (float)atof(floatAsString);
    clampedVal = LAYOUT_CLAMP(clampedVal, min, max);
    return snprintf(floatAsString, LAYOUT_TEXTBOX_BUFFER_SIZE, "%g", clampedVal);
}

const char *LayoutModel_Trim(const char *str)
{
    size_t i;
    for (i = 0; str[i] != '\0'; i++)
    {
        if (str[i] != ' ')
        {
            break;
        }
    }
    return str + i;
}

bool LayoutModel_ExtensionEqualsIgnoreCase(const char *a, const char *b)
{
    if (a == NULL || b == NULL)
    {
        return false;
    }
    while (*a && *b)
    {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
        {
            return false;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static const char *get_file_extension(const char *path)
{
    const char *slash = strrchr(path, '/');
#if defined(_WIN32)
    const char *backslash = strrchr(path, '\\');
    if (slash == NULL || (backslash != NULL && backslash > slash))
    {
        slash = backslash;
    }
#endif
    const char *dot = strrchr(path, '.');
    if (dot == NULL || (slash != NULL && dot < slash))
    {
        return NULL;
    }
    return dot;
}

static char path_slash_for_dir(const char *dir)
{
#if defined(_WIN32)
    if (strchr(dir, '\\') != NULL)
    {
        return '\\';
    }
#endif
    return '/';
}

static bool split_output_path(const char *outputPath, char *dir, size_t dirCap, char *name, size_t nameCap)
{
    const char *slash = strrchr(outputPath, '/');
#if defined(_WIN32)
    const char *backslash = strrchr(outputPath, '\\');
    if (slash == NULL || (backslash != NULL && backslash > slash))
    {
        slash = backslash;
    }
#endif
    const char *filename = slash == NULL ? outputPath : slash + 1;
    size_t dirLen = slash == NULL ? 1 : (size_t)(slash - outputPath);
    const char *dirStart = slash == NULL ? "." : outputPath;
    if (dirLen + 1 > dirCap)
    {
        return false;
    }
    memcpy(dir, dirStart, dirLen);
    dir[dirLen] = '\0';

    const char *dot = strrchr(filename, '.');
    size_t nameLen = dot == NULL ? strlen(filename) : (size_t)(dot - filename);
    if (nameLen + 1 > nameCap)
    {
        return false;
    }
    memcpy(name, filename, nameLen);
    name[nameLen] = '\0';
    return true;
}

static double parse_seconds(const char *s)
{
    if (s == NULL || s[0] == '\0')
    {
        return -1.0;
    }
    char *end = NULL;
    double val = strtod(s, &end);
    if (end == s || val < 0.0)
    {
        return -1.0;
    }
    return val;
}

static double parse_timecode_seconds(const char *s)
{
    if (s == NULL || s[0] == '\0')
    {
        return -1.0;
    }

    int hours = 0;
    int minutes = 0;
    double seconds = 0.0;
    if (sscanf(s, "%d:%d:%lf", &hours, &minutes, &seconds) == 3)
    {
        if (hours < 0 || minutes < 0 || minutes > 59 || seconds < 0.0)
        {
            return -1.0;
        }
        return hours * 3600.0 + minutes * 60.0 + seconds;
    }

    return parse_seconds(s);
}

static bool value_is_end(const char *s)
{
    return s == NULL || s[0] == '\0' || strcmp(s, "end") == 0;
}

static double estimate_trimmed_duration(const LayoutModel *model, LayoutTextboxID startId, LayoutTextboxID endId, LayoutTextboxID speedId)
{
    double start = parse_seconds(LayoutModel_GetTextboxValue(model, startId));
    if (start < 0.0)
    {
        start = 0.0;
    }

    const char *endText = LayoutModel_GetTextboxValue(model, endId);
    double end = -1.0;
    if (value_is_end(endText))
    {
        end = model->streamData.inputDurationSeconds;
    }
    else
    {
        end = parse_seconds(endText);
    }
    if (end < 0.0 || end < start)
    {
        return -1.0;
    }

    double speed = parse_seconds(LayoutModel_GetTextboxValue(model, speedId));
    if (speed <= 0.0)
    {
        return -1.0;
    }

    return (end - start) / speed;
}

void LayoutModel_Init(LayoutModel *model)
{
    if (model == NULL)
    {
        return;
    }
    memset(model, 0, sizeof(*model));
    for (size_t i = 0; i < LAYOUT_TEXTBOX_ID_DUMMY_LAST; i++)
    {
        model->textboxData.isEnabled[i] = true;
    }
    model->textboxData.hoveredTextbox = -1;
    model->textboxData.focusData.focusIndex = -1;
    model->tabData.selectedTab = LAYOUT_TAB_ID_DIMENSIONS;
    model->previewImageData.imageSize = 500;
    model->previewImageData.imageSizeMin = 100;
    model->previewImageData.imageSizeMax = 1500;
}

const char *LayoutModel_GetTextboxValue(const LayoutModel *model, LayoutTextboxID textboxId)
{
    const LayoutTextboxBuffer *buffer = &model->textboxData.textboxBuffers[textboxId];
    if (buffer->length > 0)
    {
        return buffer->chars;
    }
    return buffer->charsDefault != NULL ? buffer->charsDefault : "";
}

const char *LayoutModel_GetDropdownValue(const LayoutModel *model, LayoutDropdownID dropdownId)
{
    const char *value = model->dropdownData.selectedValues[dropdownId];
    return value != NULL ? value : "";
}

static bool input_key(const GuiInputFrame *input, LayoutKey key)
{
    return input->keyPressed[key] || input->keyRepeated[key];
}

static bool textbox_focusable(const LayoutModel *model, int textboxId)
{
    return textboxId >= 0 &&
           textboxId < LAYOUT_TEXTBOX_ID_DUMMY_LAST &&
           model->textboxData.isEnabled[textboxId];
}

static int next_focusable_textbox(const LayoutModel *model, int startIndex, int direction)
{
    int index = startIndex;
    for (size_t i = 0; i < LAYOUT_TEXTBOX_ID_DUMMY_LAST; i++)
    {
        index = (index + direction + LAYOUT_TEXTBOX_ID_DUMMY_LAST) % LAYOUT_TEXTBOX_ID_DUMMY_LAST;
        if (textbox_focusable(model, index))
        {
            return index;
        }
    }
    return -1;
}

static void focus_textbox(LayoutModel *model, int textboxId, double now)
{
    model->textboxData.focusData.focusIndex = textboxId;
    if (textboxId >= 0)
    {
        LayoutTextboxBuffer *buffer = &model->textboxData.textboxBuffers[textboxId];
        buffer->cursorPosition = buffer->length;
        model->textboxData.focusData.focusStartTime = now;
    }
}

void LayoutModel_ApplyTextboxInput(LayoutModel *model, const GuiInputFrame *input)
{
    LayoutTextboxBuffer *buffer;
    if (model == NULL || input == NULL)
    {
        return;
    }

    if (!textbox_focusable(model, model->textboxData.focusData.focusIndex))
    {
        model->textboxData.focusData.focusIndex = -1;
    }

    if (model->textboxData.focusData.focusIndex >= 0)
    {
        buffer = &model->textboxData.textboxBuffers[model->textboxData.focusData.focusIndex];

        for (size_t k = 0; k < input->typedCharCount; k++)
        {
            int key = input->typedChars[k];
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

            if ((keyWithinBounds || validDecimalPoint) && buffer->length < LAYOUT_TEXTBOX_BUFFER_SIZE - 1)
            {
                size_t i = ++buffer->length;
                for (; i > buffer->cursorPosition; i--)
                {
                    buffer->chars[i] = buffer->chars[i - 1];
                }
                buffer->chars[buffer->cursorPosition] = (char)key;
                buffer->cursorPosition++;

                if (buffer->numberboxConfig.isNumberbox)
                {
                    if (key == '.')
                    {
                        if (buffer->length == 1)
                        {
                            strcpy(buffer->chars, "0.");
                            buffer->length = 2;
                            buffer->cursorPosition = 2;
                        }
                        if (buffer->cursorPosition != buffer->length)
                        {
                            int charsWritten = clamp_float_as_string(buffer->chars, buffer->numberboxConfig.min, buffer->numberboxConfig.max);
                            buffer->length = (size_t)charsWritten;
                            buffer->cursorPosition = (size_t)charsWritten;
                        }
                    }
                    else if (key == '0')
                    {
                        if (dotPosition == NULL)
                        {
                            int charsWritten = clamp_float_as_string(buffer->chars, buffer->numberboxConfig.min, buffer->numberboxConfig.max);
                            buffer->length = (size_t)charsWritten;
                            buffer->cursorPosition = (size_t)charsWritten;
                            model->textboxData.focusData.focusStartTime = input->now;
                            continue;
                        }

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

                        bool leadingZero = buffer->cursorPosition - 1 <= (size_t)(firstSigPosition - buffer->chars);
                        bool trailingZero = buffer->cursorPosition - 1 >= (size_t)(lastSigPosition - buffer->chars);

                        if (!(leadingZero || trailingZero))
                        {
                            int charsWritten = clamp_float_as_string(buffer->chars, buffer->numberboxConfig.min, buffer->numberboxConfig.max);
                            buffer->length = (size_t)charsWritten;
                            buffer->cursorPosition = (size_t)charsWritten;
                        }
                    }
                    else
                    {
                        int charsWritten = clamp_float_as_string(buffer->chars, buffer->numberboxConfig.min, buffer->numberboxConfig.max);
                        buffer->length = (size_t)charsWritten;
                        buffer->cursorPosition = (size_t)charsWritten;
                    }
                }

                model->textboxData.focusData.focusStartTime = input->now;
            }
        }

        bool nonZeroCursorPosition = buffer->cursorPosition > 0;
        bool nonMaxCursorPosition = buffer->cursorPosition < buffer->length;

        if (input_key(input, LAYOUT_KEY_BACKSPACE) && nonZeroCursorPosition)
        {
            int offset = 0;
            if (input->ctrlDown)
            {
                while ((int)buffer->cursorPosition - 1 - offset >= 0 &&
                       LayoutModel_CharMatchesAny(buffer->chars[buffer->cursorPosition - 1 - offset], " ./\\"))
                {
                    offset++;
                }
                while ((int)buffer->cursorPosition - 1 - offset >= 0 &&
                       !LayoutModel_CharMatchesAny(buffer->chars[buffer->cursorPosition - 1 - offset], " ./\\"))
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
                buffer->chars[i - (size_t)offset] = buffer->chars[i];
            }
            buffer->cursorPosition -= (size_t)offset;
            buffer->length -= (size_t)offset;
            buffer->chars[buffer->length] = '\0';
            model->textboxData.focusData.focusStartTime = input->now;

            if (buffer->numberboxConfig.isNumberbox)
            {
                if (buffer->length == 1 && buffer->chars[0] == '.')
                {
                    strcpy(buffer->chars, "0.");
                    buffer->length = 2;
                    buffer->cursorPosition += 1;
                }
                else if (buffer->length > 1)
                {
                    int charsWritten = clamp_float_as_string(buffer->chars, buffer->numberboxConfig.min, buffer->numberboxConfig.max);
                    buffer->length = (size_t)charsWritten;
                    if (buffer->cursorPosition > buffer->length)
                    {
                        buffer->cursorPosition = (size_t)charsWritten;
                    }
                }
            }
        }
        else if (input_key(input, LAYOUT_KEY_DELETE) && nonMaxCursorPosition)
        {
            int offset = 0;
            if (input->ctrlDown)
            {
                while ((int)buffer->cursorPosition + offset < (int)buffer->length &&
                       LayoutModel_CharMatchesAny(buffer->chars[buffer->cursorPosition + (size_t)offset], " ./\\"))
                {
                    offset++;
                }
                while ((int)buffer->cursorPosition + offset < (int)buffer->length &&
                       !LayoutModel_CharMatchesAny(buffer->chars[buffer->cursorPosition + (size_t)offset], " ./\\"))
                {
                    offset++;
                }
            }
            else
            {
                offset = 1;
            }

            for (size_t i = buffer->cursorPosition; i <= buffer->length - (size_t)offset; i++)
            {
                buffer->chars[i] = buffer->chars[i + (size_t)offset];
            }
            buffer->length -= (size_t)offset;
            buffer->chars[buffer->length] = '\0';
            model->textboxData.focusData.focusStartTime = input->now;

            if (buffer->numberboxConfig.isNumberbox)
            {
                if (buffer->length == 1 && buffer->chars[0] == '.')
                {
                    strcpy(buffer->chars, "0.");
                    buffer->length = 2;
                    buffer->cursorPosition += 1;
                }
                else if (buffer->length > 1)
                {
                    int charsWritten = clamp_float_as_string(buffer->chars, buffer->numberboxConfig.min, buffer->numberboxConfig.max);
                    buffer->length = (size_t)charsWritten;
                    if (buffer->cursorPosition > buffer->length)
                    {
                        buffer->cursorPosition = (size_t)charsWritten;
                    }
                }
            }
        }
        else if (input_key(input, LAYOUT_KEY_LEFT) && nonZeroCursorPosition)
        {
            if (input->ctrlDown)
            {
                while (buffer->cursorPosition > 0 && LayoutModel_CharMatchesAny(buffer->chars[buffer->cursorPosition - 1], " ./\\"))
                {
                    buffer->cursorPosition--;
                }
                while (buffer->cursorPosition > 0 && !LayoutModel_CharMatchesAny(buffer->chars[buffer->cursorPosition - 1], " ./\\"))
                {
                    buffer->cursorPosition--;
                }
            }
            else
            {
                buffer->cursorPosition--;
            }
            model->textboxData.focusData.focusStartTime = input->now;
        }
        else if (input_key(input, LAYOUT_KEY_RIGHT) && nonMaxCursorPosition)
        {
            if (input->ctrlDown)
            {
                while (buffer->cursorPosition < buffer->length && LayoutModel_CharMatchesAny(buffer->chars[buffer->cursorPosition], " ./\\"))
                {
                    buffer->cursorPosition++;
                }
                while (buffer->cursorPosition < buffer->length && !LayoutModel_CharMatchesAny(buffer->chars[buffer->cursorPosition], " ./\\"))
                {
                    buffer->cursorPosition++;
                }
            }
            else
            {
                buffer->cursorPosition++;
            }
            model->textboxData.focusData.focusStartTime = input->now;
        }
        else if (input_key(input, LAYOUT_KEY_TAB))
        {
            int direction = input->shiftDown ? -1 : 1;
            focus_textbox(model, next_focusable_textbox(model, model->textboxData.focusData.focusIndex, direction), input->now);
        }
    }
    else if (input->keyPressed[LAYOUT_KEY_TAB])
    {
        int direction = input->shiftDown ? -1 : 1;
        int startIndex = input->shiftDown ? 0 : -1;
        focus_textbox(model, next_focusable_textbox(model, startIndex, direction), input->now);
    }

    if (input->leftMouseReleased && !model->textboxData.focusData.focusRegistered)
    {
        model->textboxData.focusData.focusIndex = -1;
    }
}

int LayoutModel_ParseStreamsJson(LayoutModel *model, const char *json, size_t len, char *error, size_t errorCap)
{
    if (model == NULL || json == NULL)
    {
        return set_error(error, errorCap, "Input metadata is missing.");
    }

    cJSON *root = cJSON_ParseWithLength(json, len);
    if (root == NULL)
    {
        return set_error(error, errorCap, "Input metadata JSON could not be parsed.");
    }

    cJSON *streams = cJSON_GetObjectItemCaseSensitive(root, "streams");
    if (!cJSON_IsArray(streams))
    {
        cJSON_Delete(root);
        return set_error(error, errorCap, "Input metadata is missing streams.");
    }

    model->streamData.streamCounts[LAYOUT_STREAM_ID_VIDEO] = 0;
    model->streamData.streamCounts[LAYOUT_STREAM_ID_AUDIO] = 0;
    model->streamData.streamCounts[LAYOUT_STREAM_ID_SUBTITLES] = 0;
    model->streamData.inputDurationSeconds = -1.0;

    cJSON *stream = NULL;
    cJSON_ArrayForEach(stream, streams)
    {
        cJSON *codecType = cJSON_GetObjectItemCaseSensitive(stream, "codec_type");
        if (cJSON_IsString(codecType) && codecType->valuestring != NULL)
        {
            if (strcmp(codecType->valuestring, "video") == 0)
            {
                model->streamData.streamCounts[LAYOUT_STREAM_ID_VIDEO]++;
            }
            else if (strcmp(codecType->valuestring, "audio") == 0)
            {
                model->streamData.streamCounts[LAYOUT_STREAM_ID_AUDIO]++;
            }
            else if (strcmp(codecType->valuestring, "subtitle") == 0)
            {
                model->streamData.streamCounts[LAYOUT_STREAM_ID_SUBTITLES]++;
            }
        }
        else
        {
            cJSON_Delete(root);
            return set_error(error, errorCap, "Input metadata contains a stream without codec_type.");
        }
    }

    cJSON *format = cJSON_GetObjectItemCaseSensitive(root, "format");
    if (cJSON_IsObject(format))
    {
        cJSON *duration = cJSON_GetObjectItemCaseSensitive(format, "duration");
        if (cJSON_IsString(duration) && duration->valuestring != NULL)
        {
            double seconds = parse_seconds(duration->valuestring);
            if (seconds >= 0.0)
            {
                model->streamData.inputDurationSeconds = seconds;
            }
        }
        else if (cJSON_IsNumber(duration) && duration->valuedouble >= 0.0)
        {
            model->streamData.inputDurationSeconds = duration->valuedouble;
        }
    }

    cJSON_Delete(root);
    if (error != NULL && errorCap > 0)
    {
        error[0] = '\0';
    }
    return 0;
}

static bool output_ext_allowed(char outputType, const char *ext)
{
    if (ext == NULL)
    {
        return false;
    }
    switch (outputType)
    {
    case 'v':
        return LayoutModel_ExtensionEqualsIgnoreCase(ext, ".gif") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".mkv") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".mov") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".mp4") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".webm");
    case 'a':
        return LayoutModel_ExtensionEqualsIgnoreCase(ext, ".wav") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".wave") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".mp3") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".m4a") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".flac") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".ogg") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".oga") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".opus");
    case 'i':
        return LayoutModel_ExtensionEqualsIgnoreCase(ext, ".jpg") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".jpeg") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".jpe") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".jfif") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".png") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".tiff") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".tif") ||
               LayoutModel_ExtensionEqualsIgnoreCase(ext, ".webp");
    default:
        return false;
    }
}

static const char *default_output_ext(char outputType)
{
    switch (outputType)
    {
    case 'v':
        return ".gif";
    case 'a':
        return ".wav";
    case 'i':
        return ".jpg";
    default:
        return "";
    }
}

static bool default_platform_file_exists(const char *path, void *userData)
{
    (void)path;
    (void)userData;
    return false;
}

static bool default_platform_directory_exists(const char *path, void *userData)
{
    (void)path;
    (void)userData;
    return false;
}

static bool default_platform_filename_valid(const char *name, void *userData)
{
    (void)userData;
    return name != NULL && name[0] != '\0';
}

int LayoutModel_BuildConvertPlan(LayoutModel *model, const GuiPlatform *platform, ConvertPlan *out, char *error, size_t errorCap)
{
    bool (*fileExists)(const char *, void *) = platform != NULL && platform->fileExists != NULL ? platform->fileExists : default_platform_file_exists;
    bool (*directoryExists)(const char *, void *) = platform != NULL && platform->directoryExists != NULL ? platform->directoryExists : default_platform_directory_exists;
    bool (*isFileNameValid)(const char *, void *) = platform != NULL && platform->isFileNameValid != NULL ? platform->isFileNameValid : default_platform_filename_valid;
    void *userData = platform != NULL ? platform->userData : NULL;
    char outputDir[LAYOUT_TEXTBOX_BUFFER_SIZE];
    char outputName[LAYOUT_TEXTBOX_BUFFER_SIZE];
    const char *outputExt;
    char slash;
    char outputType;

    if (model == NULL || out == NULL)
    {
        return set_error(error, errorCap, "Internal error: conversion model is missing.");
    }
    memset(out, 0, sizeof(*out));
    model->streamData.convertOutputLength = 0;
    model->streamData.convertOutput[0] = '\0';

    if (!fileExists(model->streamData.inputPath, userData))
    {
        return set_error(error, errorCap, "The selected input file does not exist.");
    }

    const char *outputPath = LayoutModel_Trim(LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_OUTPUT_PATH));
    if (outputPath[0] == '\0')
    {
        return set_error(error, errorCap, "Please choose an output file path.");
    }
    if (!split_output_path(outputPath, outputDir, sizeof(outputDir), outputName, sizeof(outputName)))
    {
        return set_error(error, errorCap, "Internal error: unable to build the output file path.");
    }
    if (!directoryExists(outputDir, userData))
    {
        return set_error(error, errorCap, "The selected output directory does not exist.");
    }
    if (!isFileNameValid(outputName, userData))
    {
        return set_error(error, errorCap, "The output file name is invalid.");
    }

    outputType = LayoutModel_GetDropdownValue(model, LAYOUT_DROPDOWN_ID_OUTPUT_TYPE)[0];
    if (outputType != 'v' && outputType != 'a' && outputType != 'i')
    {
        return set_error(error, errorCap, "Unsupported output type selected.");
    }
    outputExt = get_file_extension(outputPath);
    if (!output_ext_allowed(outputType, outputExt))
    {
        outputExt = default_output_ext(outputType);
    }
    slash = path_slash_for_dir(outputDir);

    char *fullOutputPath = layout_argprintf("%s%c%s%s", outputDir, slash, outputName, outputExt);
    if (fullOutputPath == NULL)
    {
        return set_error(error, errorCap, "Internal error: unable to prepare output path.");
    }
    if (strcmp(model->streamData.inputPath, fullOutputPath) == 0)
    {
        free(fullOutputPath);
        return set_error(error, errorCap, "The input and output files cannot be the same.");
    }

    bool outputVideo = outputType == 'v';
    bool gifInput = LayoutModel_ExtensionEqualsIgnoreCase(get_file_extension(model->streamData.inputPath), ".gif");
    bool gifOutput = LayoutModel_ExtensionEqualsIgnoreCase(outputExt, ".gif");
    bool outputAudio = (outputType == 'a' || (outputVideo && !LayoutModel_ExtensionEqualsIgnoreCase(outputExt, ".gif"))) &&
                       model->streamData.streamCounts[LAYOUT_STREAM_ID_AUDIO] > 0;
    bool outputImage = outputType == 'i';
    bool burnSubtitles = LayoutModel_GetDropdownValue(model, LAYOUT_DROPDOWN_ID_SUBTITLES)[0] != '\0';

    if (burnSubtitles)
    {
        const char *subtitlePath = LayoutModel_Trim(LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_SUBTITLES_SOURCE));
        if (platform != NULL && platform->validateSubtitleFile != NULL)
        {
            int ret = platform->validateSubtitleFile(subtitlePath, userData);
            if (ret != 0)
            {
                free(fullOutputPath);
                return set_error(error, errorCap, "The subtitle file could not be validated.");
            }
        }
    }

    LayoutStringBuilder sb;
    sb_init(&sb);
    bool ok = true;
    if (outputVideo)
    {
        ok = ok && sb_appendf(
                       &sb,
                       "[0:v]fps=%s,trim=%s:%s,setpts=(PTS-STARTPTS)/%s,crop=%s:%s:%s:%s,scale=%s:%s%s%s%s%s[out_v];",
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_FPS),
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_DURATION_START_VIDEO),
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_DURATION_END_VIDEO)[0] == 'e' ? "" : LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_DURATION_END_VIDEO),
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_SPEED_VIDEO),
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_CROP_W),
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_CROP_H),
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_CROP_X),
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_CROP_Y),
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_SCALE_W),
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_SCALE_H),
                       !burnSubtitles ? "" : ",subtitles='",
                       !burnSubtitles ? "" : LayoutModel_Trim(LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_SUBTITLES_SOURCE)),
                       !burnSubtitles ? "" : "'",
                       !gifInput ? "" : ",format=yuv420p");
    }
    if (outputAudio)
    {
        bool channelLayout = LayoutModel_GetDropdownValue(model, LAYOUT_DROPDOWN_ID_CHANNEL_LAYOUT)[0] != '\0';
        ok = ok && sb_appendf(
                       &sb,
                       "[0:a]atrim=%s:%s,",
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_DURATION_START_AUDIO),
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_DURATION_END_AUDIO)[0] == 'e' ? "" : LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_DURATION_END_AUDIO));
        float multiplier = (float)atof(LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_SPEED_AUDIO));
        while (multiplier < 0.5f)
        {
            ok = ok && sb_append(&sb, "atempo=0.5,");
            multiplier *= 2.0f;
        }
        ok = ok && sb_appendf(&sb, "atempo=%f,", multiplier);
        ok = ok && sb_appendf(
                       &sb,
                       "adelay=%s:1%s,aformat=%s%s[out_a]",
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_DELAY),
                       LayoutModel_GetDropdownValue(model, LAYOUT_DROPDOWN_ID_LOUDNORM_ENABLE)[0] == '\0' ? "" : ",loudnorm",
                       !channelLayout ? "" : "channel_layouts=",
                       !channelLayout ? "" : LayoutModel_GetDropdownValue(model, LAYOUT_DROPDOWN_ID_CHANNEL_LAYOUT));
    }
    if (outputImage)
    {
        ok = ok && sb_appendf(
                       &sb,
                       "[0:v]crop=%s:%s:%s:%s,scale=%s:%s[out_v]",
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_CROP_W),
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_CROP_H),
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_CROP_X),
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_CROP_Y),
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_SCALE_W),
                       LayoutModel_GetTextboxValue(model, LAYOUT_TEXTBOX_ID_SCALE_H));
    }
    if (!ok)
    {
        free(sb.buffer);
        free(fullOutputPath);
        return set_error(error, errorCap, "Internal error: unable to build conversion filters.");
    }

    LayoutArgvBuilder a;
    argv_init(&a);
    ok = argv_push(&a, "ffmpeg") &&
         argv_push(&a, "-v") &&
         argv_push(&a, "error") &&
         argv_push(&a, "-progress") &&
         argv_push(&a, "pipe:1") &&
         argv_push(&a, "-y") &&
         argv_push(&a, "-i") &&
         argv_push(&a, model->streamData.inputPath) &&
         argv_push(&a, "-filter_complex") &&
         argv_push(&a, sb.buffer);
    if (outputVideo || outputImage)
    {
        ok = ok && argv_push(&a, "-map") && argv_push(&a, "[out_v]");
    }
    if (outputVideo && !gifOutput)
    {
        ok = ok && argv_push(&a, "-c:v") && argv_push(&a, "libx264") && argv_push(&a, "-movflags") && argv_push(&a, "+faststart");
    }
    if (outputAudio)
    {
        ok = ok && argv_push(&a, "-map") && argv_push(&a, "[out_a]");
    }
    if (outputImage)
    {
        ok = ok && argv_push(&a, "-vframes") && argv_push(&a, "1");
    }
    ok = ok && argv_push(&a, fullOutputPath);
    if (!ok)
    {
        argv_free(&a);
        free(sb.buffer);
        free(fullOutputPath);
        return set_error(error, errorCap, "Internal error: unable to build ffmpeg arguments.");
    }

    out->argv = a.v;
    out->argc = a.len;
    out->filterGraph = sb.buffer;
    out->outputPath = fullOutputPath;
    if (error != NULL && errorCap > 0)
    {
        error[0] = '\0';
    }
    return 0;
}

void LayoutModel_FreeConvertPlan(ConvertPlan *plan)
{
    if (plan == NULL)
    {
        return;
    }
    if (plan->argv != NULL)
    {
        for (size_t i = 0; plan->argv[i] != NULL; i++)
        {
            free(plan->argv[i]);
        }
    }
    free(plan->argv);
    free(plan->filterGraph);
    free(plan->outputPath);
    memset(plan, 0, sizeof(*plan));
}

double LayoutModel_EstimateConvertDuration(const LayoutModel *model)
{
    if (model == NULL)
    {
        return -1.0;
    }

    char outputType = LayoutModel_GetDropdownValue(model, LAYOUT_DROPDOWN_ID_OUTPUT_TYPE)[0];
    if (outputType == 'i')
    {
        return -1.0;
    }

    double best = -1.0;
    if (outputType == 'v')
    {
        double videoDuration = estimate_trimmed_duration(
            model,
            LAYOUT_TEXTBOX_ID_DURATION_START_VIDEO,
            LAYOUT_TEXTBOX_ID_DURATION_END_VIDEO,
            LAYOUT_TEXTBOX_ID_SPEED_VIDEO);
        if (videoDuration >= 0.0)
        {
            best = videoDuration;
        }
    }

    bool includeAudio = (outputType == 'a' || outputType == 'v') &&
                        model->streamData.streamCounts[LAYOUT_STREAM_ID_AUDIO] > 0;
    if (includeAudio)
    {
        double audioDuration = estimate_trimmed_duration(
            model,
            LAYOUT_TEXTBOX_ID_DURATION_START_AUDIO,
            LAYOUT_TEXTBOX_ID_DURATION_END_AUDIO,
            LAYOUT_TEXTBOX_ID_SPEED_AUDIO);
        if (audioDuration >= 0.0 && audioDuration > best)
        {
            best = audioDuration;
        }
    }

    return best > 0.0 ? best : -1.0;
}

void LayoutModel_ResetConvertProgress(LayoutModel *model)
{
    if (model == NULL)
    {
        return;
    }
    model->streamData.progress = (LayoutConvertProgress){0};
    model->streamData.progress.estimatedDurationSeconds = -1.0;
    model->streamData.progressPartialLine[0] = '\0';
    model->streamData.progressPartialLineLength = 0;
}

void LayoutModel_BeginConvertProgress(LayoutModel *model)
{
    if (model == NULL)
    {
        return;
    }
    LayoutModel_ResetConvertProgress(model);
    model->streamData.progress.active = true;
    model->streamData.progress.estimatedDurationSeconds = LayoutModel_EstimateConvertDuration(model);
    model->streamData.progress.hasPercent = model->streamData.progress.estimatedDurationSeconds > 0.0;
    snprintf(model->streamData.progress.status, sizeof(model->streamData.progress.status), "%s", "continue");
}

static void update_progress_percent(LayoutModel *model)
{
    LayoutConvertProgress *progress = &model->streamData.progress;
    if (progress->estimatedDurationSeconds > 0.0)
    {
        progress->hasPercent = true;
        progress->percent = progress->outTimeSeconds / progress->estimatedDurationSeconds;
        if (progress->percent < 0.0)
        {
            progress->percent = 0.0;
        }
        if (progress->percent > 1.0)
        {
            progress->percent = 1.0;
        }
    }
    else
    {
        progress->hasPercent = false;
        progress->percent = 0.0;
    }
}

bool LayoutModel_ParseProgressLine(LayoutModel *model, const char *line)
{
    if (model == NULL || line == NULL)
    {
        return false;
    }

    const char *equals = strchr(line, '=');
    if (equals == NULL)
    {
        return false;
    }

    size_t keyLength = (size_t)(equals - line);
    const char *value = equals + 1;
    LayoutConvertProgress *progress = &model->streamData.progress;

    if (keyLength == 5 && strncmp(line, "frame", keyLength) == 0)
    {
        progress->frame = atoi(value);
    }
    else if (keyLength == 3 && strncmp(line, "fps", keyLength) == 0)
    {
        progress->fps = parse_seconds(value);
    }
    else if (keyLength == 7 && strncmp(line, "bitrate", keyLength) == 0)
    {
        snprintf(progress->bitrate, sizeof(progress->bitrate), "%s", value);
    }
    else if (keyLength == 5 && strncmp(line, "speed", keyLength) == 0)
    {
        char speedBuffer[32];
        snprintf(speedBuffer, sizeof(speedBuffer), "%s", value);
        char *x = strchr(speedBuffer, 'x');
        if (x != NULL)
        {
            *x = '\0';
        }
        progress->speed = parse_seconds(speedBuffer);
    }
    else if ((keyLength == 11 && strncmp(line, "out_time_us", keyLength) == 0) ||
             (keyLength == 11 && strncmp(line, "out_time_ms", keyLength) == 0))
    {
        double units = parse_seconds(value);
        if (units >= 0.0)
        {
            progress->outTimeSeconds = units / 1000000.0;
            update_progress_percent(model);
        }
    }
    else if (keyLength == 8 && strncmp(line, "out_time", keyLength) == 0)
    {
        double seconds = parse_timecode_seconds(value);
        if (seconds >= 0.0)
        {
            progress->outTimeSeconds = seconds;
            update_progress_percent(model);
        }
    }
    else if (keyLength == 8 && strncmp(line, "progress", keyLength) == 0)
    {
        snprintf(progress->status, sizeof(progress->status), "%s", value);
        if (strcmp(value, "end") == 0)
        {
            progress->finished = true;
            progress->active = false;
            if (progress->hasPercent)
            {
                progress->percent = 1.0;
            }
        }
    }
    else
    {
        return false;
    }

    return true;
}

void LayoutModel_AppendConvertOutput(LayoutModel *model, const char *data, size_t dataLength)
{
    if (model == NULL || data == NULL || dataLength == 0)
    {
        return;
    }

    for (size_t i = 0; i < dataLength; i++)
    {
        char c = data[i];
        if (c == '\r')
        {
            continue;
        }

        if (c == '\n')
        {
            model->streamData.progressPartialLine[model->streamData.progressPartialLineLength] = '\0';
            if (model->streamData.progressPartialLineLength > 0 &&
                !LayoutModel_ParseProgressLine(model, model->streamData.progressPartialLine))
            {
                append_convert_error_line(model, model->streamData.progressPartialLine);
            }
            model->streamData.progressPartialLineLength = 0;
            model->streamData.progressPartialLine[0] = '\0';
            continue;
        }

        if (model->streamData.progressPartialLineLength + 1 < sizeof(model->streamData.progressPartialLine))
        {
            model->streamData.progressPartialLine[model->streamData.progressPartialLineLength++] = c;
        }
        else
        {
            append_convert_error_output(model, model->streamData.progressPartialLine, model->streamData.progressPartialLineLength);
            append_convert_error_output(model, &c, 1);
            model->streamData.progressPartialLineLength = 0;
            model->streamData.progressPartialLine[0] = '\0';
        }
    }
}

const char *LayoutModel_SanitizeErrorMessage(const char *message)
{
    if (message == NULL || message[0] == '\0')
    {
        return "An unexpected error occurred.";
    }

    static char buffer[256];
    size_t out = 0;
    const char *p = message;

    while (*p != '\0' && out + 1 < sizeof(buffer))
    {
        if (*p == '\r')
        {
            p++;
            continue;
        }
        if (*p == '\n')
        {
            break;
        }
        if (*p == '\x1b')
        {
            while (*p != '\0' && *p != 'm')
            {
                p++;
            }
            if (*p == 'm')
            {
                p++;
            }
            continue;
        }
        buffer[out++] = *p;
        p++;
    }
    buffer[out] = '\0';

    char *trimmed = buffer;
    while (*trimmed == ' ' || *trimmed == '\t')
    {
        trimmed++;
    }

    if (strstr(trimmed, "height") != NULL && strstr(trimmed, "divisible by 2") != NULL)
    {
        return "The selected output height must be divisible by 2 for this conversion.";
    }
    if (strstr(trimmed, "Invalid argument") != NULL)
    {
        return "The requested conversion settings could not be applied.";
    }
    if (strstr(trimmed, "Error") != NULL || strstr(trimmed, "error") != NULL)
    {
        return trimmed;
    }

    return trimmed;
}

void LayoutModel_SelectDropdownHoveredOption(LayoutModel *model, LayoutDropdownID dropdownId)
{
    model->dropdownData.selectedOptions[dropdownId] = model->dropdownData.hoveredOption;
    model->dropdownData.selectedValues[dropdownId] = model->dropdownData.hoveredValue;
}

void LayoutModel_SelectTab(LayoutModel *model, LayoutTabID tabId)
{
    if (!model->tabData.isDisabled[tabId])
    {
        model->tabData.selectedTab = tabId;
    }
}

void LayoutModel_PreviewSizeDown(LayoutModel *model)
{
    size_t size = model->previewImageData.imageSize;
    size_t min = model->previewImageData.imageSizeMin;
    model->previewImageData.imageSize = size < min + 100 ? min : size - 100;
}

void LayoutModel_PreviewSizeUp(LayoutModel *model)
{
    size_t size = model->previewImageData.imageSize;
    size_t max = model->previewImageData.imageSizeMax;
    model->previewImageData.imageSize = size > max - 100 ? max : size + 100;
}

void LayoutModel_RemoveErrorPopup(LayoutModel *model, size_t index)
{
    if (index >= model->errorPopupData.count)
    {
        return;
    }
    for (size_t i = index; i + 1 < model->errorPopupData.count; i++)
    {
        model->errorPopupData.popups[i] = model->errorPopupData.popups[i + 1];
    }
    model->errorPopupData.count--;
}
