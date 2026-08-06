#include <Base/Base.h>

#include "CDataAllocator.h"

#include <string.h>

CDataAllocator::CDataAllocator(DWORD bytesPerData, DWORD dataPerBlock) {
  m_bytesPerData = bytesPerData < 4 ? 4 : bytesPerData;
  m_dataPerBlock = dataPerBlock < 1 ? 1 : dataPerBlock;
  m_dataUsed = 0;
  m_blockList = 0;
  m_dataList = 0;
}

CDataAllocator::~CDataAllocator() {
}

void CDataAllocator::Clear(LPCSTR fileName, int lineNumber) {
  UINT dataUsed = m_dataUsed;

  if (dataUsed) {
    if (!fileName) {
      fileName = __FILE__;
      lineNumber = __LINE__;
    }

    SErrDisplayErrorFmt(
        STORM_ERROR_MEMORY_NEVER_RELEASED, __FILE__, __LINE__, TRUE, 1, "CDataAllocator@0x%08x: leaked %u: %s(%d) [%u][%u]", this, dataUsed, fileName,
        lineNumber, m_dataPerBlock, m_bytesPerData
    );
  }

  while (m_blockList) {
    Block *block = m_blockList;
    m_blockList = block->m_next;
    SMemFree(block, 0, 0, 0);
  }

  m_dataUsed = 0;
  m_dataList = 0;
}

LPVOID CDataAllocator::GetData(int zero, LPCSTR fileName, int lineNumber) {
  if (m_dataPerBlock == 1) {
    fileName = 0;
    lineNumber = 0;
  }

  if (!m_dataList) {
    UINT   index;
    Block *block;
    Data  *data;

    if (!fileName) {
      fileName = __FILE__;
      lineNumber = __LINE__;
    }

    block = static_cast<Block *>(SMemAlloc(m_dataPerBlock * m_bytesPerData + sizeof(Block), fileName, lineNumber, 0));
    data = reinterpret_cast<Data *>(block + 1);
    m_dataList = data;

    for (index = 0; index < m_dataPerBlock - 1; ++index) {
      Data *next = reinterpret_cast<Data *>(reinterpret_cast<BYTE *>(data) + m_bytesPerData);
      data->m_next = next;
      data = next;
    }

    data->m_next = 0;
    block->m_next = m_blockList;
    m_blockList = block;
  }

  Data *data = m_dataList;
  m_dataList = data->m_next;
  if (zero) {
    memset(data, 0, m_bytesPerData);
  }
  ++m_dataUsed;
  return data;
}

void CDataAllocator::PutData(LPVOID data, LPCSTR fileName, int lineNumber) {
  ASSERT(m_dataUsed > 0);

  Data *allocatorData = static_cast<Data *>(data);
  allocatorData->m_next = m_dataList;
  m_dataList = allocatorData;
  --m_dataUsed;
}
