#include "ConsoleClient.h"
#include "ConsoleDetect.h"
#include "ConsoleVar.h"

#include <Base/CmdLine.h>
#include <Base/Coordinate.h>
#include <Base/Handle.h>
#include <DB/DBClient/AutoCode/VideoHardwareRec.h>
#include <Event/EvtApi.h>
#include <Gx/Gx.h>
#include <Gxu/IGxuFont.h>
#include <Os/W32/OsClipboard.h>
#include <WorldClient/WorldParam.h>
#include <storm.h>
#include <stpl.h>

#include <math.h>
#include <malloc.h>
#include <new>
#include <stdio.h>
#include <string.h>
#include <typeinfo>
#include <windows.h>

struct HLAYER__;
struct HTEXTFONT__;

enum HIGHLIGHTSTATE {
  HS_NONE = 0,
  HS_HIGHLIGHTING = 1,
  HS_ENDHIGHLIGHT = 2
};

enum CONSOLERESIZESTATE {
  CS_NONE = 0,
  CS_STRETCH = 1
};

struct CONSOLELINE : public TSLinkedNode<CONSOLELINE> {
  char        *buffer;
  unsigned int chars;
  unsigned int charsalloc;
  unsigned int inputpos;
  unsigned int inputstart;
  COLOR_T      colorType;
  CGxString   *fontPointer;
};

CGxFont *__fastcall     TextBlockGetFontPtr(HTEXTFONT__ *fontHandle);
HTEXTFONT__ *__fastcall TextBlockGenerateFont(const char *fontName, unsigned int fontFlags, float fontHeight);

static const char                                  *formatToInt[CGxFormat::Formats_Last] = {"16", "24", "24", "30", "16", "24", "24", "32"};
static int                                          s_rates[11] = {60, 70, 72, 75, 85, 90, 100, 120, 160, 180, 200};
static const char                                   s_apis[2][16] = {"OpenGL", "Direct3D"};
static unsigned int                                 s_FormatTobpp[4] = {16, 32, 32, 32};
static float                                        carettime;
static CGxDevice                                   *s_device;
static int                                          s_active;
static CVar                                        *s_cvGxColorBits;
static SCritSect                                    s_critsect;
static CVar                                        *s_cvGxDepthBits;
static CVar                                        *s_cvGxResolution;
static CVar                                        *s_cvGxRefresh;
static CVar                                        *s_cvGxApi;
static CVar                                        *s_cvGxVSync;
static CVar                                        *s_cvGxWindow;
static CVar                                        *s_cvHwDetect;
static RECTF                                        s_rect = {0.0f, 0.0f, 1.0f, 1.0f};
static float                                        s_caretpixwidth;
static float                                        s_caretpixheight;
static int                                          s_caret;
static TSList<CONSOLELINE, TSGetLink<CONSOLELINE> > s_linelist;
static CONSOLELINE                                 *s_currlineptr;
static int                                          s_NumLines;
static HLAYER__                                    *s_layerBackground;
static HLAYER__                                    *s_layerText;
static int                                          s_completionMode;
static const char                                  *s_completedCmd;
static const char                                  *s_previousCmd;
static char                                         s_partial[0x100];
static int                                          s_historyIndex = -1;
static NTempest::CImVector                          s_colorArray[NUM_COLORTYPES] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFF808080, 0xFFFF0000, 0xFFFFFF00,
                                                                                    0xFFFFFFFF, 0xFFFFFFFF, 0x80FFFFFF, 0xC0000000};
static float                                        s_fontHeight = 0.02f;
static HTEXTFONT__                                 *s_textFont;
static char                                         s_fontName[0x104];
static CONSOLERESIZESTATE                           s_consoleResizeState;
static HIGHLIGHTSTATE                               s_highlightState;
static float                                        s_consoleLines = 10.0f;
static float                                        s_consoleHeight;
static char                                         s_repeatBuffer[0x20];
static unsigned int                                 s_repeatCount;
static RECTF                                        s_hRect = {0.0f, 0.0f, 1.0f, 1.0f};
static char                                         s_copyText[0x80];
static float                                        s_highlightHStart;
static float                                        s_highlightHEnd;
static unsigned int                                 s_highlightLeftCharIndex;
static unsigned int                                 s_highlightRightCharIndex;
static float                                        s_charSpacing;
static unsigned int                                 s_baseTextFlags = 8;

static int __fastcall ConsoleCommand_FontColor(const char *cmd, const char *arguments);
static int __fastcall ConsoleCommand_BackGroundColor(const char *cmd, const char *arguments);
static int __fastcall ConsoleCommand_HighLightColor(const char *cmd, const char *arguments);
static int __fastcall ConsoleCommand_FontSize(const char *cmd, const char *arguments);
static int __fastcall ConsoleCommand_Font(const char *cmd, const char *arguments);
static int __fastcall ConsoleCommand_BufferSize(const char *cmd, const char *arguments);
static int __fastcall ConsoleCommand_ClearConsole(const char *cmd, const char *arguments);
static int __fastcall ConsoleCommand_Proportional(const char *cmd, const char *args);
static int __fastcall ConsoleCommand_CharSpacing(const char *cmd, const char *args);
static int __fastcall ConsoleCommand_CurrentSettings(const char *cmd, const char *arguments);
static int __fastcall ConsoleCommand_DefaultSettings(const char *cmd, const char *arguments);
static int __fastcall ConsoleCommand_CloseConsole(const char *cmd, const char *arguments);
static int __fastcall ConsoleCommand_RepeatHandler(const char *cmd, const char *arguments);

static const char *s_consoleCommands[13] = {"fontcolor",        "bgcolor", "highlightcolor", "fontsize", "font",         "consolelines", "clear",
                                            "proportionaltext", "spacing", "settings",       "default",  "closeconsole", "repeat"};
static const char *s_consoleHelpText[13] = {
    "[ColorClassName] [Red 0-255] [Green 0-255] [Blue 0-255]",
    "[alpha 0-255] [Red 0-255] [Green 0-255] [Blue 0-255]",
    "[alpha 0-255] [Red 0-255] [Green 0-255] [Blue 0-255]",
    "[15-50] arbitrary font size",
    "[fontname] make sure to use the .ttf file name",
    "[number] number of lines to show in the console",
    "Clears the console buffer",
    "Toggles fixed-width text characters",
    "[float] specifies inter-character spacing, in pixels",
    "Shows current font and console settings",
    "Resets all the font and console settings",
    "Closes the Console window",
    "Repeats a command"
};
static Hardware        s_hardware;
static DefaultSettings s_defaults;
static CGxFormat       s_fallbackFormat(false, NTempest::C2iVector(640, 480), CGxFormat::Fmt_Rgb565, CGxFormat::Fmt_Ds160, 60, true, true, false);
static CGxFormat       s_desktopFormat(false, NTempest::C2iVector(800, 600), CGxFormat::Fmt_ArgbX888, CGxFormat::Fmt_Ds24X, 60, true, true, false);
static CGxFormat       s_requestedFormat;
static CGxFormat       s_lastGoodFormat;
static CONSOLECOMMANDHANDLER const s_commandHandlers[13] = {
    ConsoleCommand_FontColor,    ConsoleCommand_BackGroundColor, ConsoleCommand_HighLightColor,  ConsoleCommand_FontSize,
    ConsoleCommand_Font,         ConsoleCommand_BufferSize,      ConsoleCommand_ClearConsole,    ConsoleCommand_Proportional,
    ConsoleCommand_CharSpacing,  ConsoleCommand_CurrentSettings, ConsoleCommand_DefaultSettings, ConsoleCommand_CloseConsole,
    ConsoleCommand_RepeatHandler
};
static TSGrowableArray<CGxMonitorMode> s_gxMonitorModes;

static void GenerateNodeString(CONSOLELINE *node);
static void RegenerateFontStrings();
static void EnforceMaxLines();

void __fastcall OsGuiSetWindowTitle(void *inWindow, const char *inText);

typedef void(__fastcall *SCREENPAINTFUNC)(void *, const RECTF *, const RECTF *, float);

void __fastcall ScrnLayerCreate(const RECTF *rect, float zorder, unsigned long flags, void *param, SCREENPAINTFUNC paintfunc, HLAYER__ **layer);
void __fastcall ScrnLayerSetRect(HLAYER__ *layer, const RECTF *rect);
void __fastcall ScrnPerfEnable(int enable);

static void __fastcall PaintBackground(void *, const RECTF *, const RECTF *, float);
static void __fastcall PaintText(void *, const RECTF *, const RECTF *, float elapsedSec);
static CONSOLELINE    *GetInputLine();
static void            ReserveInputSpace(CONSOLELINE *lineptr, unsigned long chars);
static void            PasteInInputLine(const char *characters);
static void            ResetHighlight();
static void            UpdateHighlight();

