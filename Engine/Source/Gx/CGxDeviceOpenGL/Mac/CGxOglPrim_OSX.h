#ifndef ENGINE_SOURCE_GX_CGXDEVICEOPENGL_MAC_CGXOGLPRIM_OSX_H
#define ENGINE_SOURCE_GX_CGXDEVICEOPENGL_MAC_CGXOGLPRIM_OSX_H

#include "../CGxDeviceOpenGl.h"

#include <gl/gl.h>

#define GL_VERTEX_ARRAY_RANGE_APPLE        0x851D
#define GL_VERTEX_ARRAY_STORAGE_HINT_APPLE 0x851F
#define GL_STORAGE_CACHED_APPLE            0x85BE

extern "C" {
void glVertexArrayRangeAPPLE(GLsizei length, GLvoid *pointer);
void glFlushVertexArrayRangeAPPLE(GLsizei length, GLvoid *pointer);
void glVertexArrayParameteriAPPLE(GLenum pname, GLint param);
void glGenFencesAPPLE(GLsizei n, GLuint *fences);
void glDeleteFencesAPPLE(GLsizei n, const GLuint *fences);
void glSetFenceAPPLE(GLuint fence);
void glFinishFenceAPPLE(GLuint fence);
void glFinishObjectAPPLE(GLenum object, GLint name);
void glBindVertexArrayAPPLE(GLuint array);
void glDeleteVertexArraysAPPLE(GLsizei n, const GLuint *arrays);
void glGenVertexArraysAPPLE(GLsizei n, GLuint *arrays);
}

enum EGxVxrBufferFlavor {
  kVxrBuffer_Prim = 0,
  kVxrBuffer_Dynamic = 1,
  kVxrBuffer_Index = 2,
  kVxrBufferCount = 3
};

struct SGxVxrBufferDef {
  UINT               sizeLog2;
  EGxVxrBufferFlavor flavor;
};

class CGxVxrRingBuffer {
 public:
  UINT_PTR m_baseAddress;
  UINT     m_sizeLog2;
  UINT     m_size;
  UINT     m_fence0;
  UINT     m_fence1;
  UINT     m_offset;

  CGxVxrRingBuffer(LPVOID baseAddress, UINT sizeLog2, UINT fence0, UINT fence1)
      : m_baseAddress((UINT_PTR)baseAddress), m_sizeLog2(sizeLog2), m_size(1 << sizeLog2), m_fence0(fence0), m_fence1(fence1), m_offset(0) {
    ASSERT(((uint)m_baseAddress & 0xFFF) == 0);
    ASSERT(sizeLog2 >= 12);
  }

  LPVOID Alloc(UINT bytes);
};

class CGxVxrManager {
 public:
  enum {
    kFenceCount = 2 * kVxrBufferCount
  };

  UINT              m_totalSize;
  LPVOID            m_unalignedBuf;
  LPBYTE            m_alignedBuf;
  GLuint            m_fences[kFenceCount];
  CGxVxrRingBuffer *m_bufs[kVxrBufferCount];
  int               m_rangeEnabled;

  CGxVxrManager(SGxVxrBufferDef *defs, int *success);
  ~CGxVxrManager();
  LPVOID Alloc(UINT bytes, EGxVxrBufferFlavor flavor);
  void   Set_GL_VERTEX_ARRAY_RANGE_APPLE(int enable);
};

struct CGxVaoDesc {
  int                   m_active;
  int                   m_index;
  GLuint                m_vao;
  UINT                  m_pageCount;
  UINT                  m_firstPage;
  UINT                  m_count;
  EGxVertexBufferFormat m_format;
  UINT                  m_vtxBytes;
  LPVOID                m_ptrs[GxVertexMembers_Last];
};

class CGxVaoManager {
 public:
  enum {
    kVaoCount = 16384,
    kPageCount = 16384,
    kPageSizeLog2 = 12
  };

  LPVOID     m_unalignedMem;
  LPBYTE     m_alignedMem;
  int        m_pageMap[kPageCount];
  UINT       m_pageMapAllocMark;
  CGxVaoDesc m_desc[kVaoCount];
  int        m_nextSlot;

  CGxVaoManager();
  ~CGxVaoManager();
  int  AllocBuf(UINT count, EGxVertexBufferFormat format);
  void FreeBuf(int index);
};

class CGxBufOgl : public CGxBuf {
  friend class CGxDeviceOpenGl;

  int    m_vaoToken;
  LPVOID vertexPtr[GxVertexMembers_Last];
  UINT   m_vtxBytes;
  WORD  *indexPtr;
  UINT   m_idxBytes;

 public:
  CGxBufOgl() : m_vaoToken(-1) {
  }
};

#endif
