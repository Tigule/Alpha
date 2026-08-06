#ifndef WOW_SOURCE_UI_MINIMAPFRAME_H
#define WOW_SOURCE_UI_MINIMAPFRAME_H

#include <Frame/CSimpleFrame.h>
#include <Gx/Gx.h>

class CEvent;
class CMouseEvent;
class CStatus;
class CSimpleFontString;
class CSimpleModel;
class XMLNode;
struct DNInfo;
struct MinimapTexParams;
struct QUADDATA;

namespace NTempest {
  class C3Vector;
}

class CGMinimapFrame : public CSimpleFrame {
 public:
  static void          Initialize(int continentID);
  static void          Shutdown();
  static CSimpleFrame *Create(CSimpleFrame *parent) {
    return NEW(CGMinimapFrame)(parent);
  }

  virtual void PostLoadXML(const XMLNode *node, CStatus *status);
  virtual void OnLayerUpdate(float elapsedSec);
  virtual void OnFrameRender(CRenderBatch *batch, UINT layer);
  virtual int  OnLayerTrackUpdate(const CMouseEvent &evt);
  virtual void OnLayerCursorExit();

  static void                      RenderCallback(LPVOID param);
  void                             ForceUpdateGeometry();
  int                              OnEvent(const CEvent &event);
  static void                      SetPingPosition(const DWORDLONG &sender, const NTempest::C2Vector &pos);
  static const NTempest::C2Vector &GetPingPosition() {
    return m_pingPosition;
  }
  void Init();
  void Render();

  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

 protected:
  virtual int LookupScriptMethod(lua_State *L, LPCSTR name);

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

 private:
  CGMinimapFrame(const CGMinimapFrame &);
  CGMinimapFrame(CSimpleFrame *parent);

  enum {
    MODEL_ARROW_LOADED = 0x8
  };

  void SetPlayerArrowPosition();
  void UpdateArrowRotation(float angle);
  void UpdateGeometry(const NTempest::C2Vector &centerPoint, float radius);
  void RenderObjectBlips(const DNInfo *dnInfo);

  static int                ObjectEnumProc(DWORDLONG object, LPVOID param);
  static NTempest::C2Vector WorldPosToMinimapFrameCoords(const NTempest::C3Vector centerPoint, float radius, float x, float y, float scale);
  static void
       MinimapTextureCallback(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
  void RenderInside(float minimapSize, const NTempest::C2Vector &localOffset);
  static void RenderInsideTexture();
  static void RenderInsideSortQuads(QUADDATA *&rHead);
  static void RenderInsideQuad(QUADDATA *q);

  CSimpleFrame             *m_tooltip;
  CSimpleFontString        *m_tooltipText;
  CSimpleModel             *m_rotatingArrowFrame[3];
  CSimpleModel             *m_rotatingPartyFrame[5];
  CSimpleModel             *m_playerArrowFrame;
  float                     m_lastFacing;
  static NTempest::C2Vector m_pingPosition;
  UINT                      m_lastBlipUpdate;
  static MinimapTexParams   s_minimapTexParams;
};

#endif
