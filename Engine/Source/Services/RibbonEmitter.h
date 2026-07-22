#pragma once

#include "Texture.h"

#include <stpl.h>

struct CRibbonMat {
  CRibbonMat() : enableLighting(1), enableFog(1), enableDepthTest(1), enableDepthWrite(1), enableCulling(1), alpha(GxBlend_Opaque) {
  }

  int      enableLighting : 1;
  int      enableFog : 1;
  int      enableDepthTest : 1;
  int      enableDepthWrite : 1;
  int      enableCulling : 1;
  EGxBlend alpha;
};

struct CRibbonVertex {
  ~CRibbonVertex();

  NTempest::C3Vector pos;
  NTempest::C2Vector texCoord;
};

class CRibbonEmitter {
  friend class RibbonManager;

 public:
  CRibbonEmitter();
  CRibbonEmitter(const CRibbonEmitter &rhs);
  ~CRibbonEmitter();

  CRibbonEmitter *AddRef();
  void            DecRef();
  void            Initialize(
      float                                edgesPerSec,
      float                                edgeLifeSpanInSec,
      const NTempest::CImVector           &diffuseClr,
      const TSGrowableArray<HTEXTURE>     &textures,
      const TSGrowableArray<CRibbonMat>   &materials,
      const TSGrowableArray<unsigned int> &replaces,
      const NTempest::CRect               &texBox,
      unsigned int                         rows,
      unsigned int                         cols
  );
  void         SetEnabled(int enable_);
  void         SetTexSlot(unsigned int slot);
  void         SetAbove(float above);
  void         SetBelow(float below);
  void         SetGravity(float gravity);
  void         SingletonMgrUpdate(float elapsedTime, const NTempest::C3Vector &cameraWorldPos, int suppressNewEdges);
  int          Render();
  int          IsDead();
  void         Update(float elapsedSec, int suppressNewEdges);
  unsigned int ReplaceTexture(unsigned int replaceableId, HTEXTURE texture);

 protected:
  void ConvertTexSlotToTexCoords();
  void InitInterpDeltas();
  void InterpEdge(float age, float t, unsigned int advance);
  void Advance(unsigned int &pos, unsigned int amount);
  void CloseTextureHandles();

 private:
  void PrivCopy(const CRibbonEmitter &rhs);

  unsigned int                    m_refCount;
  TSGrowableArray<float>          m_edges;
  unsigned int                    m_writePos;
  unsigned int                    m_readPos;
  float                           m_startTime;
  NTempest::C3Vector              m_prevPos;
  NTempest::C3Vector              m_cameraPos;
  TSGrowableArray<CRibbonVertex>  m_gxVertices;
  TSGrowableArray<unsigned short> m_gxIndices;
  float                           m_ooLifeSpan;
  float                           m_tmpDU;
  float                           m_tmpDV;
  float                           m_ooTmpDU;
  float                           m_ooTmpDV;
  NTempest::CRect                 m_texSlotBox;
  NTempest::C3Vector              m_prevVertical;
  NTempest::C3Vector              m_currVertical;
  NTempest::C3Vector              m_prevDir;
  NTempest::C3Vector              m_currDir;
  NTempest::C3Vector              m_prevDirScaled;
  NTempest::C3Vector              m_currDirScaled;
  NTempest::C3Vector              m_below0;
  NTempest::C3Vector              m_below1;
  NTempest::C3Vector              m_above0;
  NTempest::C3Vector              m_above1;

 protected:
  float                         m_edgesPerSec;
  float                         m_edgeLifeSpan;
  TSGrowableArray<CRibbonMat>   m_materials;
  TSGrowableArray<HTEXTURE>     m_textures;
  TSGrowableArray<unsigned int> m_replaces;
  NTempest::CImVector           m_diffuseClr;
  NTempest::CRect               m_texBox;
  unsigned int                  m_rows;
  unsigned int                  m_cols;
  unsigned int                  m_posSet : 1;
  unsigned int                  m_initialized : 1;
  unsigned int                  m_enabled : 1;
  unsigned int                  m_updated : 1;
  unsigned int                  m_singletonUpdated : 1;
  NTempest::C3Vector            m_currPos;
  unsigned int                  m_texSlot;
  float                         m_above;
  float                         m_below;
  float                         m_gravity;
};
