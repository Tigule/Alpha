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
  TSGrowableArray<VERTDATA> data;
  TSGrowableArray<WORD>     indices;
  int                       startTime;
  NTempest::C3Vector        position;
  bool                      skip;
  NTempest::CImVector       color;
  CHUNKDATA                *chunk;
  LINKDECLEX(SPLATDATA, orderLink);
  LINKDECLEX(SPLATDATA, normalLink);

  bool Update(float progress, bool &nuke);
  bool Culled() const;
};

struct LISTBASE {
  LISTDECLEX(SPLATDATA, orderLink, m_splatOrder);
  LISTDECL(CHUNKDATA, m_chunks);
  HTEXTURE__ *m_texture;
  int         m_currentCount;
  int         m_maxCount;
  int         m_flags;

  LISTBASE(int m, int f);
  ~LISTBASE();
  void         Render();
  void         Add(const NTempest::C3Vector &position, const NTempest::CAaBox &box, const NTempest::C44Matrix &matrix);
  void         SetTexture(LPCSTR n);
  CHUNKDATA   *FindChunk(int id);
  virtual bool MakeSpace() = 0;
};

struct TIMEDTEXTURE : public LISTBASE {
  TIMEDTEXTURE() : LISTBASE(128, 0) {
  }
  bool MakeSpace();
};

struct PERSISTENTTEXTURE : public LISTBASE {
  PERSISTENTTEXTURE();
  bool MakeSpace();
};

NODEDECL(CHUNKDATA) {
  int m_sourceID;
  LISTDECLEX(SPLATDATA, normalLink, m_splats);
  int                 m_flags;
  int                 m_vertCount;
  int                 m_indexCount;
  int                 m_numSplats;
  NTempest::C44Matrix m_matrix;

  CHUNKDATA() : m_sourceID(0), m_flags(0), m_vertCount(0), m_indexCount(0), m_numSplats(0) {
  }
  ~CHUNKDATA();
  SPLATDATA *Add(const CWTriData::Batch &batch, const NTempest::CAaBox &box, const NTempest::C44Matrix &basis);
  void       RecycleSplat(SPLATDATA * splat);
  void       Render();
  int        GetVertCount(const CWTriData::Batch &batch, int &lowest, int &highest);
};