static int __fastcall OnChar(const EVENT_DATA_CHAR *data, void *__formal);
static int __fastcall OnIdle(const EVENT_DATA_IDLE *data, void *__formal);
static int __fastcall OnKeyDown(const EVENT_DATA_KEY *data, void *__formal);
static int __fastcall OnKeyDownRepeat(const EVENT_DATA_KEY *data, void *__formal);
static int __fastcall OnKeyUp(const EVENT_DATA_KEY *data, void *__formal);
static int __fastcall OnMouseDown(const EVENT_DATA_MOUSE *data, void *__formal);
static int __fastcall OnMouseUp(const EVENT_DATA_MOUSE *data, void *__formal);
static int __fastcall OnMouseMove(const EVENT_DATA_MOUSE *data, void *__formal);

long __fastcall OsWindowProc(void *_window, unsigned int message, unsigned int wparam, long lparam);
void __fastcall OsGuiSetGxWindow(void *window);

static bool __fastcall CVGxResolutionCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static bool __fastcall CVGxColorBitsCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static bool __fastcall CVGxDepthBitsCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static bool __fastcall CVGxRefreshCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static bool __fastcall CVGxApiCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static bool __fastcall CVGxVSyncCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static bool __fastcall CVGxWindowCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static void            ValidateFormatMonitor(CGxFormat &fmt);

static void SetColor(COLOR_T colorType, NTempest::CImVector color) {
  if (colorType < NUM_COLORTYPES) {
    s_colorArray[colorType] = color;
  }
}

static void GenerateNodeString(CONSOLELINE *node) {
  NTempest::C3Vector position(0.0f);
  CGxFont           *font = TextBlockGetFontPtr(s_textFont);

  if (!font || !node || !node->buffer || !node->buffer[0]) {
    return;
  }

  if (node->fontPointer) {
    GxuFontDestroyString(node->fontPointer);
  }

  GxuFontCreateString(
      font, node->buffer, s_fontHeight, position, 1.0f, s_fontHeight, 0.0f, node->fontPointer, GxVJ_Middle, GxHJ_Left, s_baseTextFlags,
      s_colorArray[node->colorType], s_charSpacing
  );
}

static void EnforceMaxLines() {
  CONSOLELINE *line;

  if (s_NumLines <= 256) {
    return;
  }

  line = s_linelist.Tail();
  if (line) {
    FREEIFUSED(line->buffer);
    if (line->fontPointer) {
      GxuFontDestroyString(line->fontPointer);
    }
    s_linelist.DeleteNode(line);
    --s_NumLines;
  }
}

static void DrawBackground() {
  NTempest::C3Vector position[4];
  unsigned short     indices[4] = {0, 1, 2, 3};

  position[0].x = s_rect.left;
  position[0].y = s_rect.bottom;
  position[0].z = 0.0f;
  position[1].x = s_rect.right;
  position[1].y = s_rect.bottom;
  position[1].z = 0.0f;
  position[2].x = s_rect.left;
  position[2].y = s_rect.top;
  position[2].z = 0.0f;
  position[3].x = s_rect.right;
  position[3].y = s_rect.top;
  position[3].z = 0.0f;

  GxRsPush();
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Fog, 0);
  GxRsSet(GxRs_DepthTest, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_PolygonOffset, 0.0f);
  GxRsSet(GxRs_Blend, 2);
  GxVertexShaderSelect(GxVS_PassThru);
  GxPrimLockVertexPtrs(4, position, sizeof(NTempest::C3Vector), 0, 0, &s_colorArray[BACKGROUND_COLOR], 0, 0, 0, 0, 0, 0, 0);
  GxPrimDrawElements(GxPrim_TriangleStrip, 4, indices);
  GxPrimUnlockVertexPtrs();
  GxRsPop();
}

static void DrawHighLight() {
  NTempest::C3Vector position[4];
  unsigned short     indices[4] = {0, 1, 2, 3};

  position[0].x = s_hRect.left;
  position[0].y = s_hRect.bottom;
  position[0].z = 0.0f;
  position[1].x = s_hRect.right;
  position[1].y = s_hRect.bottom;
  position[1].z = 0.0f;
  position[2].x = s_hRect.left;
  position[2].y = s_hRect.top;
  position[2].z = 0.0f;
  position[3].x = s_hRect.right;
  position[3].y = s_hRect.top;
  position[3].z = 0.0f;

  GxRsPush();
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Blend, 2);
  GxVertexShaderSelect(GxVS_PassThru);
  GxPrimLockVertexPtrs(4, position, sizeof(NTempest::C3Vector), 0, 0, &s_colorArray[HIGHLIGHT_COLOR], 0, 0, 0, 0, 0, 0, 0);
  GxPrimDrawElements(GxPrim_TriangleStrip, 4, indices);
  GxPrimUnlockVertexPtrs();
  GxRsPop();
}

static void DrawCaret(const NTempest::C3Vector &caretpos) {
  NTempest::C3Vector position[4];
  unsigned short     indices[4] = {0, 1, 2, 3};

  position[0].x = caretpos.x;
  position[0].y = caretpos.y;
  position[0].z = 0.0f;
  position[1].x = caretpos.x + s_caretpixwidth * 2.0f;
  position[1].y = caretpos.y;
  position[1].z = 0.0f;
  position[2].x = caretpos.x;
  position[2].y = caretpos.y + s_fontHeight;
  position[2].z = 0.0f;
  position[3].x = caretpos.x + s_caretpixwidth * 2.0f;
  position[3].y = caretpos.y + s_fontHeight;
  position[3].z = 0.0f;

  GxRsPush();
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Fog, 0);
  GxRsSet(GxRs_DepthTest, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_PolygonOffset, 0.0f);
  GxRsSet(GxRs_Blend, 2);
  GxVertexShaderSelect(GxVS_PassThru);
  GxPrimLockVertexPtrs(4, position, sizeof(NTempest::C3Vector), 0, 0, &s_colorArray[INPUT_COLOR], 0, 0, 0, 0, 0, 0, 0);
  GxPrimDrawElements(GxPrim_TriangleStrip, 4, indices);
  GxPrimUnlockVertexPtrs();
  GxRsPop();
}

static void __fastcall PaintBackground(void *, const RECTF *, const RECTF *, float) {
  if (s_rect.bottom < 1.0f) {
    DrawBackground();
    if (s_highlightState != HS_NONE) {
      DrawHighLight();
    }
  }
}

static CONSOLELINE *GetInputLine() {
  CONSOLELINE *line = s_linelist.Head();

  if (!line || !line->inputpos) {
    line = (CONSOLELINE *)SMemAlloc(sizeof(CONSOLELINE), typeid(CONSOLELINE).raw_name(), SERR_LINECODE_OBJECT, SMEM_FLAG_ZEROMEMORY);
    if (line) {
      new (line) CONSOLELINE;
    }
    s_linelist.LinkNode(line, 1, 0);
    line->charsalloc = 16;
    line->buffer = (char *)ALLOC(16);
    SStrCopy(line->buffer, "> ", line->charsalloc);
    line->chars = SStrLen(line->buffer);
    line->inputpos = line->chars;
    line->inputstart = line->chars;
    line->colorType = INPUT_COLOR;
    s_currlineptr = line;
    ++s_NumLines;
    EnforceMaxLines();
  }
  return line;
}

static void __fastcall PaintText(void *, const RECTF *, const RECTF *, float elapsedSec) {
  CONSOLELINE       *inputLine;
  CONSOLELINE       *line;
  CGxFont           *font;
  NTempest::C3Vector caretpos;
  NTempest::C3Vector pos;

  if (s_rect.bottom >= 1.0f) {
    return;
  }
  carettime += elapsedSec;
  if ((s_caret && carettime >= 0.3f) || (!s_caret && carettime >= 0.2f)) {
    carettime = 0.0f;
    s_caret = !s_caret;
  }

  inputLine = GetInputLine();
  pos.x = s_rect.left;
  pos.y = s_rect.bottom + s_fontHeight * 0.75f;
  pos.z = -0.9f;
  font = TextBlockGetFontPtr(s_textFont);
  GxuFontRenderString(
      font, inputLine->buffer, s_fontHeight, pos, s_colorArray[INPUT_COLOR], 1.0f, s_fontHeight, GxVJ_Middle, GxHJ_Left, s_baseTextFlags, 0.0f,
      s_charSpacing
  );
  if (inputLine->inputpos) {
    caretpos = pos;
    GxuFontGetTextExtent(font, inputLine->buffer, inputLine->inputpos, s_fontHeight, &caretpos.x, s_charSpacing, s_baseTextFlags);
    DrawCaret(caretpos);
  }

  line = s_currlineptr;
  pos.y += s_fontHeight;
  while (line && pos.y < 1.0f) {
    if (line != inputLine) {
      GxuFontSetStringPosition(line->fontPointer, pos);
      GxuFontAddToInternalBatch(line->fontPointer);
      pos.y += s_fontHeight;
    }
    line = s_linelist.Next(line);
  }
  GxuFontRenderInternalBatch();
}

