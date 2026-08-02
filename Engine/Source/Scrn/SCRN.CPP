#include <Base/Base.h>

#include "Scrn.h"

static int consoleInitialized;

void __fastcall ILayerDestroy();
void __fastcall ILayerInitialize();
void __fastcall ILayerPaint();
void __fastcall IPerfDestroy();
void __fastcall IPerfInitialize();
void __fastcall IStockDestroy();
void __fastcall IStockInitialize();

void __fastcall ScrnDestroy() {
  ILayerDestroy();
  IPerfDestroy();
  IStockDestroy();
}

void __fastcall ScrnInitialize(int initConsole) {
  ILayerInitialize();
  consoleInitialized = initConsole;
  IPerfInitialize();
  IStockInitialize();
}

void __fastcall ScrnPaint() {
  ILayerPaint();
}
