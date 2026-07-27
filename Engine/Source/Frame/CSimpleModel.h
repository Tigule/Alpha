#ifndef ENGINE_SOURCE_FRAME_CSIMPLEMODEL_H
#define ENGINE_SOURCE_FRAME_CSIMPLEMODEL_H

#include "Frame/CSimpleFrame.h"
#include "Gx/CGxDevice.h"
#include "Model/IModel.h"
#include "Tempest/caabox.h"
#include "Tempest/c3vector.h"
#include "Tempest/cimvector.h"

class CSimpleModel : public CSimpleFrame {
  friend class CGMinimapFrame;

 public:
  CSimpleModel(CSimpleFrame *parent);
  virtual ~CSimpleModel();

  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

  virtual void  LoadXML(const XMLNode *node, CStatus *status);
  virtual void  LoadXML_Scripts(const XMLNode *node, CStatus *status);
  virtual float GetWidth();
  virtual float GetHeight();
  virtual void  OnFrameRender(CRenderBatch *batch, unsigned int layer);
  virtual void  UpdateModel();

  void SetModel(HMODEL model);
  void SetModel(const char *sourcefile, CModelCreate *data, CStatus *status);
  void SetCamera(HCAMERA camera);
  void SetCameraByIndex(unsigned int index);
  void SetLight(const CGxLight &light);
  void ReplaceTexture(unsigned int materialID, const char *textureName);

  virtual void SetAlpha(unsigned char alpha);

  void SetPosition(const NTempest::C3Vector &position) {
    m_position = position;
  }

  void SetFacing(float facing) {
    m_facing = facing;
  }

  void SetScale(float scale) {
    m_scale = scale;
  }

  void SetSequence(unsigned int index);
  int  SetSequenceTime(unsigned int index, int timeOffset);
  int  AdvanceTime();

  NTempest::C3Vector GetPosition() {
    return m_position;
  }

  float GetFacing() {
    return m_facing;
  }

  float GetScale() {
    return m_scale;
  }

  int ModelJustLoaded() const;

  void SetFog(int fog) {
    if (fog) {
      m_flags |= 0x2;
    } else {
      m_flags &= ~0x2;
    }
  }

  void SetFogColor(const NTempest::CImVector &color) {
    m_fogColor = color;
  }

  void SetFogNear(float fogNear) {
    m_fogNear = fogNear;
  }

  void SetFogFar(float fogFar) {
    m_fogFar = fogFar;
  }

  void SetOnUpdateModelScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnUpdateModel", GetName());
    SetEventScript(m_onUpdateModel, source, description);
  }

  void SetOnAnimFinishedScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnAnimFinished", GetName());
    SetEventScript(m_onAnimFinished, source, description);
  }

  void RunOnAnimFinishedScript() {
    if (m_onAnimFinished) {
      FrameScript_Execute(m_onAnimFinished, this);
    }
  }

  HMODEL GetModel() {
    return m_model;
  }

  HCAMERA GetCamera() {
    return (m_flags & 0x4) ? 0 : m_camera;
  }

  int HasFog() {
    return (m_flags & 0x2) != 0;
  }

  int IsModelLoaded();
  int IsUserFlagSet(unsigned int flag);
  void SetUserFlag(unsigned int flag, int set);
  void SetModelLoaded(int loaded);
  int IsWaitingForCamera();
  void SetWaitingForCamera(int waiting);

  const NTempest::CImVector &GetFogColor() {
    return m_fogColor;
  }

  float GetFogNear() {
    return m_fogNear;
  }

  float GetFogFar() {
    return m_fogFar;
  }

  void RunOnUpdateModelScript() {
    if (m_onUpdateModel) {
      FrameScript_Execute(m_onUpdateModel, this);
    }
  }

 protected:
  virtual int LookupScriptMethod(lua_State *L, const char *name);

  void FinishLoadingModel();

 private:
  void SetCameraInternal(HCAMERA camera);

 public:
  static void RenderModel(void *param);

 protected:
  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

  HMODEL m_model;
  union {
    HCAMERA      m_camera;
    unsigned int m_cameraIndex;
  };
  CGxLight            m_light;
  NTempest::C3Vector  m_position;
  float               m_facing;
  float               m_scale;
  unsigned int        m_flags;
  NTempest::CImVector m_fogColor;
  float               m_fogNear;
  float               m_fogFar;
  NTempest::CAaBox    m_cachedExtents;
  int                 m_onUpdateModel;
  int                 m_onAnimFinished;
};

#endif
