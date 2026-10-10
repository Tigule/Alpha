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

enum {
  MDLFILE_TEXT = 0,
  MDLFILE_BIN = 1,
  NUM_MDLFILE_TYPES = 2
};

class CMdlScanner : public mdl_scan {
  CMDLStatus *m_status;

 public:
  CMdlScanner(CMDLStatus *status, LPCSTR input, int size) : mdl_scan(input, size), m_status(status) {
  }

  virtual void __cdecl mdlerror(char *format, ...);
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
  CMdlScanner scanner(status, (LPCSTR)buffer, 255);
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

  UINT  lastOffset = 0;
  DWORD lastSectionTag = 0;
  while (totalLength < size) {
    DWORD sectionTag = buf.GetDword();
    UINT  sectionLength = buf.GetUint();
    totalLength += 8;
    if (sectionLength) {
      if ((int)sectionLength > buf.Bytes()) {
        status->Add(STATUS_FATAL, "Section length was greater than bytes remaining in file.\n");
        status->Add(
            STATUS_FATAL, "Section failed after section '%c%c%c%c' starting at offset %u\n",
            ((BYTE *)&lastSectionTag)[0] ? ((BYTE *)&lastSectionTag)[0] : ' ',
            ((BYTE *)&lastSectionTag)[1] ? ((BYTE *)&lastSectionTag)[1] : ' ',
            ((BYTE *)&lastSectionTag)[2] ? ((BYTE *)&lastSectionTag)[2] : ' ',
            ((BYTE *)&lastSectionTag)[3] ? ((BYTE *)&lastSectionTag)[3] : ' ', lastOffset
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
  if (totalLength > size) {
    status->Add(STATUS_FATAL, "MDLFile overran total file size.\n");
    return 0;
  }
  return ReadObjectPtrs(&data, status);
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
  if (fclose(file)) {
    return 0;
  }
  return written == bytes;
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
  switch (type) {
    case MDLFILE_BIN:
      SStrPack(path, ".mdl", MAX_PATH);
      type = MDLFILE_TEXT;
      break;
    case MDLFILE_TEXT:
      SStrPack(path, ".mdx", MAX_PATH);
      type = MDLFILE_BIN;
      break;
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
  if (extension && SStrLen(extension) == 4 && !SStrCmpI(extension, ".md", 3)) {
    switch (extension[3]) {
      case 'l':
      case 'L':
        return MDLFILE_TEXT;
      case 'x':
      case 'X':
        return MDLFILE_BIN;
    }
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
    CMsgBuffer buffer;
    if (!ModelDataToBin(mdldata, buffer, status)) {
      return 0;
    }
    int bytes = buffer.Bytes();
    if (IWriteFile(path, "wb", buffer.GetData(bytes), bytes)) {
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
  VALIDATEBEGIN;
  VALIDATE(extension);
  VALIDATEENDVOID;
  s_defaultWriteFormat = DiscoverFileType(extension);
}

int MDLFileWrite(LPCSTR path, const MDLDATA &mdldata, CStatus *status) {
  VALIDATEBEGIN;
  VALIDATE(path);
  VALIDATE(path[0]);
  VALIDATEEND;
  if (!status) {
    status = &s_nullStatus;
  }
  return IWriteMdlFile(path, mdldata, (CMDLStatus *)status);
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
  DWORD size = 0;
  UINT  type = DiscoverFileType(path);
  if (type == NUM_MDLFILE_TYPES) {
    status->FatalBadFileName(path);
    return 0;
  }

  LPVOID fileData = LoadMdlData(path, &size);
  if (!fileData) {
    type = PickAlternateFilename(path, type);
    fileData = LoadMdlData(path, &size);
    if (!fileData) {
      FileReadError(path, status);
      return 0;
    }
  }

  int result;
  switch (type) {
    case MDLFILE_BIN: {
      CMsgBuffer buf;
      buf.SetData((BYTE *)fileData, size, 0);
      result = BinToModelData(buf, size, *mdldata, status);
      if (buf.Bytes()) {
        result = 0;
        buf.GetData(buf.Bytes());
      }
      break;
    }
    default:
      result = TextToModelData(fileData, *mdldata, status);
      break;
  }
  SMemFree(fileData, __FILE__, __LINE__, 0);
  return result;
}

BOOL MDLFileRead(LPCSTR path, MDLDATA *mdldata, CStatus *status) {
  VALIDATEBEGIN;
  VALIDATE(path && SStrLen(path));
  VALIDATE(mdldata);
  VALIDATEEND;
  if (!status) {
    status = &s_nullStatus;
  }

  SStrCopy(mdldata->header.sourceFilename, path, 0x7FFFFFFF);
  if (ReadMdlFile(mdldata->header.sourceFilename, mdldata, (CMDLStatus *)status)) {
    return 1;
  }
  status->Prepend(status->GetHighestSeverity(), "%s\n", path);
  return 0;
}

BYTE *MDLFileBinaryLoad(char *path, UINT *fileBytes, CStatus *status) {
  VALIDATEBEGIN;
  VALIDATE(path);
  VALIDATEEND;

  if (!status) {
    status = &s_nullStatus;
  }

  BYTE *fileData = (BYTE *)LoadMdlData(path, (DWORD *)fileBytes);
  if (!fileData) {
    FileReadError(path, (CMDLStatus *)status);
    return 0;
  }

  if (*(UINT *)fileData != 'XLDM') {
    SFile::Unload(fileData);
    status->Add(STATUS_FATAL, "%s\nFile is not a binary model file.\n", path);
    return 0;
  }

  *fileBytes -= sizeof(UINT);
  return fileData + sizeof(UINT);
}

BYTE *MDLFileBinaryLoad(LPCSTR path, UINT *fileBytes, CStatus *status) {
  VALIDATEBEGIN;
  VALIDATE(path);
  VALIDATEEND;

  if (!status) {
    status = &s_nullStatus;
  }

  BYTE *fileData = (BYTE *)LoadMdlData(path, (DWORD *)fileBytes);
  if (!fileData) {
    FileReadError(path, (CMDLStatus *)status);
    return 0;
  }

  if (*(UINT *)fileData != 'XLDM') {
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
  VALIDATEBEGIN;
  VALIDATE(fileData);
  VALIDATEEND;

  BYTE *fileEnd = fileData + fileBytes;
  while (fileData < fileEnd) {
    UINT tag = *(UINT *)fileData;
    fileData += sizeof(UINT);
    if (sectionTag == tag) {
      return fileData;
    }

    UINT sectionBytes = *(UINT *)fileData;
    fileData += sizeof(UINT) + sectionBytes;
  }

  return 0;
}

int MDLFileBinaryWrite(LPCSTR path, const BYTE *fileData, UINT fileBytes) {
  return IWriteFile(path, "wb", fileData - 4, fileBytes + 4);
}
