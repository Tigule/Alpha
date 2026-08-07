#include "MDLStatus.h"
#include "MDLTypes.h"
#include "lex.h"
#include "Base/MsgBuffer.h"

#include <storm.h>
#include <stdio.h>
#include <stdarg.h>

namespace MDL {

  void InitializeTokenText();
  void DestroyTokenText();
  BOOL CallTextWriteHandlers(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL CallBinWriteHandlers(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int  CallBinReadHandler(DWORD, CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int  CallTextReadHandler(UINT, mdl_scan &, MDLDATA &, CMDLStatus *);

}  // namespace MDL

BOOL  ReadObjectPtrs(MDLDATA *data, CMDLStatus *status);
char *OsGetLastErrorStr();
void  OsFreeLastErrorStr(char *msgBuf);

class CMdlScanner : public mdl_scan {
 public:
  CMdlScanner(CMDLStatus *status, LPCSTR input, int size) : mdl_scan(input, size), m_status(status) {
  }

  virtual void __cdecl mdlerror(char *format, ...);

 private:
  CMDLStatus *m_status;
};

void __cdecl CMdlScanner::mdlerror(char *format, ...) {
  static char buffer[256];
  va_list     args;
  va_start(args, format);
  SStrVPrintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  m_status->Add(STATUS_FATAL, "Error (line %d): %s\n", mdllineno, buffer);
}

static CNullStatus s_nullStatus;
static UINT        s_defaultWriteFormat = 1;

static int TextToModelData(LPCVOID buffer, MDLDATA &data, CMDLStatus *status) {
  CMdlScanner scanner(status, static_cast<LPCSTR>(buffer), 255);
  UINT        token = scanner.mdllex();
  while (token) {
    if (!MDL::CallTextReadHandler(token, scanner, data, status)) {
      return 0;
    }
    token = scanner.mdllex();
  }
  return ReadObjectPtrs(&data, status);
}

static int BinToModelData(CMsgBuffer &buf, UINT size, MDLDATA &data, CMDLStatus *status) {
  ASSERT(status);
  UINT totalLength = 4;
  if (buf.GetDword() != 'XLDM') {
    status->Add(STATUS_FATAL, "File is not a binary model file.\n");
    return 0;
  }

  DWORD lastSectionTag = 0;
  UINT  lastOffset = 0;
  while (totalLength < size) {
    DWORD sectionTag = buf.GetDword();
    UINT  sectionLength = buf.GetUint();
    totalLength += 8;
    if (sectionLength) {
      if (sectionLength > static_cast<UINT>(buf.Bytes())) {
        status->Add(STATUS_FATAL, "Section length was greater than bytes remaining in file.\n");
        status->Add(
            STATUS_FATAL, "Section failed after section '%c%c%c%c' starting at offset %u\n", lastSectionTag ? static_cast<char>(lastSectionTag) : ' ',
            lastSectionTag >> 8 ? static_cast<char>(lastSectionTag >> 8) : ' ', lastSectionTag >> 16 ? static_cast<char>(lastSectionTag >> 16) : ' ',
            lastSectionTag >> 24 ? static_cast<char>(lastSectionTag >> 24) : ' ', lastOffset
        );
        return 0;
      }
      if (!MDL::CallBinReadHandler(sectionTag, buf, sectionLength, data, status)) {
        return 0;
      }
      totalLength += sectionLength;
    }
    lastSectionTag = sectionTag;
    lastOffset = buf.GetReadPosition();
  }
  if (totalLength <= size) {
    return ReadObjectPtrs(&data, status);
  }
  status->Add(STATUS_FATAL, "MDLFile overran total file size.\n");
  return 0;
}

static int ModelDataToText(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *status) {
  return MDL::CallTextWriteHandlers(data, buffer, status);
}

static int ModelDataToBin(const MDLDATA &data, CMsgBuffer &buffer, CMDLStatus *status) {
  buffer.AddDword('XLDM');
  return MDL::CallBinWriteHandlers(data, buffer, status);
}

static BOOL IWriteFile(LPCSTR path, LPCSTR mode, LPCVOID data, UINT bytes) {
  FILE *file = fopen(path, mode);
  if (!file) {
    return 0;
  }
  UINT written = fwrite(data, 1, bytes, file);
  return !fclose(file) && written == bytes;
}

static void FileReadError(LPCSTR path, CMDLStatus *status) {
  char lpMsgBuf[256];

  SErrGetErrorStr(SErrGetLastError(), lpMsgBuf, sizeof(lpMsgBuf));
  status->Add(STATUS_FATAL, "%s: %s\n", path, lpMsgBuf);
}

static UINT PickAlternateFilename(char *path, UINT type) {
  char *extension = SStrChrR(path, '.');
  if (extension) {
    *extension = 0;
  }
  if (type == 0) {
    SStrPack(path, ".mdx", MAX_PATH);
    return 1;
  }
  if (type == 1) {
    SStrPack(path, ".mdl", MAX_PATH);
    return 0;
  }
  return type;
}

static void FileWriteError(LPCSTR path, CMDLStatus *status) {
  char *errorText = OsGetLastErrorStr();
  status->Add(STATUS_FATAL, "%s: %s\n", path, errorText);
  OsFreeLastErrorStr(errorText);
}

static UINT DiscoverFileType(LPCSTR path) {
  LPCSTR extension = SStrChrR(path, '.');
  if (!extension || SStrLen(extension) != 4 || SStrCmpI(extension, ".md", 3)) {
    return s_defaultWriteFormat;
  }
  if (extension[3] == 'l' || extension[3] == 'L') {
    return 0;
  }
  if (extension[3] == 'x' || extension[3] == 'X') {
    return 1;
  }
  return s_defaultWriteFormat;
}

void MDLFileInitialize() {
  MDL::InitializeTokenText();
}

void MDLFileDestroy() {
  MDL::DestroyTokenText();
}

static BOOL IWriteMdlFile(LPCSTR path, const MDLDATA &mdldata, CMDLStatus *status) {
  if (DiscoverFileType(path)) {
    CMsgBuffer buffer(1);
    if (!ModelDataToBin(mdldata, buffer, status)) {
      return 0;
    }
    int     bytes = buffer.Bytes();
    LPCVOID data = buffer.GetData(bytes);
    if (IWriteFile(path, "wb", data, bytes)) {
      return 1;
    }
  } else {
    TSGrowableArray<char> buffer;
    buffer.SetChunkSize(0x100000);
    buffer.ReserveSpace(0x400000);
    if (!ModelDataToText(mdldata, buffer, status)) {
      return 0;
    }
    if (IWriteFile(path, "wt", buffer.Ptr(), buffer.Count())) {
      return 1;
    }
  }
  FileWriteError(path, status);
  return 0;
}

void MDLFileSetDefaultWriteFormat(LPCSTR extension) {
  FATALASSERT(extension);
  s_defaultWriteFormat = DiscoverFileType(extension);
}

int MDLFileWrite(LPCSTR path, const MDLDATA &mdldata, CStatus *status) {
  FATALASSERT(path);
  FATALASSERT(path[0]);
  if (!status) {
    status = &s_nullStatus;
  }
  return IWriteMdlFile(path, mdldata, static_cast<CMDLStatus *>(status));
}

static LPVOID LoadMdlData(char *path, DWORD *bytes) {
  SFile *fileHandle;

  if (!SFile::Open(path, &fileHandle)) {
    return 0;
  }

  int success = SFile::GetActualFileName(fileHandle, path, MAX_PATH);
  ASSERT(success);

  UINT   fileBytes = SFile::GetFileSize(fileHandle, 0);
  LPVOID fileData = SMemAlloc(fileBytes + 1, __FILE__, __LINE__, 0x8);

  if (!SFile::Read(fileHandle, fileData, fileBytes, bytes, 0, 0)) {
    SMemFree(fileData, __FILE__, __LINE__, 0);
    return 0;
  }

  SFile::Close(fileHandle);
  return fileData;
}

static LPVOID LoadMdlData(LPCSTR path, DWORD *bytes) {
  SFile *fileHandle;

  if (!SFile::Open(path, &fileHandle)) {
    return 0;
  }

  UINT   fileBytes = SFile::GetFileSize(fileHandle, 0);
  LPVOID fileData = SMemAlloc(fileBytes + 1, __FILE__, __LINE__, 0x8);

  if (!SFile::Read(fileHandle, fileData, fileBytes, bytes, 0, 0)) {
    SMemFree(fileData, __FILE__, __LINE__, 0);
    return 0;
  }

  SFile::Close(fileHandle);
  return fileData;
}

static int ReadMdlFile(char *path, MDLDATA *mdldata, CMDLStatus *status) {
  UINT type = DiscoverFileType(path);
  if (type == 2) {
    status->Add(STATUS_FATAL, "%s: bad model file name.\n", path);
    return 0;
  }

  DWORD  size = 0;
  LPVOID fileData = LoadMdlData(path, &size);
  if (!fileData) {
    type = PickAlternateFilename(path, type);
    fileData = LoadMdlData(path, &size);
  }
  if (!fileData) {
    FileReadError(path, status);
    return 0;
  }

  int result;
  if (type == 1) {
    CMsgBuffer buf(0);
    buf.SetData(static_cast<BYTE *>(fileData), size, 0);
    result = BinToModelData(buf, size, *mdldata, status);
    if (buf.Bytes()) {
      result = 0;
      buf.GetData(buf.Bytes());
    }
  } else {
    static_cast<char *>(fileData)[size] = 0;
    result = TextToModelData(fileData, *mdldata, status);
  }
  SMemFree(fileData, __FILE__, __LINE__, 0);
  return result;
}

BOOL MDLFileRead(LPCSTR path, MDLDATA *mdldata, CStatus *status) {
  FATALASSERT(path && SStrLen(path));
  FATALASSERT(mdldata);
  if (!status) {
    status = &s_nullStatus;
  }

  SStrCopy(mdldata->header.sourceFilename, path, 0x7FFFFFFF);
  if (ReadMdlFile(mdldata->header.sourceFilename, mdldata, static_cast<CMDLStatus *>(status))) {
    return 1;
  }
  status->Prepend(status->GetHighestSeverity(), "%s:\n", path);
  return 0;
}

BYTE *MDLFileBinaryLoad(char *path, UINT *fileBytes, CStatus *status) {
  ASSERT(path);

  if (!status) {
    status = &s_nullStatus;
  }

  BYTE *fileData = static_cast<BYTE *>(LoadMdlData(path, reinterpret_cast<DWORD *>(fileBytes)));
  if (!fileData) {
    FileReadError(path, static_cast<CMDLStatus *>(status));
    return 0;
  }

  if (*reinterpret_cast<UINT *>(fileData) != 'XLDM') {
    SFile::Unload(fileData);
    status->Add(STATUS_FATAL, "%s\nFile is not a binary model file.\n", path);
    return 0;
  }

  *fileBytes -= sizeof(UINT);
  return fileData + sizeof(UINT);
}

BYTE *MDLFileBinaryLoad(LPCSTR path, UINT *fileBytes, CStatus *status) {
  ASSERT(path);

  if (!status) {
    status = &s_nullStatus;
  }

  BYTE *fileData = static_cast<BYTE *>(LoadMdlData(path, reinterpret_cast<DWORD *>(fileBytes)));
  if (!fileData) {
    FileReadError(path, static_cast<CMDLStatus *>(status));
    return 0;
  }

  if (*reinterpret_cast<UINT *>(fileData) != 'XLDM') {
    SFile::Unload(fileData);
    status->Add(STATUS_FATAL, "%s\nFile is not a binary model file.\n", path);
    return 0;
  }

  *fileBytes -= sizeof(UINT);
  return fileData + sizeof(UINT);
}

void MDLFileBinaryUnload(BYTE *fileData) {
  SFile::Unload(fileData - sizeof(UINT));
}

BYTE *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag) {
  FATALASSERT(fileData);

  BYTE *fileEnd = fileData + fileBytes;
  while (fileData < fileEnd) {
    UINT tag = *reinterpret_cast<UINT *>(fileData);
    fileData += sizeof(UINT);
    if (tag == sectionTag) {
      return fileData;
    }

    UINT sectionBytes = *reinterpret_cast<UINT *>(fileData);
    fileData += sizeof(UINT) + sectionBytes;
  }

  return 0;
}

int MDLFileBinaryWrite(LPCSTR path, const BYTE *fileData, UINT fileBytes) {
  return IWriteFile(path, "wb", fileData - 4, fileBytes + 4);
}
