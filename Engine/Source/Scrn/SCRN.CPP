#include <Base/Base.h>

#include "Scrn.h"

static int consoleInitialized;

void ILayerDestroy();
void ILayerInitialize();
void ILayerPaint();
void IPerfDestroy();
void IPerfInitialize();
void IStockDestroy();
void IStockInitialize();

void ScrnDestroy() {
  ILayerDestroy();
  IPerfDestroy();
  IStockDestroy();
}

void ScrnInitialize(int initConsole) {
  ILayerInitialize();
  consoleInitialized = initConsole;
  IPerfInitialize();
  IStockInitialize();
}

void ScrnPaint() {
  ILayerPaint();
}
