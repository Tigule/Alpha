#pragma once

#include <Base/SFileExtras.h>
#include <DB/WowLocale.h>
#include <storm.h>
#include <new>
#include <string.h>

template <class RECORD>
class WowClientDB {
 public:
  WowClientDB() {
    Init();
  }

  ~WowClientDB() {
    Free();
  }

  void Load() {
    unsigned int signature;
    char        *stringBuffer;
    unsigned int stringSize;
    int          i;
    unsigned int rowSize, numColumns;
    SFile       *f;
    if (!SFile::Open(RECORD::GetFilename(), &f)) {
      FATALERROR(("Unable to open %s", RECORD::GetFilename()));
    }
    SFileReadTyped(f, &signature);
    if (signature != 0x43424457) {
      SFile::Close(f);
      return;
    }
    SFileReadTyped(f, &m_numRecords);
    if (!m_numRecords) {
      SFile::Close(f);
      return;
    }

    SFileReadTyped(f, &numColumns);
    if (numColumns != RECORD::GetNumColumns()) {
      FATALERROR(("%s has wrong number of columns (found %i, expected %i)", RECORD::GetFilename(), numColumns, RECORD::GetNumColumns()));
    }
    SFileReadTyped(f, &rowSize);
    if (rowSize != RECORD::GetRowSize()) {
      FATALERROR(("%s has wrong row size (found %i, expected %i)", RECORD::GetFilename(), rowSize, RECORD::GetRowSize()));
    }
    SFileReadTyped(f, &stringSize);
    m_records = static_cast<RECORD *>(ALLOC(sizeof(RECORD) * m_numRecords + stringSize));

    stringBuffer = reinterpret_cast<char *>(m_records + m_numRecords);
    m_maxID = 0;
    for (i = 0; i < m_numRecords; ++i) {
      new (&m_records[i]) RECORD;
      m_records[i].Read(f, stringBuffer);
      if (m_records[i].NeedIDAssigned()) {
        m_records[i].SetID(i);
      }
      m_maxID = m_records[i].GetID() > m_maxID ? m_records[i].GetID() : m_maxID;
    }
    if (!SFile::Read(f, stringBuffer, stringSize, 0, 0, 0)) {
      FATALERROR(("%s: Cannot read string table", RECORD::GetFilename()));
    }
    SFile::Close(f);
    m_recordsById = static_cast<RECORD **>(ALLOC(sizeof(RECORD *) * (m_maxID + 1)));
    memset(m_recordsById, 0, sizeof(RECORD *) * (m_maxID + 1));
    for (i = 0; i < m_numRecords; ++i) {
      m_recordsById[m_records[i].GetID()] = &m_records[i];
    }
    m_loaded = 1;
  }

  void Reload() {
    Free();
    Init();
    Load();
  }

  void Unload() {
    Free();
    Init();
  }

  RECORD *GetRecord(int id) {
    if (id < 0 || id > m_maxID) {
      return 0;
    }

    return m_recordsById[id];
  }

  int GetMaxID() {
    return m_maxID;
  }

  int GetNumRecords() {
    return m_numRecords;
  }

  RECORD *GetRecordByIndex(int index) {
    return index < 0 || index >= m_numRecords ? 0 : &m_records[index];
  }

 private:
  void Free() {
    if (m_loaded) {
      int i;
      for (i = 0; i < m_numRecords; ++i) {
        m_records[i].~RECORD();
      }
      FREE(m_records);
      delete[] m_recordsById;
    }
  }

  void Init() {
    m_records = 0;
    m_recordsById = 0;
    m_numRecords = 0;
    m_maxID = -1;
    m_loaded = 0;
  }

  RECORD  *m_records;
  int      m_numRecords;
  RECORD **m_recordsById;
  int      m_maxID;
  int      m_loaded;
};