static int __fastcall OnIdle(const EVENT_DATA_IDLE *data, void *__formal) {
  float finalPos;

  if (s_active) {
    finalPos = 1.0f - s_consoleHeight;
    if (finalPos > 1.0f) {
      finalPos = 1.0f;
    } else if (finalPos <= 0.0f) {
      finalPos = 0.0f;
    }
  } else {
    finalPos = 1.0f;
  }

  if (s_repeatCount && s_repeatBuffer[0]) {
    ConsoleCommandExecute(s_repeatBuffer, 0);
    --s_repeatCount;
  }

  if (s_rect.bottom != finalPos) {
    if (s_consoleResizeState == CS_STRETCH) {
      s_rect.bottom = finalPos;
    } else {
      s_rect.bottom += (s_rect.bottom <= finalPos ? 1.0f : -1.0f) * data->elapsedSec * 5.0f;
      if (s_active) {
        if (s_rect.bottom < finalPos) {
          s_rect.bottom = finalPos;
        }
      } else if (s_rect.bottom > finalPos) {
        s_rect.bottom = finalPos;
      }
    }
    ScrnLayerSetRect(s_layerBackground, &s_rect);
    ScrnLayerSetRect(s_layerText, &s_rect);
  }
  return 1;
}

static void ReserveInputSpace(CONSOLELINE *lineptr, unsigned long chars) {
  unsigned int required = lineptr->chars + chars;
  char        *buffer;

  if (required < lineptr->charsalloc) {
    return;
  }
  do {
    lineptr->charsalloc += 16;
  } while (required >= lineptr->charsalloc);

  buffer = (char *)ALLOC(lineptr->charsalloc);
  SStrCopy(buffer, lineptr->buffer, lineptr->charsalloc);
  FREE(lineptr->buffer);
  lineptr->buffer = buffer;
}

static CONSOLELINE *GetLineAtMousePosition(float y) {
  int          lineNumber = (int)((s_consoleHeight - (1.0f - y)) / s_fontHeight);
  CONSOLELINE *head = s_linelist.Head();
  CONSOLELINE *line;

  if (lineNumber == 1) {
    return head;
  }

  line = s_currlineptr;
  if (s_currlineptr != head) {
    --lineNumber;
  }
  if (!line) {
    return 0;
  }
  while (lineNumber > 1) {
    --lineNumber;
    line = s_linelist.Next(line);
    if (!line) {
      return 0;
    }
  }
  return line;
}

static void PasteInInputLine(const char *characters) {
  unsigned int length = SStrLen(characters);
  CONSOLELINE *inputLine;
  char        *tail;
  char        *buffer;

  if (!length) {
    return;
  }
  inputLine = GetInputLine();
  ReserveInputSpace(inputLine, length);
  if (inputLine->inputpos < inputLine->chars) {
    if (length <= 1) {
      memmove(&inputLine->buffer[inputLine->inputpos + 1], &inputLine->buffer[inputLine->inputpos], inputLine->chars - inputLine->inputpos + 1);
      inputLine->buffer[inputLine->inputpos] = *characters;
      ++inputLine->inputpos;
      ++inputLine->chars;
    } else {
      tail = (char *)ALLOC(inputLine->charsalloc);
      SStrCopy(tail, &inputLine->buffer[inputLine->inputpos], 0x7FFFFFFF);
      buffer = (char *)ALLOC(inputLine->charsalloc);
      SStrCopy(buffer, inputLine->buffer, 0x7FFFFFFF);
      buffer[inputLine->inputpos] = 0;
      SStrPack(buffer, characters, inputLine->charsalloc);
      inputLine->inputpos = SStrLen(buffer);
      SStrPack(buffer, tail, inputLine->charsalloc);
      SStrCopy(inputLine->buffer, buffer, 0x7FFFFFFF);
      inputLine->chars = SStrLen(inputLine->buffer);
      FREEIFUSED(tail);
      FREEIFUSED(buffer);
    }
  } else {
    unsigned int index;
    for (index = 0; index < length; ++index) {
      inputLine->buffer[inputLine->inputpos++] = characters[index];
    }
    inputLine->buffer[inputLine->inputpos] = 0;
    inputLine->chars = inputLine->inputpos;
  }
}

static void ResetHighlight() {
  s_highlightState = HS_NONE;
  s_hRect.right = 0.0f;
  s_hRect.left = 0.0f;
  s_hRect.top = 0.0f;
  s_hRect.bottom = 0.0f;
}

static int __fastcall OnChar(const EVENT_DATA_CHAR *data, void *__formal) {
  char character[2];

  if (EventIsKeyDown(KEY_TILDE) || !s_active) {
    return 1;
  }
  character[0] = (char)data->ch;
  character[1] = 0;
  PasteInInputLine(character);
  ResetHighlight();
  return 0;
}

static void UpdateHighlight() {
  CGxFont     *font = TextBlockGetFontPtr(s_textFont);
  unsigned int length = SStrLen(s_copyText);
  float        right;
  float        left;

  ASSERT(font);
  left = s_highlightHStart < s_highlightHEnd ? s_highlightHStart : s_highlightHEnd;
  right = s_highlightHStart > s_highlightHEnd ? s_highlightHStart : s_highlightHEnd;
  if (left < 0.0f) {
    left = 0.0f;
  }
  if (right > 1.0f) {
    right = 1.0f;
  }

  s_highlightLeftCharIndex =
      GxuFontGetMaxCharsWithinWidth(font, s_copyText, s_fontHeight, left, length, &s_hRect.left, s_charSpacing, s_baseTextFlags);
  if (s_highlightLeftCharIndex) {
    --s_highlightLeftCharIndex;
  }
  if (s_hRect.left < 0.015f) {
    s_hRect.left = 0.0f;
  }
  s_highlightRightCharIndex =
      GxuFontGetMaxCharsWithinWidth(font, s_copyText, s_fontHeight, right, length, &s_hRect.right, s_charSpacing, s_baseTextFlags);
}

static int __fastcall OnMouseDown(const EVENT_DATA_MOUSE *data, void *__formal) {
  float        clickPos;
  float        visibleHeight;
  int          lineIndex;
  CONSOLELINE *line;

  if (EventIsKeyDown(KEY_TILDE) || !s_active || 1.0f - s_consoleHeight > data->y) {
    return 1;
  }

  visibleHeight = s_consoleHeight >= 1.0f ? 1.0f : s_consoleHeight;
  clickPos = 1.0f - data->y;
  if (clickPos >= visibleHeight - s_fontHeight * 0.75f && clickPos <= s_consoleHeight) {
    ResetHighlight();
    s_consoleResizeState = CS_STRETCH;
    return 0;
  }

  ResetHighlight();
  line = GetLineAtMousePosition(data->y);
  if (line) {
    SStrCopy(s_copyText, line->buffer, sizeof(s_copyText));
    s_highlightState = HS_HIGHLIGHTING;
    lineIndex = (int)((s_consoleHeight - clickPos) / s_fontHeight - 1.0f);
    s_hRect.top = 1.0f - (s_consoleHeight - s_fontHeight * 0.75f - s_fontHeight - lineIndex * s_fontHeight);
    s_hRect.bottom = s_hRect.top - s_fontHeight;
    s_highlightHStart = data->x;
    s_highlightHEnd = data->x;
    UpdateHighlight();
  } else {
    s_copyText[0] = 0;
  }
  return 0;
}

static int __fastcall OnMouseUp(const EVENT_DATA_MOUSE *data, void *__formal) {
  if (!EventIsKeyDown(KEY_TILDE) && s_active) {
    s_highlightState = HS_ENDHIGHLIGHT;
    s_consoleResizeState = CS_NONE;
  }
  return 1;
}

static int __fastcall OnMouseMove(const EVENT_DATA_MOUSE *data, void *__formal) {
  if (EventIsKeyDown(KEY_TILDE) || !s_active) {
    return 1;
  }

  if (s_consoleResizeState == CS_STRETCH) {
    s_consoleHeight = 1.0f - data->y;
    if (s_consoleHeight < s_fontHeight) {
      s_consoleHeight = s_fontHeight;
    }
  } else if (1.0f - s_consoleHeight > data->y) {
    return 1;
  }

  s_highlightHEnd = data->x;
  if (s_highlightState == HS_HIGHLIGHTING) {
    UpdateHighlight();
  }
  return 1;
}

static void MakeCommandCurrent(CONSOLELINE *lineptr, const char *command) {
  unsigned int length;

  lineptr->inputpos = lineptr->inputstart;
  lineptr->chars = lineptr->inputstart;
  lineptr->buffer[lineptr->inputstart] = 0;
  length = SStrLen(command);
  ReserveInputSpace(lineptr, length);
  SStrCopy(&lineptr->buffer[lineptr->inputpos], command, 0x7FFFFFFF);
  lineptr->inputpos += length;
  lineptr->chars = lineptr->inputpos;
}

