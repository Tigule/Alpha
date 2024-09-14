#pragma once

class CDataStore {
 public:
  void Get(unsigned char *val);
};

void EngineHook_CDataStore();
