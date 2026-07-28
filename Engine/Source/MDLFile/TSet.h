#ifndef ENGINE_SOURCE_MDLFILE_TSET_H
#define ENGINE_SOURCE_MDLFILE_TSET_H

class CMDLStatus;

class TSet {
 public:
  TSet() : count(0) {}

  void Add(unsigned int token, int needed, int allowDuplicates);
  int Check(unsigned int token);
  int Found(unsigned int token);
  int NotFound(unsigned int token);
  void Complete(CMDLStatus *status);

 private:
  struct {
    unsigned int token;
    int needed;
    int dupsOk;
    int seen;
  } set[64];

  int count;
};

#endif