static void MoveLinePtr(int direction, int modifier) {
  CONSOLELINE *line;
  int          count;

  if (modifier == 1) {
    line = s_currlineptr;
    count = 0;
    while (line) {
      CONSOLELINE *next;
      if (direction == 1) {
        next = line->Next();
      } else {
        next = line->Prev();
      }
      if (next) {
        line = next;
      }
      if (++count >= 10) {
        break;
      }
    }
  } else {
    line = s_currlineptr;
    if (line == s_linelist.Head()) {
      line = line->Next();
      s_currlineptr = line;
    }
    if (direction == 1) {
      line = line->Next();
    } else {
      line = line->Prev();
    }
  }
  if (line) {
    s_currlineptr = line;
  }
}

static int __fastcall OnKeyDown(const EVENT_DATA_KEY *data, void *__formal) {
  CONSOLELINE *inputLine;
  const char  *history;
  unsigned int historyIndex;
  unsigned int length;
  char         buffer[0x80];
  char        *clipboard;

  if (data->key == KEY_TILDE) {
    int wasActive = s_active;
    s_active = !s_active;
    if (wasActive) {
      ResetHighlight();
    }
    return 0;
  }
  if (EventIsKeyDown(KEY_TILDE) || !s_active) {
    return 1;
  }

  inputLine = GetInputLine();
  switch (data->key) {
    case KEY_ENTER:
      if (inputLine->inputpos > inputLine->inputstart) {
        inputLine->inputpos = 0;
        GenerateNodeString(inputLine);
        ConsoleCommandExecute(&inputLine->buffer[inputLine->inputstart], 1);
        s_historyIndex = -1;
      }
      break;

    case KEY_C:
      if (data->metaKeyState == 2) {
        if (s_copyText[0]) {
          length = s_highlightRightCharIndex - s_highlightLeftCharIndex;
          if (length >= sizeof(buffer)) {
            length = sizeof(buffer) - 1;
          }
          memcpy(buffer, &s_copyText[s_highlightLeftCharIndex], length);
          buffer[length] = 0;
          OsClipboardPutString(buffer);
        }
        ResetHighlight();
      }
      break;

    case KEY_V:
      if (data->metaKeyState == 2) {
        clipboard = OsClipboardGetString();
        if (clipboard) {
          PasteInInputLine(clipboard);
          FREE(clipboard);
          ResetHighlight();
        }
      }
      break;

    case KEY_ESCAPE:
      if (inputLine->inputpos > inputLine->inputstart) {
        inputLine->inputpos = inputLine->inputstart;
        inputLine->chars = inputLine->inputstart;
        inputLine->buffer[inputLine->inputstart] = 0;
      } else {
        s_active = 0;
      }
      break;

    case KEY_BACKSPACE:
      if (inputLine->inputpos > inputLine->inputstart) {
        if (inputLine->chars <= inputLine->inputpos) {
          inputLine->buffer[inputLine->inputpos - 1] = 0;
        } else {
          memmove(&inputLine->buffer[inputLine->inputpos - 1], &inputLine->buffer[inputLine->inputpos], inputLine->chars - inputLine->inputpos + 1);
        }
        --inputLine->inputpos;
        --inputLine->chars;
      }
      break;

    case KEY_TAB:
      if (!s_completionMode) {
        s_completionMode = 1;
        s_completedCmd = 0;
        SStrCopy(s_partial, &inputLine->buffer[inputLine->inputstart], 0x7FFFFFFF);
      }
      if (ConsoleCommandComplete(s_partial, &s_completedCmd, data->metaKeyState == 1)) {
        MakeCommandCurrent(inputLine, s_completedCmd);
      }
      break;

    case KEY_LEFT:
      if (inputLine->inputpos > inputLine->inputstart) {
        --inputLine->inputpos;
      }
      break;

    case KEY_RIGHT:
      if (inputLine->inputpos < inputLine->chars) {
        ++inputLine->inputpos;
      }
      break;

    case KEY_UP:
      if ((unsigned int)s_historyIndex != ConsoleCommandHistoryDepth() - 1) {
        historyIndex = s_historyIndex + 1;
        history = ConsoleCommandHistory(s_historyIndex);
        if (history) {
          MakeCommandCurrent(inputLine, history);
          s_historyIndex = historyIndex;
        }
      }
      break;

    case KEY_DOWN:
      if (s_historyIndex != -1) {
        historyIndex = s_historyIndex - 1;
        history = s_historyIndex ? ConsoleCommandHistory(s_historyIndex) : "";
        if (history) {
          MakeCommandCurrent(inputLine, history);
          s_historyIndex = historyIndex;
        }
      }
      break;

    case KEY_DELETE:
      if (inputLine->inputpos <= inputLine->chars) {
        memmove(&inputLine->buffer[inputLine->inputpos], &inputLine->buffer[inputLine->inputpos + 1], inputLine->chars - inputLine->inputpos);
        --inputLine->chars;
      }
      break;

    case KEY_HOME:
      if (data->metaKeyState == 2) {
        s_currlineptr = s_linelist.Tail();
      } else if (inputLine->inputpos > inputLine->inputstart) {
        inputLine->inputpos = inputLine->inputstart;
      }
      break;

    case KEY_END:
      if (data->metaKeyState == 2) {
        s_currlineptr = s_linelist.Head();
      } else if (inputLine->inputpos < inputLine->chars) {
        inputLine->inputpos = inputLine->chars;
      }
      break;

    case KEY_PAGEUP:
      MoveLinePtr(1, data->metaKeyState);
      break;

    case KEY_PAGEDOWN:
      MoveLinePtr(0, data->metaKeyState);
      break;
  }

  if (data->key != KEY_TAB && data->key != KEY_SHIFT && data->key != KEY_ALT && data->metaKeyState != 2) {
    s_completionMode = 0;
    ResetHighlight();
  }
  return 0;
}

static int __fastcall OnKeyDownRepeat(const EVENT_DATA_KEY *data, void *__formal) {
  CONSOLELINE *inputLine;
  const char  *history;
  unsigned int historyIndex;

  if (data->key == KEY_TILDE) {
    s_active = !s_active;
    return 0;
  }
  if (EventIsKeyDown(KEY_TILDE) || !s_active) {
    return 1;
  }

  inputLine = GetInputLine();
  switch (data->key) {
    case KEY_BACKSPACE:
      if (inputLine->inputpos > inputLine->inputstart) {
        if (inputLine->chars <= inputLine->inputpos) {
          inputLine->buffer[inputLine->inputpos - 1] = 0;
        } else {
          memmove(&inputLine->buffer[inputLine->inputpos - 1], &inputLine->buffer[inputLine->inputpos], inputLine->chars - inputLine->inputpos + 1);
        }
        --inputLine->inputpos;
        --inputLine->chars;
      }
      break;
    case KEY_LEFT:
      if (inputLine->inputpos > inputLine->inputstart) {
        --inputLine->inputpos;
      }
      break;
    case KEY_UP:
      if ((unsigned int)s_historyIndex != ConsoleCommandHistoryDepth() - 1) {
        historyIndex = s_historyIndex + 1;
        history = ConsoleCommandHistory(s_historyIndex);
        if (history) {
          MakeCommandCurrent(inputLine, history);
          s_historyIndex = historyIndex;
        }
      }
      break;
    case KEY_RIGHT:
      if (inputLine->inputpos < inputLine->chars) {
        ++inputLine->inputpos;
      }
      break;
    case KEY_DOWN:
      if (s_historyIndex != -1) {
        historyIndex = s_historyIndex - 1;
        history = s_historyIndex ? ConsoleCommandHistory(s_historyIndex) : "";
        if (history) {
          MakeCommandCurrent(inputLine, history);
          s_historyIndex = historyIndex;
        }
      }
      break;
    case KEY_DELETE:
      if (inputLine->inputpos <= inputLine->chars) {
        memmove(&inputLine->buffer[inputLine->inputpos], &inputLine->buffer[inputLine->inputpos + 1], inputLine->chars - inputLine->inputpos);
        --inputLine->chars;
      }
      break;
    case KEY_PAGEUP:
      MoveLinePtr(1, data->metaKeyState);
      break;
    case KEY_PAGEDOWN:
      MoveLinePtr(0, data->metaKeyState);
      break;
  }

  if (data->key != KEY_TAB && data->key != KEY_SHIFT && data->key != KEY_ALT && data->metaKeyState != 2) {
    s_completionMode = 0;
    ResetHighlight();
  }
  return 0;
}

static int __fastcall OnKeyUp(const EVENT_DATA_KEY *data, void *__formal) {
  return !s_active;
}

