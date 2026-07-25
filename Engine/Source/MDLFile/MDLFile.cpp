#include "MDLStatus.h"
#include "MDLTypes.h"

#include <storm.h>
#include <stdio.h>

class CMsgBuffer {
 public:
  CMsgBuffer(unsigned int count)
      : m_alloc(count), m_freeData(count != 0), m_read(0), m_write(0),
        m_data(count ? static_cast<unsigned char *>(SMemAlloc(count, __FILE__, __LINE__, 0)) : 0) {
  }
  ~CMsgBuffer() {
    if (m_freeData && m_data) {
      SMemFree(m_data, __FILE__, __LINE__, 0);
    }
  }
  int Bytes() {
    return m_write - m_read;
  }
  unsigned int GetReadPosition() {
    return m_read;
  }
  void SetData(unsigned char *data, unsigned int count, int freeData) {
    if (m_freeData && m_data) {
      SMemFree(m_data, __FILE__, __LINE__, 0);
    }
    m_alloc = count;
    m_freeData = freeData;
    m_read = 0;
    m_write = count;
    m_data = data;
  }
  void AddDword(unsigned long value) {
    if (m_write + sizeof(value) > m_alloc) {
      unsigned int newAlloc = m_alloc ? m_alloc * 2 : 256;
      while (newAlloc < m_write + sizeof(value)) {
        newAlloc *= 2;
      }
      unsigned char *newData = static_cast<unsigned char *>(SMemAlloc(newAlloc, __FILE__, __LINE__, 0));
      if (m_write) {
        memcpy(newData, m_data, m_write);
      }
      if (m_freeData && m_data) {
        SMemFree(m_data, __FILE__, __LINE__, 0);
      }
      m_data = newData;
      m_alloc = newAlloc;
      m_freeData = 1;
    }
    *reinterpret_cast<unsigned long *>(m_data + m_write) = value;
    m_write += sizeof(value);
  }
  unsigned long GetDword() {
    FATALASSERT(m_read + sizeof(unsigned long) <= m_write);
    unsigned long value = *reinterpret_cast<unsigned long *>(m_data + m_read);
    m_read += sizeof(value);
    return value;
  }
  unsigned int GetUint() {
    return GetDword();
  }
  const void *GetData(int count) {
    FATALASSERT(count >= 0 && m_read + static_cast<unsigned int>(count) <= m_write);
    const void *data = m_data + m_read;
    m_read += count;
    return data;
  }

 private:
  unsigned int   m_alloc;
  int            m_freeData;
  unsigned int   m_read;
  unsigned int   m_write;
  unsigned char *m_data;
};

namespace MDL {

  void __fastcall InitializeTokenText();
  void __fastcall DestroyTokenText();

}  // namespace MDL

static CNullStatus s_nullStatus;
static unsigned int s_defaultWriteFormat;

static int TextToModelData(const void* buffer, MDLDATA& data, CMDLStatus* status) {
  status->Add(STATUS_FATAL, "Text model parsing is unavailable.\n");
  return 0;
}

static int BinToModelData(CMsgBuffer& buf, unsigned int size, MDLDATA& data, CMDLStatus* status) {
  ASSERT(status);
  unsigned int totalLength = 4;
  if (buf.GetDword() != 0x584C444D) {
    status->Add(STATUS_FATAL, "File is not a binary model file.\n");
    return 0;
  }

  unsigned long lastSectionTag = 0;
  unsigned int lastOffset = 0;
  while (totalLength < size) {
    unsigned long sectionTag = buf.GetDword();
    unsigned int sectionLength = buf.GetUint();
    totalLength += 8;
    if (sectionLength) {
      if (sectionLength > static_cast<unsigned int>(buf.Bytes())) {
        status->Add(STATUS_FATAL, "Section length was greater than remaining file size.\n");
        status->Add(
            STATUS_FATAL, "Section failed after %c%c%c%c at offset 0x%08X.\n",
            lastSectionTag ? static_cast<char>(lastSectionTag) : ' ',
            lastSectionTag >> 8 ? static_cast<char>(lastSectionTag >> 8) : ' ',
            lastSectionTag >> 16 ? static_cast<char>(lastSectionTag >> 16) : ' ',
            lastSectionTag >> 24 ? static_cast<char>(lastSectionTag >> 24) : ' ',
            lastOffset
        );
        return 0;
      }
      buf.GetData(sectionLength);
      totalLength += sectionLength;
    }
    lastSectionTag = sectionTag;
    lastOffset = buf.GetReadPosition();
  }
  if (totalLength <= size) {
    status->Add(STATUS_FATAL, "Binary model conversion is unavailable.\n");
    return 0;
  }
  status->Add(STATUS_FATAL, "Mdlfile overran end of file.\n");
  return 0;
}

