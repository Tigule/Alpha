#include "Base.h"
#include "RCString.h"

void BaseFileInitialize();
void BaseFileDestroy();
void HandleInitialize();
void HandleDestroy();

void BaseInitializeGlobal() {
  PropInitialize();
  BaseFileInitialize();
}

void BaseDestroyGlobal() {
  BaseFileDestroy();
  PropDestroy();
  CStringManager::DestroyManager();
}

void BaseInitializeContext() {
  HandleInitialize();
}

void BaseDestroyContext() {
  HandleDestroy();
}
