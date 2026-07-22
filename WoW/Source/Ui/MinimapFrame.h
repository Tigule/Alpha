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
  virtual ~CGMinimapFrame();

  static void __fastcall          Initialize(int continentID);
  static void __fastcall          Shutdown();
  static CSimpleFrame *__fastcall Create(CSimpleFrame *parent) {
    return NEW(CGMinimapFrame)(parent);
  }

  virtual void PostLoadXML(const XMLNode *node, CStatus *status);
  virtual void OnLayerUpdate(float elapsedSec);
  virtual void OnFrameRender(CRenderBatch *batch, unsigned int layer);
  virtual int  OnLayerTrackUpdate(const CMouseEvent &evt);
  virtual void OnLayerCursorExit();

  static void __fastcall           RenderCallback(void *param);
  void                             ForceUpdateGeometry();
  int                              OnEvent(const CEvent &event);
  static void __fastcall           SetPingPosition(const unsigned __int64 &sender, const NTempest::C2Vector &pos);
  static const NTempest::C2Vector &GetPingPosition() {
    return m_pingPosition;
  }
  void Init();
  void Render();

  static void __fastcall RegisterScriptMethods();
  static void __fastcall UnregisterScriptMethods();

 protected:
  virtual int LookupScriptMethod(lua_State *L, const char *name);

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

 private:
  CGMinimapFrame(CGMinimapFrame &);
  CGMinimapFrame(CSimpleFrame *parent);

  enum {
    MODEL_ARROW_LOADED = 0x8
  };

  void SetPlayerArrowPosition();
  void UpdateArrowRotation(float angle);
  void UpdateGeometry(NTempest::C2Vector &centerPoint, float radius);
  void RenderObjectBlips(DNInfo *dnInfo);

  static int __fastcall                ObjectEnumProc(unsigned __int64 object, void *param);
  static NTempest::C2Vector __fastcall WorldPosToMinimapFrameCoords(NTempest::C3Vector centerPoint, float radius, float x, float y, float scale);
  static void __fastcall               MinimapTextureCallback(
      EGxTexCommand cmd,
      unsigned int  w,
      unsigned int  h,
      unsigned int  d,
      unsigned int  mipLevel,
      void         *userArg,
      unsigned int &texelStrideInBytes,
      const void  *&texels
  );
  void                   RenderInside(float minimapSize, NTempest::C2Vector &localOffset);
  static void __fastcall RenderInsideTexture();
  static void __fastcall RenderInsideSortQuads(QUADDATA *&rHead);
  static void __fastcall RenderInsideQuad(QUADDATA *q);

  CSimpleFrame             *m_tooltip;
  CSimpleFontString        *m_tooltipText;
  CSimpleModel             *m_rotatingArrowFrame[3];
  CSimpleModel             *m_rotatingPartyFrame[5];
  CSimpleModel             *m_playerArrowFrame;
  float                     m_lastFacing;
  static NTempest::C2Vector m_pingPosition;
  unsigned int              m_lastBlipUpdate;
  static MinimapTexParams   s_minimapTexParams;
};

#endif