static int ModelDataToBin(const MDLDATA& data, CMsgBuffer& buffer, CMDLStatus* status) {
  buffer.AddDword(0x584C444D);
  status->Add(STATUS_FATAL, "Binary model conversion is unavailable.\n");
  return 0;
}

static int IWriteFile(const char* path, const char* mode, const void* data, unsigned int bytes) {
  FILE *file = fopen(path, mode);
  if (!file) {
    return 0;
  }
  unsigned int written = fwrite(data, 1, bytes, file);
  return !fclose(file) && written == bytes;
}

static void __fastcall FileReadError(const char *path, CMDLStatus *status) {
  char errorText[256];

  SErrGetErrorStr(SErrGetLastError(), errorText, sizeof(errorText));
  status->Add(STATUS_FATAL, "%s: %s\n", path, errorText);
}

static unsigned int PickAlternateFilename(char* path, unsigned int type) {
  char *extension = SStrChrR(path, '.');
  if (extension) {
    *extension = 0;
  }
  if (type == 0) {
    SStrPack(path, ".mdx", 260);
    return 1;
  }
  if (type == 1) {
    SStrPack(path, ".mdl", 260);
    return 0;
  }
  return type;
}

static void FileWriteError(const char* path, CMDLStatus* status) {
  char errorText[256];
  SErrGetErrorStr(SErrGetLastError(), errorText, sizeof(errorText));
  status->Add(STATUS_FATAL, "%s: %s\n", path, errorText);
}

static unsigned int DiscoverFileType(const char* path) {
  const char *extension = SStrChrR(path, '.');
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

void __fastcall MDLFileInitialize() {
  MDL::InitializeTokenText();
}

void __fastcall MDLFileDestroy() {
  MDL::DestroyTokenText();
}

static int IWriteMdlFile(const char* path, const MDLDATA& mdldata, CMDLStatus* status) {
  if (DiscoverFileType(path)) {
    CMsgBuffer buffer(1);
    if (!ModelDataToBin(mdldata, buffer, status)) {
      return 0;
    }
    int bytes = buffer.Bytes();
    const void *data = buffer.GetData(bytes);
    if (IWriteFile(path, "wb", data, bytes)) {
      return 1;
    }
  } else {
    status->Add(STATUS_FATAL, "Text model writing is unavailable.\n");
    return 0;
  }
  FileWriteError(path, status);
  return 0;
}

void __fastcall MDLFileSetDefaultWriteFormat(const char* extension) {
  FATALASSERT(extension);
  s_defaultWriteFormat = DiscoverFileType(extension);
}

int __fastcall MDLFileWrite(const char* path, const MDLDATA& mdldata, CStatus* status) {
  FATALASSERT(path);
  FATALASSERT(path[0]);
  if (!status) {
    status = &s_nullStatus;
  }
  return IWriteMdlFile(path, mdldata, static_cast<CMDLStatus *>(status));
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
  unsigned int type = DiscoverFileType(path);
  if (type == 2) {
    status->Add(STATUS_FATAL, "%s: bad model file name.\n", path);
    return 0;
  }

  unsigned long size = 0;
  void *fileData = LoadMdlData(path, &size);
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
    CMsgBuffer buffer(0);
    buffer.SetData(static_cast<unsigned char *>(fileData), size, 0);
    result = BinToModelData(buffer, size, *mdldata, status);
    if (buffer.Bytes()) {
      result = 0;
      buffer.GetData(buffer.Bytes());
    }
  } else {
    static_cast<char *>(fileData)[size] = 0;
    result = TextToModelData(fileData, *mdldata, status);
  }
  SMemFree(fileData, __FILE__, __LINE__, 0);
  return result;
}

int __fastcall MDLFileRead(const char* path, MDLDATA* mdldata, CStatus* status) {
  FATALASSERT(path && SStrLen(path));
  FATALASSERT(mdldata);
  if (!status) {
    status = &s_nullStatus;
  }

  char actualPath[260];
  SStrCopy(actualPath, path, sizeof(actualPath));
  if (ReadMdlFile(actualPath, mdldata, static_cast<CMDLStatus *>(status))) {
    return 1;
  }
  status->Prepend(status->GetHighestSeverity(), "%s:\n", path);
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
  return IWriteFile(path, "wb", fileData - 4, fileBytes + 4);
}
