#include "Base.h"
#include "RCString.h"

void __fastcall BaseFileInitialize();
void __fastcall BaseFileDestroy();
void __fastcall HandleInitialize();
void __fastcall HandleDestroy();

void __fastcall BaseInitializeGlobal() {
  PropInitialize();
  BaseFileInitialize();
}

void __fastcall BaseDestroyGlobal() {
  BaseFileDestroy();
  PropDestroy();
  CStringManager::DestroyManager();
}

void __fastcall BaseInitializeContext() {
  HandleInitialize();
}

void __fastcall BaseDestroyContext() {
  HandleDestroy();
}