static void RegisterHandlers() {
  EventRegisterEx(EVENT_ID_CHAR, (EVENTHANDLER)OnChar, 0, 7.0f);
  EventRegisterEx(EVENT_ID_IDLE, (EVENTHANDLER)OnIdle, 0, 7.0f);
  EventRegisterEx(EVENT_ID_KEYDOWN, (EVENTHANDLER)OnKeyDown, 0, 7.0f);
  EventRegisterEx(EVENT_ID_KEYUP, (EVENTHANDLER)OnKeyUp, 0, 7.0f);
  EventRegisterEx(EVENT_ID_KEYDOWN_REPEATING, (EVENTHANDLER)OnKeyDownRepeat, 0, 7.0f);
  EventRegisterEx(EVENT_ID_MOUSEDOWN, (EVENTHANDLER)OnMouseDown, 0, 7.0f);
  EventRegisterEx(EVENT_ID_MOUSEUP, (EVENTHANDLER)OnMouseUp, 0, 7.0f);
  EventRegisterEx(EVENT_ID_MOUSEMOVE, (EVENTHANDLER)OnMouseMove, 0, 7.0f);
}

static void UnregisterHandlers() {
  EventUnregister(EVENT_ID_CHAR, (EVENTHANDLER)OnChar);
  EventUnregister(EVENT_ID_IDLE, (EVENTHANDLER)OnIdle);
  EventUnregister(EVENT_ID_KEYDOWN, (EVENTHANDLER)OnKeyDown);
  EventUnregister(EVENT_ID_KEYDOWN_REPEATING, (EVENTHANDLER)OnKeyDownRepeat);
  EventUnregister(EVENT_ID_KEYUP, (EVENTHANDLER)OnKeyUp);
  EventUnregister(EVENT_ID_MOUSEDOWN, (EVENTHANDLER)OnMouseDown);
  EventUnregister(EVENT_ID_MOUSEUP, (EVENTHANDLER)OnMouseUp);
  EventUnregister(EVENT_ID_MOUSEMOVE, (EVENTHANDLER)OnMouseMove);
}

static void RegenerateFontStrings() {
  CONSOLELINE *node = s_linelist.Head();

  while (node) {
    GenerateNodeString(node);
    node = s_linelist.Next(node);
  }
}

static int __fastcall ConsoleCommand_ClearConsole(const char *cmd, const char *arguments) {
  CONSOLELINE *line;

  s_NumLines = 0;
  while ((line = s_linelist.Head()) != 0) {
    FREEIFUSED(line->buffer);
    if (line->fontPointer) {
      GxuFontDestroyString(line->fontPointer);
    }
    s_linelist.DeleteNode(line);
  }
  return 1;
}

static int __fastcall ConsoleCommand_Proportional(const char *cmd, const char *args) {
  s_baseTextFlags ^= 0x10;
  RegenerateFontStrings();
  return 1;
}

static int __fastcall ConsoleCommand_CharSpacing(const char *cmd, const char *args) {
  if (args && args[0]) {
    s_charSpacing = SStrToFloat(args);
  } else {
    s_charSpacing = 0.0f;
  }
  RegenerateFontStrings();
  return 1;
}

static int __fastcall ConsoleCommand_DefaultSettings(const char *cmd, const char *arguments) {
  s_colorArray[DEFAULT_COLOR] = NTempest::CImVector(255, 255, 255, 255);
  s_colorArray[INPUT_COLOR] = NTempest::CImVector(255, 255, 255, 255);
  s_colorArray[ECHO_COLOR] = NTempest::CImVector(255, 128, 128, 128);
  s_colorArray[ERROR_COLOR] = NTempest::CImVector(255, 255, 0, 0);
  *(DWORD *)&s_colorArray[WARNING_COLOR] = 0xFFFFFF00;
  *(DWORD *)&s_colorArray[GLOBAL_COLOR] = 0xFFFFFFFF;
  *(DWORD *)&s_colorArray[ADMIN_COLOR] = 0xFFFFFFFF;
  *(DWORD *)&s_colorArray[BACKGROUND_COLOR] = 0xC0000000;
  *(DWORD *)&s_colorArray[HIGHLIGHT_COLOR] = 0x80FFFFFF;

  s_fontHeight = 0.02f;
  SStrCopy(s_fontName, "Fonts\\ARIALN.ttf", sizeof(s_fontName));
  if (s_textFont) {
    HandleClose((HOBJECT)s_textFont);
  }
  s_textFont = TextBlockGenerateFont(s_fontName, 0, NDCToDDCHeight(s_fontHeight));
  RegenerateFontStrings();
  s_consoleLines = 10.0f;
  s_consoleHeight = s_fontHeight * 10.0f;
  return 1;
}

static int __fastcall ConsoleCommand_CloseConsole(const char *cmd, const char *arguments) {
  s_active = 0;
  return 0;
}

static int __fastcall ConsoleCommand_RepeatHandler(const char *cmd, const char *arguments) {
  const char  *command;
  unsigned int count;
  unsigned int length;
  char        *copy;

  if (arguments && arguments[0]) {
    length = SStrLen(arguments);
    copy = (char *)_alloca(length + 1);
    SStrCopy(copy, arguments, length + 1);
    command = copy;
    while (*command && *command != '\t' && *command != ' ') {
      ++command;
    }
    if (*command) {
      *(char *)command++ = 0;
    }
    count = SStrToUnsigned(copy);
    if (count && command && *command) {
      SStrPrintf(s_repeatBuffer, sizeof(s_repeatBuffer), "%s", command);
      s_repeatCount = count;
      return 1;
    }
  } else {
    s_repeatCount = 0;
  }
  return 1;
}

static int __fastcall ConsoleCommand_CurrentSettings(const char *cmd, const char *arguments) {
  char buffer[0x104];

  SStrPrintf(buffer, sizeof(buffer), "Font Height is %f", s_fontHeight);
  ConsoleWrite(buffer, DEFAULT_COLOR);
  SStrPrintf(buffer, sizeof(buffer), "Font Name is %s", s_fontName);
  ConsoleWrite(buffer, DEFAULT_COLOR);
  SStrPrintf(buffer, sizeof(buffer), "Number of Console Lines %f", s_consoleLines);
  ConsoleWrite(buffer, DEFAULT_COLOR);
  return 1;
}

static int __fastcall ConsoleCommand_FontColor(const char *cmd, const char *arguments) {
  char                colorType[32];
  unsigned int        blue;
  unsigned int        green;
  unsigned int        red;
  COLOR_T             type;
  NTempest::CImVector color;
  CONSOLELINE        *line;

  if (sscanf(arguments, "%s %d %d %d", colorType, &red, &green, &blue) != 4) {
    ConsoleWrite("Invalid number of parameters", ERROR_COLOR);
    ConsoleCommandWriteHelp(cmd);
    return 0;
  }

  if (!SStrCmpI(colorType, "input", 0x7FFFFFFF)) {
    type = INPUT_COLOR;
  } else if (!SStrCmpI(colorType, "default", 0x7FFFFFFF)) {
    type = DEFAULT_COLOR;
  } else if (!SStrCmpI(colorType, "echo", 0x7FFFFFFF)) {
    type = ECHO_COLOR;
  } else if (!SStrCmpI(colorType, "error", 0x7FFFFFFF)) {
    type = ERROR_COLOR;
  } else if (!SStrCmpI(colorType, "warning", 0x7FFFFFFF)) {
    type = WARNING_COLOR;
  } else if (!SStrCmpI(colorType, "admin", 0x7FFFFFFF)) {
    type = ADMIN_COLOR;
  } else if (!SStrCmpI(colorType, "global", 0x7FFFFFFF)) {
    type = GLOBAL_COLOR;
  } else {
    ConsoleWrite(
        "Unknown color class. Choose 'input','echo','warning','admin',or "
        "'global' as the first parameter",
        ERROR_COLOR
    );
    return 0;
  }

  if (red > 255 || green > 255 || blue > 255) {
    ConsoleWrite("One or more colors are not in the 0 to 255 range or missing.", ERROR_COLOR);
    ConsoleWrite("Make sure to specify the red, green and blue colors.", ERROR_COLOR);
    return 0;
  }

  color.Set(255, (unsigned char)red, (unsigned char)green, (unsigned char)blue);
  SetColor(type, color);
  line = s_linelist.Head();
  while (line) {
    if (line->colorType == type && line->fontPointer) {
      GxuFontSetStringColor(line->fontPointer, color);
    }
    line = s_linelist.Next(line);
  }
  return 1;
}

static int __fastcall ConsoleCommand_BufferSize(const char *cmd, const char *arguments) {
  float height;

  if (arguments && arguments[0]) {
    if (sscanf(arguments, "%f", &height) != 1) {
      ConsoleWrite("Invalid number of parameters", ERROR_COLOR);
      ConsoleCommandWriteHelp(cmd);
      return 0;
    }
    if (height == 0.0f) {
      return 0;
    }
    s_consoleLines = height;
    s_consoleHeight = s_fontHeight * height;
    return 1;
  }

  ConsoleWrite("Please specify how many lines to display", DEFAULT_COLOR);
  return 1;
}

