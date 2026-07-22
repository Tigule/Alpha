#include "Scrn.h"
const float PI = 3.14159265358979323846f;
const float TWO_PI = PI + PI;
const float OO_TWO_PI = 1.0f / TWO_PI;

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
