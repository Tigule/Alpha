#pragma once

#include <storm.h>

struct CGxFont;
struct HLAYER__;
struct HTEXTFONT__;

enum SCRNSTOCK {
  STOCK_SYSFONT = 0,
  STOCK_PERFFONT = 1,
  SCRNSTOCKOBJECTS = 2
};

enum PERF_REMOTE_MODE {
  PERF_REMOTE_MODE__ONPAINT = 0,
  PERF_REMOTE_MODE__ADVANCESYNCGAMETIME = 1,
  PERF_REMOTE_MODE_MAX = 2
};

void __fastcall ScrnInitialize(int initConsole);
void __fastcall ScrnDestroy();
void __fastcall ScrnPaint();

void __fastcall ScrnLayerCreate(
    const RECTF  *rect,
    float         zorder,
    unsigned long flags,
    void         *param,
    void(__fastcall *paintfunc)(void *, const RECTF *, const RECTF *, float),
    HLAYER__ **layer
);
void __fastcall         ScrnLayerSetRect(HLAYER__ *layer, const RECTF *rect);
unsigned int __fastcall ScrnLayerGetFlags(HLAYER__ *layer);
void __fastcall         ScrnLayerSetFlags(HLAYER__ *layer, unsigned int flags);
void __fastcall         ScrnLayerDisable(int disable);
int __fastcall          ScrnLayerIsDisabled();
void __fastcall         ScrnScreenshot(void(__fastcall *callback)(int));

HTEXTFONT__ *__fastcall ScrnGetStockFont(SCRNSTOCK id);
void __fastcall         ScrnSetStockFont(SCRNSTOCK stockID, const char *fontTexturePath);
float __fastcall        ScrnGetStockFontHeight(SCRNSTOCK stockID);

void __fastcall             ScrnPerfEnable(int enable);
int __fastcall              ScrnPerfIsEnabled();
int __fastcall              ScrnPerfRemoteShutdown();
int __fastcall              ScrnPerfRemoteStartup();
void __fastcall             ScrnPerfRemoteSetMode(PERF_REMOTE_MODE mode);
PERF_REMOTE_MODE __fastcall ScrnPerfRemoteGetMode();
void __fastcall             ScrnPerfRemoteLogString(const char *prompt);
void __fastcall             ScrnPerfRemoteLogPerfCounters();
void __fastcall             ScrnPerfSetTextFunction(void(__fastcall *inFunc)(char *, int, void *), void *inParam);
void __fastcall             ScrnPerfToggleDisplayedValues();
void __fastcall             ScrnPerfResetTimePeaks();