static int __fastcall ConsoleCommand_FontSize(const char *cmd, const char *arguments) {
  s_fontHeight = SStrToFloat(arguments) * 0.001f;
  if (s_fontHeight < 0.01f) {
    s_fontHeight = 0.01f;
  } else if (s_fontHeight > 0.05f) {
    s_fontHeight = 0.05f;
  }

  if (s_textFont) {
    HandleClose((HOBJECT)s_textFont);
  }
  s_textFont = TextBlockGenerateFont(s_fontName, 0, NDCToDDCHeight(s_fontHeight));
  s_consoleLines = s_consoleHeight / s_fontHeight;
  s_consoleHeight = s_consoleLines * s_fontHeight;
  RegenerateFontStrings();
  return 1;
}

static int __fastcall ConsoleCommand_Font(const char *cmd, const char *arguments) {
  char         buffer[0x104] = "Fonts\\";
  HTEXTFONT__ *font;

  SStrPack(buffer, arguments, sizeof(buffer));
  SStrPack(buffer, ".ttf", sizeof(buffer));
  font = TextBlockGenerateFont(buffer, 0, NDCToDDCHeight(s_fontHeight));
  if (font) {
    if (s_textFont) {
      HandleClose((HOBJECT)s_textFont);
    }
    s_textFont = font;
    SStrCopy(s_fontName, buffer, 0x7FFFFFFF);
  }
  RegenerateFontStrings();
  return 1;
}

static int __fastcall ConsoleCommand_BackGroundColor(const char *cmd, const char *arguments) {
  unsigned int blue;
  unsigned int alpha;
  unsigned int green;
  unsigned int red;
  if (sscanf(arguments, "%d %d %d %d", &alpha, &red, &green, &blue) != 4) {
    ConsoleWrite("Invalid number of parameters", ERROR_COLOR);
    ConsoleCommandWriteHelp(cmd);
    return 0;
  }
  if (alpha > 255 || red > 255 || green > 255 || blue > 255) {
    ConsoleWrite("One or more colors are not in the 0 to 255 range or missing.", ERROR_COLOR);
    ConsoleWrite("Make sure to specify the red, green and blue colors.", ERROR_COLOR);
    return 0;
  }
  SetColor(BACKGROUND_COLOR, NTempest::CImVector((alpha << 24) | (red << 16) | (green << 8) | blue));
  return 1;
}

static int __fastcall ConsoleCommand_HighLightColor(const char *cmd, const char *arguments) {
  unsigned int blue;
  unsigned int alpha;
  unsigned int green;
  unsigned int red;
  if (sscanf(arguments, "%d %d %d %d", &alpha, &red, &green, &blue) != 4) {
    ConsoleWrite("Invalid number of parameters", ERROR_COLOR);
    ConsoleCommandWriteHelp(cmd);
    return 0;
  }
  if (alpha > 255 || red > 255 || green > 255 || blue > 255) {
    ConsoleWrite("One or more colors are not in the 0 to 255 range or missing.", ERROR_COLOR);
    ConsoleWrite("Make sure to specify the red, green and blue colors.", ERROR_COLOR);
    return 0;
  }
  SetColor(HIGHLIGHT_COLOR, NTempest::CImVector((alpha << 24) | (red << 16) | (green << 8) | blue));
  return 1;
}

static void SetCVar(CVar *cvar, float val) {
  char str[32];
  SStrPrintf(str, sizeof(str), "%f", val);
  cvar->Set(str, true, false, false);
}

static void SetCVar(CVar *cvar, unsigned int val) {
  char str[32];
  SStrPrintf(str, sizeof(str), "%d", val);
  cvar->Set(str, true, false, false);
}

static void SetCVar(CVar *cvar, bool val) {
  char str[32];
  SStrPrintf(str, sizeof(str), val ? "1" : "0");
  cvar->Set(str, true, false, false);
}

static void SetGameCVars() {
  if (!CWorldParam::cvar_farClip) {
    return;
  }

  SetCVar(CWorldParam::cvar_farClip, s_defaults.farClip);
  SetCVar(CWorldParam::cvar_lodDist, s_defaults.terrainLODDist);
  SetCVar(CWorldParam::cvar_shadowLevel, s_defaults.terrainShadowLOD);
  SetCVar(CWorldParam::cvar_alphaLevel, s_defaults.terrainShadowLOD);
  SetCVar(CWorldParam::cvar_detailDensity, s_defaults.detailDoodadDensity);
  SetCVar(CWorldParam::cvar_fullAlpha, s_defaults.detailDoodadAlpha);
  SetCVar(CWorldParam::cvar_doodadAnim, s_defaults.animatingDoodads);
  SetCVar(CWorldParam::cvar_triLinear, s_defaults.trilinear);
  SetCVar(CWorldParam::cvar_maxLights, s_defaults.numLights);
  SetCVar(CWorldParam::cvar_pixelShaders, s_defaults.specularity);
  SetCVar(CWorldParam::cvar_specular, s_defaults.specularity);
  SetCVar(CWorldParam::cvar_waterLod, s_defaults.waterLOD);
  SetCVar(CWorldParam::cvar_particleDensity, s_defaults.particleDensity);
  SetCVar(CWorldParam::cvar_unitDrawDist, s_defaults.unitDrawDist);
  SetCVar(CWorldParam::cvar_smallCull, s_defaults.smallCull);
  SetCVar(CWorldParam::cvar_distCull, s_defaults.distCull);
  SetCVar(CWorldParam::cvar_baseMip, s_defaults.baseMipLevel);
}

static bool __fastcall CVGxResolutionCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  static NTempest::C2iVector legalSizes[7] = {NTempest::C2iVector(640, 480),  NTempest::C2iVector(800, 600),  NTempest::C2iVector(1024, 768),
                                              NTempest::C2iVector(1152, 864), NTempest::C2iVector(1280, 960), NTempest::C2iVector(1280, 1024),
                                              NTempest::C2iVector(1600, 1200)};
  NTempest::C2iVector        size(-1);
  char                       x;
  unsigned int               index;

  sscanf(newValue, "%d%c%d", &size.x, &x, &size.y);
  for (index = 0; index < 7; ++index) {
    if (size.x == legalSizes[index].x && size.y == legalSizes[index].y) {
      break;
    }
  }

  if (index == 7) {
    char         msg[0x400] = "invalid resolution, must be one of ";
    char         rez[32];
    unsigned int legalIndex;

    for (legalIndex = 0; legalIndex < 7; ++legalIndex) {
      if (legalIndex) {
        strcat(msg, ", ");
      }
      SStrPrintf(rez, sizeof(rez), "%dx%d", legalSizes[legalIndex].x, legalSizes[legalIndex].y);
      strcat(msg, rez);
    }
    ConsoleWrite(msg, DEFAULT_COLOR);
    return false;
  }

  s_requestedFormat.size = size;
  ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
  return true;
}

static bool __fastcall CVGxColorBitsCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  int bits = SStrToInt(newValue);

  if (bits == 16) {
    s_requestedFormat.colorFormat = CGxFormat::Fmt_Rgb565;
  } else if (bits == 24) {
    s_requestedFormat.colorFormat = CGxFormat::Fmt_ArgbX888;
  } else if (bits == 30) {
    s_requestedFormat.colorFormat = CGxFormat::Fmt_Argb2101010;
  } else {
    ConsoleWrite("Color bits must be 16, 24, or 30", DEFAULT_COLOR);
    return false;
  }
  ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
  return true;
}

static bool __fastcall CVGxDepthBitsCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  int bits = SStrToInt(newValue);

  if (bits == 16) {
    s_requestedFormat.depthFormat = CGxFormat::Fmt_Ds160;
  } else if (bits == 24) {
    s_requestedFormat.depthFormat = CGxFormat::Fmt_Ds24X;
  } else if (bits == 32) {
    s_requestedFormat.depthFormat = CGxFormat::Fmt_Ds320;
  } else {
    ConsoleWrite("Depth bits must be 16, 24, or 32", DEFAULT_COLOR);
    return false;
  }
  ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
  return true;
}

static bool __fastcall CVGxRefreshCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  int          rate = SStrToInt(newValue);
  unsigned int index;

  for (index = 0; index < 11; ++index) {
    if (s_rates[index] == rate) {
      break;
    }
  }
  if (index == 11) {
    char         msg[0x400] = "unsupported refresh rate, must be one of ";
    char         number[32];
    unsigned int rateIndex;

    for (rateIndex = 0; rateIndex < 11; ++rateIndex) {
      if (rateIndex) {
        strcat(msg, ", ");
      }
      SStrPrintf(number, sizeof(number), "%d", s_rates[rateIndex]);
      strcat(msg, number);
    }
    ConsoleWrite(msg, DEFAULT_COLOR);
    return false;
  }
  s_requestedFormat.refreshRate = s_rates[index];
  ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
  return true;
}

