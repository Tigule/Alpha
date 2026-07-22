#pragma once

#include <Services/Texture.h>
#include <Tempest/c2vector.h>
#include <Tempest/c3vector.h>
#include <Tempest/c44matrix.h>
#include <Tempest/caabox.h>
#include <Tempest/cimvector.h>
#include <stpl.h>
#include <WorldClient/World.h>

struct CHUNKDATA;

struct VERTDATA {
  NTempest::C3Vector p;
  NTempest::C2Vector t[2];
};

struct SPLATDATA {
  TSGrowableArray<VERTDATA>       data;
  TSGrowableArray<unsigned short> indices;
  int                             startTime;
  NTempest::C3Vector              position;
  unsigned int                    skip;
  NTempest::CImVector             color;
  CHUNKDATA                      *chunk;
  TSLink<SPLATDATA>               orderLink;
  TSLink<SPLATDATA>               normalLink;

  unsigned int Update(float progress, unsigned int &nuke);
  unsigned int Culled();
};

struct LISTBASE {
  TSExplicitList<SPLATDATA, 68>            m_splatOrder;
  TSList<CHUNKDATA, TSGetLink<CHUNKDATA> > m_chunks;
  HTEXTURE__                              *m_texture;
  int                                      m_currentCount;
  int                                      m_maxCount;
  int                                      m_flags;

  LISTBASE(int m, int f);
  virtual ~LISTBASE();
  void                 Render();
  void                 Add(NTempest::C3Vector &position, NTempest::CAaBox &box, NTempest::C44Matrix &matrix);
  void                 SetTexture(const char *n);
  CHUNKDATA           *FindChunk(int id);
  virtual unsigned int MakeSpace() = 0;
};

struct TIMEDTEXTURE : public LISTBASE {
  TIMEDTEXTURE() : LISTBASE(128, 0) {
  }
  unsigned int MakeSpace();
};

struct PERSISTENTTEXTURE : public LISTBASE {
  PERSISTENTTEXTURE();
  unsigned int MakeSpace();
};

struct CHUNKDATA : public TSLinkedNode<CHUNKDATA> {
  int                           m_sourceID;
  TSExplicitList<SPLATDATA, 76> m_splats;
  int                           m_flags;
  int                           m_vertCount;
  int                           m_indexCount;
  int                           m_numSplats;
  NTempest::C44Matrix           m_matrix;

  CHUNKDATA() : m_sourceID(0), m_flags(0), m_vertCount(0), m_indexCount(0), m_numSplats(0) {
  }
  ~CHUNKDATA();
  SPLATDATA *Add(CWTriData::Batch &batch, NTempest::CAaBox &box, NTempest::C44Matrix &basis);
  void       RecycleSplat(SPLATDATA *splat);
  void       Render();
  int        GetVertCount(CWTriData::Batch &batch, int &lowest, int &highest);
};
