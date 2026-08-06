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
  NTempest::C3Vector pos;
  NTempest::C2Vector texCoord;
};

class CRibbonEmitter {
  friend class RibbonManager;

 public:
  CRibbonEmitter();
  CRibbonEmitter(const CRibbonEmitter &rhs);
  ~CRibbonEmitter();
  const CRibbonEmitter &operator=(const CRibbonEmitter &rhs);
  CRibbonEmitter       *Clone() const {
    return NEW(CRibbonEmitter)(*this);
  }

  CRibbonEmitter *AddRef();
  void            DecRef();
  void            Initialize(
      float                              edgesPerSec,
      float                              edgeLifeSpanInSec,
      const NTempest::CImVector         &diffuseClr,
      const TSGrowableArray<HTEXTURE>   &textures,
      const TSGrowableArray<CRibbonMat> &materials,
      const TSGrowableArray<UINT>       &replaces,
      const NTempest::CRect             &texBox,
      UINT                               rows,
      UINT                               cols
  );
  void SetEnabled(int enable_);
  void SetTexSlot(UINT slot);
  void SetAbove(float above);
  void SetBelow(float below);
  void SetGravity(float gravity);
  void SetPos(const NTempest::C44Matrix &orient, const NTempest::C3Vector &cameraPosition);
  void SetMats(const TSGrowableArray<CRibbonMat> &materials, const TSGrowableArray<HTEXTURE> &textures, const TSGrowableArray<UINT> &replaces);
  void SetColor(const float r, const float g, const float b);
  void SetAlpha(const float a);
  void SingletonMgrUpdate(float elapsedTime, const NTempest::C3Vector &cameraWorldPos, int suppressNewEdges);
  int  Render();
  int  IsDead();
  void Update(float elapsedSec, int suppressNewEdges);
  UINT ReplaceTexture(UINT replaceableId, HTEXTURE texture);
  void MaterialDisableLight(int disable);
  void MaterialDisableFog(int disable);

 protected:
  void ConvertTexSlotToTexCoords();
  void InitInterpDeltas();
  void InterpEdge(float age, float t, UINT advance);
  void Advance(UINT &pos, UINT amount);
  void CloseTextureHandles();
  void BuildMaterialStack();

 private:
  void PrivCopy(const CRibbonEmitter &rhs);

  UINT                           m_refCount;
  TSGrowableArray<float>         m_edges;
  UINT                           m_writePos;
  UINT                           m_readPos;
  float                          m_startTime;
  NTempest::C3Vector             m_prevPos;
  NTempest::C3Vector             m_cameraPos;
  TSGrowableArray<CRibbonVertex> m_gxVertices;
  TSGrowableArray<WORD>          m_gxIndices;
  float                          m_ooLifeSpan;
  float                          m_tmpDU;
  float                          m_tmpDV;
  float                          m_ooTmpDU;
  float                          m_ooTmpDV;
  NTempest::CRect                m_texSlotBox;
  NTempest::C3Vector             m_prevVertical;
  NTempest::C3Vector             m_currVertical;
  NTempest::C3Vector             m_prevDir;
  NTempest::C3Vector             m_currDir;
  NTempest::C3Vector             m_prevDirScaled;
  NTempest::C3Vector             m_currDirScaled;
  NTempest::C3Vector             m_below0;
  NTempest::C3Vector             m_below1;
  NTempest::C3Vector             m_above0;
  NTempest::C3Vector             m_above1;

 protected:
  float                       m_edgesPerSec;
  float                       m_edgeLifeSpan;
  TSGrowableArray<CRibbonMat> m_materials;
  TSGrowableArray<HTEXTURE>   m_textures;
  TSGrowableArray<UINT>       m_replaces;
  NTempest::CImVector         m_diffuseClr;
  NTempest::CRect             m_texBox;
  UINT                        m_rows;
  UINT                        m_cols;
  UINT                        m_posSet : 1;
  UINT                        m_initialized : 1;
  UINT                        m_enabled : 1;
  UINT                        m_updated : 1;
  UINT                        m_singletonUpdated : 1;
  NTempest::C3Vector          m_currPos;
  UINT                        m_texSlot;
  float                       m_above;
  float                       m_below;
  float                       m_gravity;
};