static bool __fastcall CVGxApiCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  unsigned int index;

  for (index = 0; index < 2; ++index) {
    if (!SStrCmpI(newValue, s_apis[index], 16)) {
      break;
    }
  }
  if (index != 2) {
    return true;
  }

  {
    char         msg[0x400] = "unsupported api, must be one of ";
    unsigned int apiIndex;

    for (apiIndex = 0; apiIndex < 2; ++apiIndex) {
      if (apiIndex) {
        strcat(msg, ", ");
      }
      strcat(msg, "'");
      strcat(msg, s_apis[apiIndex]);
      strcat(msg, "'");
    }
    ConsoleWrite(msg, DEFAULT_COLOR);
  }
  return false;
}

static bool __fastcall CVGxVSyncCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  s_requestedFormat.vsync = SStrToInt(newValue) != 0;
  ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
  return true;
}

static bool __fastcall CVGxWindowCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  s_requestedFormat.window = SStrToInt(newValue) != 0;
  ConsoleWrite("set pending gxRestart", DEFAULT_COLOR);
  return true;
}

static void UpdateGxCVars() {
  s_cvGxColorBits->Update();
  s_cvGxDepthBits->Update();
  s_cvGxResolution->Update();
  s_cvGxRefresh->Update();
  s_cvGxApi->Update();
  s_cvGxVSync->Update();
  s_cvGxWindow->Update();
}

static void SetGxCVars(const CGxFormat &format) {
  char string[0x400];

  s_cvGxColorBits->Set(formatToInt[format.colorFormat], true, false, false);
  s_cvGxDepthBits->Set(formatToInt[format.depthFormat], true, false, false);
  SStrPrintf(string, sizeof(string), "%dx%d", format.size.x, format.size.y);
  s_cvGxResolution->Set(string, true, false, false);
  SStrPrintf(string, sizeof(string), "%d", format.refreshRate);
  s_cvGxRefresh->Set(string, true, false, false);
  SStrPrintf(string, sizeof(string), "%d", format.vsync);
  s_cvGxVSync->Set(string, true, false, false);
  SStrPrintf(string, sizeof(string), "%d", format.window);
  s_cvGxWindow->Set(string, true, false, false);
  UpdateGxCVars();
}

static int __fastcall CCGxRestart(const char *__formal, const char *args) {
  ValidateFormatMonitor(s_requestedFormat);
  if (GxDevSetFormat(s_requestedFormat)) {
    s_lastGoodFormat = s_requestedFormat;
    UpdateGxCVars();
    OsGuiSetGxWindow((void *)GxDevWindow());
    return 1;
  }

  ConsoleWrite("unable to set requested display mode", DEFAULT_COLOR);
  s_requestedFormat = s_lastGoodFormat;
  if (GxDevSetFormat(s_requestedFormat)) {
    SetGxCVars(s_requestedFormat);
    OsGuiSetGxWindow((void *)GxDevWindow());
    return 1;
  }

  ConsoleWrite("unable to set last good mode", DEFAULT_COLOR);
  s_requestedFormat = s_desktopFormat;
  if (GxDevSetFormat(s_requestedFormat)) {
    s_lastGoodFormat = s_requestedFormat;
    SetGxCVars(s_requestedFormat);
    OsGuiSetGxWindow((void *)GxDevWindow());
    return 1;
  }

  ConsoleWrite("unable to set default format", DEFAULT_COLOR);
  s_requestedFormat = s_fallbackFormat;
  if (GxDevSetFormat(s_requestedFormat)) {
    s_lastGoodFormat = s_requestedFormat;
    SetGxCVars(s_requestedFormat);
  } else {
    FATALERROR(("unable to set fallback mode"));
  }
  OsGuiSetGxWindow((void *)GxDevWindow());
  return 1;
}

static void RegisterGxCVars() {
  s_cvGxColorBits = CVar::Register("gxColorBits", "color bits", 3, "16", CVGxColorBitsCallback, GRAPHICS, false, 0);
  s_cvGxDepthBits = CVar::Register("gxDepthBits", "depth bits", 3, "16", CVGxDepthBitsCallback, GRAPHICS, false, 0);
  s_cvGxResolution = CVar::Register("gxResolution", "resolution", 3, "640x480", CVGxResolutionCallback, GRAPHICS, false, 0);
  s_cvGxRefresh = CVar::Register("gxRefresh", "refresh rate", 3, "75", CVGxRefreshCallback, GRAPHICS, false, 0);
  s_cvGxApi = CVar::Register("gxApi", "graphics api", 3, "direct3d", CVGxApiCallback, GRAPHICS, false, 0);
  s_cvGxVSync = CVar::Register("gxVSync", "vsync on or off", 3, "1", CVGxVSyncCallback, GRAPHICS, false, 0);
  s_cvGxWindow = CVar::Register("gxWindow", "toggle fullscreen/window", 3, "0", CVGxWindowCallback, GRAPHICS, false, 0);
  s_cvHwDetect = CVar::Register("hwDetect", "do hardware detection", 1, "1", 0, GRAPHICS, false, 0);
}

static void RegisterGxCCmds() {
  ConsoleCommandRegister("gxRestart", CCGxRestart, GRAPHICS, 0);
}

static bool ReduceFormat(CGxFormat &fmt) {
  if (fmt.colorFormat <= CGxFormat::Fmt_Rgb565) {
    return false;
  }
  if (fmt.depthFormat > CGxFormat::Fmt_Ds160) {
    fmt.depthFormat = (CGxFormat::Format)(fmt.depthFormat - 1);
    return true;
  }

  fmt.colorFormat = (CGxFormat::Format)(fmt.colorFormat - 1);
  if (fmt.colorFormat == CGxFormat::Fmt_Rgb565) {
    fmt.depthFormat = CGxFormat::Fmt_Ds160;
  } else {
    fmt.depthFormat = CGxFormat::Fmt_Ds320;
  }
  return true;
}

static void OptimizeFormat(CGxFormat &fmt) {
  if (fmt.size.x < 800 && fmt.size.y < 600 && s_defaults.format->size.x >= 800 && s_defaults.format->size.y >= 600) {
    fmt.size.x = 800;
    fmt.size.y = 600;
  }
  if (fmt.depthFormat < CGxFormat::Fmt_Ds24X && s_defaults.format->depthFormat >= CGxFormat::Fmt_Ds24X) {
    fmt.depthFormat = CGxFormat::Fmt_Ds24X;
  }
  if (fmt.colorFormat < CGxFormat::Fmt_ArgbX888 && s_defaults.format->colorFormat >= CGxFormat::Fmt_ArgbX888) {
    fmt.colorFormat = CGxFormat::Fmt_ArgbX888;
  }
}

static void ValidateFormatMonitor(CGxFormat &fmt) {
  unsigned int fmtbpp;
  unsigned int lowRate = 9999;
  unsigned int index;

  ASSERT(fmt.colorFormat < (sizeof(s_FormatTobpp) / sizeof(s_FormatTobpp[0])));

  fmtbpp = s_FormatTobpp[fmt.colorFormat];
  for (index = 0; index < s_gxMonitorModes.Count(); ++index) {
    CGxMonitorMode &mode = s_gxMonitorModes[index];
    if (fmt.size.x == mode.size.x && fmt.size.y == mode.size.y && fmtbpp == mode.bpp) {
      if (mode.refreshRate < lowRate) {
        lowRate = mode.refreshRate;
      }
      if (fmt.refreshRate == mode.refreshRate) {
        return;
      }
    }
  }

  if (lowRate == 9999) {
    GxLog("ValidateFormatMonitor(): unable to find monitor refresh");
    lowRate = 60;
  }
  GxLog("ValidateFormatMonitor(): invalid refresh rate %d, set to %d", fmt.refreshRate, lowRate);
  fmt.refreshRate = lowRate;
}

