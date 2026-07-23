#include "MDLStatus.h"
#include "MDLTypes.h"

#include <storm.h>

class CMsgBuffer;

namespace MDL {

  void __fastcall InitializeTokenText();
  void __fastcall DestroyTokenText();

}  // namespace MDL

static CNullStatus s_nullStatus;

static int TextToModelData(const void* buffer, MDLDATA& data, CMDLStatus* status) {
    // TODO: implement
    return 0;
}

static int BinToModelData(CMsgBuffer& buf, unsigned int size, MDLDATA& data, CMDLStatus* status) {
    // TODO: implement
    return 0;
}

static int ModelDataToBin(const MDLDATA& data, CMsgBuffer& buffer, CMDLStatus* status) {
    // TODO: implement
    return 0;
}

static int IWriteFile(const char* path, const char* mode, const void* data, unsigned int bytes) {
    // TODO: implement
    return 0;
}

static void __fastcall FileReadError(const char *path, CMDLStatus *status) {
  char errorText[256];

  SErrGetErrorStr(SErrGetLastError(), errorText, sizeof(errorText));
  status->Add(STATUS_FATAL, "%s: %s\n", path, errorText);
}

static unsigned int PickAlternateFilename(char* path, unsigned int type) {
    // TODO: implement
    return 0;
}

static void FileWriteError(const char* path, CMDLStatus* status) {
    // TODO: implement
}

static unsigned int DiscoverFileType(const char* path) {
    // TODO: implement
    return 0;
}

void __fastcall MDLFileInitialize() {
  MDL::InitializeTokenText();
}

void __fastcall MDLFileDestroy() {
  MDL::DestroyTokenText();
}

static int IWriteMdlFile(const char* path, const MDLDATA& mdldata, CMDLStatus* status) {
    // TODO: implement
    return 0;
}

void __fastcall MDLFileSetDefaultWriteFormat(const char* extension) {
    // TODO: implement
}

int __fastcall MDLFileWrite(const char* path, const MDLDATA& mdldata, CStatus* status) {
    // TODO: implement
    return 0;
}

static void *__fastcall LoadMdlData(char *path, unsigned long *bytes) {
  SFile *file;

  if (!SFile::Open(path, &file)) {
    return 0;
  }

  int success = SFile::GetActualFileName(file, path, 260);
  ASSERT(success);

  unsigned int fileBytes = SFile::GetFileSize(file, 0);
  void        *fileData = SMemAlloc(fileBytes + 1, __FILE__, __LINE__, 0x8);

  if (!SFile::Read(file, fileData, fileBytes, bytes, 0, 0)) {
    SMemFree(fileData, __FILE__, __LINE__, 0);
    return 0;
  }

  SFile::Close(file);
  return fileData;
}

static int ReadMdlFile(char* path, MDLDATA* mdldata, CMDLStatus* status) {
    // TODO: implement
    return 0;
}

int __fastcall MDLFileRead(const char* path, MDLDATA* mdldata, CStatus* status) {
    // TODO: implement
    return 0;
}

unsigned char *__fastcall MDLFileBinaryLoad(char *path, unsigned int *fileBytes, CStatus *status) {
  ASSERT(path);

  if (!status) {
    status = &s_nullStatus;
  }

  unsigned char *fileData = static_cast<unsigned char *>(LoadMdlData(path, reinterpret_cast<unsigned long *>(fileBytes)));
  if (!fileData) {
    FileReadError(path, static_cast<CMDLStatus *>(status));
    return 0;
  }

  if (*reinterpret_cast<unsigned int *>(fileData) != 0x584C444D) {
    SFile::Unload(fileData);
    status->Add(STATUS_FATAL, "%s\nFile is not a binary model file.\n", path);
    return 0;
  }

  *fileBytes -= sizeof(unsigned int);
  return fileData + sizeof(unsigned int);
}

void __fastcall MDLFileBinaryUnload(unsigned char *fileData) {
  SFile::Unload(fileData - sizeof(unsigned int));
}

unsigned char *__fastcall MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag) {
  FATALASSERT(fileData);

  unsigned char *fileEnd = fileData + fileBytes;
  while (fileData < fileEnd) {
    unsigned int tag = *reinterpret_cast<unsigned int *>(fileData);
    fileData += sizeof(unsigned int);
    if (tag == sectionTag) {
      return fileData;
    }

    unsigned int sectionBytes = *reinterpret_cast<unsigned int *>(fileData);
    fileData += sizeof(unsigned int) + sectionBytes;
  }

  return 0;
}

int __fastcall MDLFileBinaryWrite(const char* path, const unsigned char* fileData, unsigned int fileBytes) {
    // TODO: implement
    return 0;
}
