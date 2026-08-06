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

void ScrnInitialize(int initConsole);
void ScrnDestroy();
void ScrnPaint();

void ScrnLayerCreate(
    const RECTF *rect,
    float        zorder,
    DWORD        flags,
    LPVOID       param,
    void (*paintfunc)(LPVOID, const RECTF *, const RECTF *, float),
    HLAYER__ **layer
);
void ScrnLayerSetRect(HLAYER__ *layer, const RECTF *rect);
UINT ScrnLayerGetFlags(HLAYER__ *layer);
void ScrnLayerSetFlags(HLAYER__ *layer, UINT flags);
void ScrnLayerDisable(int disable);
int  ScrnLayerIsDisabled();
void ScrnScreenshot(void (*callback)(int));

HTEXTFONT__ *ScrnGetStockFont(SCRNSTOCK id);
void         ScrnSetStockFont(SCRNSTOCK stockID, LPCSTR fontTexturePath);
float        ScrnGetStockFontHeight(SCRNSTOCK stockID);

void             ScrnPerfEnable(int enable);
int              ScrnPerfIsEnabled();
int              ScrnPerfRemoteShutdown();
int              ScrnPerfRemoteStartup();
void             ScrnPerfRemoteSetMode(PERF_REMOTE_MODE mode);
PERF_REMOTE_MODE ScrnPerfRemoteGetMode();
void             ScrnPerfRemoteLogString(LPCSTR prompt);
void             ScrnPerfRemoteLogPerfCounters();
void             ScrnPerfSetTextFunction(void (*inFunc)(char *, int, LPVOID), LPVOID inParam);
void             ScrnPerfToggleDisplayedValues();
void             ScrnPerfResetTimePeaks();