void __fastcall ConsoleDeviceInitialize(const char *title, bool multithreaded) {
  CGxFormat      apiFormat;
  CGxMonitorMode desktopMode;
  unsigned int   i;
  EGxApi         gxApi;
  bool           alreadyReduced;
  bool           hwChanged;
  bool           hwDetect;

  GxLogOpen();
  RegisterGxCVars();
  RegisterGxCCmds();
  DetectHardware(s_hardware, hwChanged);

  if (CmdLineGetBool((CMDOPT)0x24) == 1 || s_cvHwDetect->GetInt()) {
    hwDetect = true;
    s_cvHwDetect->Set("0", true, false, false);
  } else {
    hwDetect = false;
  }

  GxAdapterMonitorModes(s_gxMonitorModes);
  ValidateFormatMonitor(s_fallbackFormat);
  desktopMode.size.x = 0;
  desktopMode.size.y = 0;
  if (GxAdapterDesktopMode(desktopMode)) {
    s_desktopFormat.size = desktopMode.size;
    s_desktopFormat.colorFormat = desktopMode.bpp > 16 ? CGxFormat::Fmt_ArgbX888 : CGxFormat::Fmt_Rgb565;
    s_desktopFormat.refreshRate = desktopMode.refreshRate;
  }

  GxLog("ConsoleDeviceInitialize(): hwDetect = %d, hwChanged = %d", hwDetect, hwChanged);
  if (hwDetect || hwChanged) {
    SetDefaults(s_defaults, s_hardware);
    s_requestedFormat = *s_defaults.format;
    SetGxCVars(s_requestedFormat);
  } else {
    SetDefaultsFormat(s_defaults, s_hardware);
  }

  gxApi = !SStrCmpI(s_cvGxApi->GetString(), "OpenGl", 0x7FFFFFFF) ? GxApi_OpenGl : GxApi_Direct3d;
  if (CmdLineGetBool(CMD_OPENGL)) {
    gxApi = GxApi_OpenGl;
  }
  if (CmdLineGetBool(CMD_D3D)) {
    gxApi = GxApi_Direct3d;
  }

  s_requestedFormat.fixLag = !CmdLineGetBool((CMDOPT)0x18);
  s_requestedFormat.hwTnL = !CmdLineGetBool(CMD_SW_TNL);
  s_requestedFormat.window = CmdLineGetBool((CMDOPT)0x1F) == 1;
  s_desktopFormat.window = s_requestedFormat.window;

  for (i = 0; i < 2; ++i) {
    alreadyReduced = false;

    apiFormat = s_requestedFormat;
    OptimizeFormat(apiFormat);
    ValidateFormatMonitor(apiFormat);
    s_device = GxDevCreate((EGxApi)((i + gxApi) & 1), OsWindowProc, apiFormat);
    while (!s_device) {
      if (!ReduceFormat(apiFormat)) {
        if (alreadyReduced) {
          break;
        }
        apiFormat = s_desktopFormat;
        alreadyReduced = true;
      }
      ValidateFormatMonitor(apiFormat);
      s_device = GxDevCreate((EGxApi)((i + gxApi) & 1), OsWindowProc, apiFormat);
    }
    if (s_device) {
      break;
    }
  }

  if (s_device) {
    s_requestedFormat = apiFormat;
    s_lastGoodFormat = apiFormat;
    SetGxCVars(apiFormat);
  }
  if (!s_device) {
    GxLog("ConsoleDeviceInitialize(): no output device available!");
    FATALERROR(("No output device available!"));
  }

  if (GxCaps().m_numTmus < 2) {
    GxDevDestroy(s_device);
    GxLog("ConsoleDeviceInitialize(): output device does not have dual TMUs!");
    FATALERROR(("Output device does not have dual TMUs!"));
  }

  {
    int    overrideValue = -2;
    EGxApi api = GxDevApi();

    if (api == GxApi_OpenGl) {
      overrideValue = s_hardware.videoHw->m_oglPixelShader;
    } else if (api == GxApi_Direct3d) {
      overrideValue = s_hardware.videoHw->m_d3dPixelShader;
    }
    if (overrideValue != -2) {
      GxDevOverride(GxOverride_PixelShader, overrideValue);
    }
  }

  OsGuiSetGxWindow((void *)GxDevWindow());
  ConsoleSetTitle(title);
  CWorldParam::Initialize();
  if (hwDetect) {
    SetGameCVars();
  } else if (hwChanged) {
    SetGameCVars();
  }
}

void __fastcall ConsoleDeviceDestroy() {
  GxDevDestroy(s_device);
  s_device = 0;
  GxLogClose();
}

static int __fastcall EventCloseCallback(void *param) {
  ConsolePostClose();
  return 0;
}

void __fastcall ConsoleScreenInitialize(const char *title) {
  NTempest::CRect windowSize(0.0f);
  float           width;
  float           height;
  unsigned int    index;

  GxCapsWindowSize(windowSize);
  width = windowSize.maxx - windowSize.minx;
  s_caretpixwidth = width == 0.0f ? 1.0f : 1.0f / width;
  height = windowSize.maxy - windowSize.miny;
  s_caretpixheight = height == 0.0f ? 1.0f : 1.0f / height;
  SStrCopy(s_fontName, "Fonts\\ARIALN.ttf", sizeof(s_fontName));
  s_textFont = TextBlockGenerateFont(s_fontName, 0, NDCToDDCHeight(s_fontHeight));
  ScrnLayerCreate(&s_rect, 6.0f, 3, 0, PaintBackground, &s_layerBackground);
  ScrnLayerCreate(&s_rect, 7.0f, 3, 0, PaintText, &s_layerText);
  ScrnPerfEnable(0);
  RegisterHandlers();
  for (index = 0; index < 13; ++index) {
    ConsoleCommandRegister(s_consoleCommands[index], s_commandHandlers[index], CONSOLE, s_consoleHelpText[index]);
  }
  EventSetConfirmCloseCallback(EventCloseCallback, 0);
  ConsoleCommandExecute("ver", 1);
}

void __fastcall ConsoleScreenDestroy() {
  CONSOLELINE *line;

  EventSetConfirmCloseCallback(0, 0);
  UnregisterHandlers();
  HandleClose((HOBJECT)s_textFont);
  HandleClose((HOBJECT)s_layerBackground);
  HandleClose((HOBJECT)s_layerText);
  while ((line = s_linelist.Head()) != 0) {
    FREEIFUSED(line->buffer);
    if (line->fontPointer) {
      GxuFontDestroyString(line->fontPointer);
    }
    s_linelist.DeleteNode(line);
  }
}

int __fastcall ConsoleIsActive() {
  return s_active;
}

void __fastcall ConsoleSetTitle(const char *title) {
  void *window = (void *)GxDevWindow();

  if (window) {
    OsGuiSetWindowTitle(window, title ? title : "");
  }
}

void __fastcall ConsoleWrite(const char *str, COLOR_T color) {
  CONSOLELINE *line;
  CONSOLELINE *head;
  unsigned int length;

  if (!str || !str[0] || !s_device || !s_textFont) {
    return;
  }

  s_critsect.Enter();
  line = (CONSOLELINE *)SMemAlloc(sizeof(CONSOLELINE), typeid(CONSOLELINE).raw_name(), SERR_LINECODE_OBJECT, SMEM_FLAG_ZEROMEMORY);
  if (line) {
    new (line) CONSOLELINE;
  }

  head = s_linelist.Head();
  if (head && head->inputpos) {
    s_linelist.LinkNode(line, 1, head);
  } else {
    s_linelist.LinkNode(line, 1, 0);
  }

  length = SStrLen(str);
  line->chars = length + 1;
  line->charsalloc = length + 1;
  line->buffer = (char *)ALLOC(length + 1);
  SStrCopy(line->buffer, str, 0x7FFFFFFF);
  line->colorType = color;
  GenerateNodeString(line);
  ++s_NumLines;
  EnforceMaxLines();
  s_critsect.Leave();
}

void __cdecl ConsoleWriteA(const char *str, COLOR_T color, ...) {
  va_list arglist;
  char    buf[0x400];

  va_start(arglist, color);
  if (str && str[0] && s_device) {
    SStrVPrintf(buf, sizeof(buf), str, arglist);
    ConsoleWrite(buf, color);
  }
}

void __cdecl ConsolePrintf(const char *str, ...) {
  va_list arglist;
  char    buf[0x400];

  va_start(arglist, str);
  if (str && str[0] && s_device) {
    SStrVPrintf(buf, sizeof(buf), str, arglist);
    ConsoleWrite(buf, DEFAULT_COLOR);
  }
}

void __fastcall ConsoleWriteV(const char *str, va_list arglist) {
  char buf[0x400];

  if (str && str[0] && s_device) {
    SStrVPrintf(buf, sizeof(buf), str, arglist);
    ConsoleWrite(buf, DEFAULT_COLOR);
  }
}

void __fastcall ConsolePostClose() {
  EventPostCloseEx(EventGetCurrentContext());
}

void __fastcall ConsoleCommandExecute(const char *commandLine, int addToHistory) {
  const char     *command;
  const char     *arguments;
  const char     *history;
  CONSOLECOMMAND *entry;

  if ((g_ExecCreateMode == EM_PROMPTOVERWRITE || g_ExecCreateMode == EM_RECORDING || g_ExecCreateMode == EM_APPEND) &&
      !AddLineToExecFile(commandLine))
  {
    return;
  }

  if (addToHistory) {
    history = ConsoleCommandHistory(0);
    if (!history || SStrCmp(commandLine, history, 0x7FFFFFFF)) {
      AddToHistory(commandLine);
    }
  }

  entry = ParseCommand(commandLine, &command, &arguments);
  if (entry) {
    entry->m_handler(command, arguments);
    return;
  }

  if (g_defaultCommand) {
    if (g_defaultCommand(command, arguments)) {
      return;
    }
  }

  entry = g_consoleCommandHash.Ptr("run");
  if (entry) {
    entry->m_handler("", commandLine);
    return;
  }

  ConsoleWrite("Unknown command", DEFAULT_COLOR);
}
