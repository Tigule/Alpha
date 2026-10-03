#pragma once

#include "../Object.h"

#include <Tempest/c3vector.h>
#include <Tempest/c34matrix.h>
#include <Tempest/cimvector.h>

class CGBag_C;
class CGWorldFrame;
struct HMODEL__;
typedef HMODEL__ *HMODEL;

HMODEL ObjectModelCreate(LPCSTR filename, OBJECT_TYPE objectType, UINT mdlCreateFlags);

enum HIGHLIGHTTYPE {
  HT_OBJSELECTION = 0,
  HT_MOUSEOVER = 1,
  NUM_HIGHLIGHTTYPES = 2
};

namespace NTempest {

  class C34Matrix;
  class CImVector;

}  // namespace NTempest

class CGObject_C : public CGObject {
  friend class CGCamera;
  friend class CGPlayer_C;
  friend class CGUnit_C;

 public:
  CGObject_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init);
  ~CGObject_C();

  void SetStorage(DWORD *storage);
  void SetTypeID(OBJECT_TYPE_ID typeID);
  void PostInit(const CClientObjCreate &init);
  void PostMovementUpdate() {
  }

  virtual void Disable(int shutdown);
  virtual void Reenable();
  virtual void PostReenable();
  BOOL         IsPostInited() const;
  BOOL         IsInReenable() const;

  static void Initialize();
  static void Shutdown();

  void  AddWorldObject();
  void  UpdateWorldObject();
  void  RemoveWorldObject();
  DWORD GetWorldObject() const {
    return m_worldObject;
  }
  static void UpdateAllWorldObjects();

  BOOL        SetBlock(UINT i, DWORD data);
  void        SetData(LPCVOID data, UINT bytes);
  static UINT OffsetOf(OBJECT_TYPE_ID type);

  float GetObjectHeight() const {
    return m_objectHeight;
  }

  virtual CGBag_C *GetBag() {
    return 0;
  }
  virtual NTempest::C3Vector GetPosition() const {
    return NTempest::C3Vector();
  }
  virtual void GetPosition(NTempest::C3Vector &vec) const {
    vec.x = 0.0f;
    vec.y = 0.0f;
    vec.z = 0.0f;
  }
  virtual float GetFacing() const {
    return 0.0f;
  }
  virtual float GetScale() const {
    return m_obj->m_scale;
  }
  virtual NTempest::C3Vector GetGroundNormal() const {
    return NTempest::C3Vector(0.0f, 0.0f, 1.0f);
  }
  void           SetAnimated(int animated);
  virtual HMODEL GetCharacterModel(int *mounted) const;
  HMODEL         GetObjectModel() const {
    return m_model;
  }
  void SetObjectModel(HMODEL model);
  int  AddAttachment(HMODEL parent, UINT parentIndex, HMODEL child, float scale);

 protected:
  BOOL           ObjectModelSetSequence(HMODEL model, UINT sequence, UINT flags, LPCSTR modelName);
  BOOL           ObjectModelSetBoneSequence(HMODEL model, UINT sequence, UINT objectID, UINT flags);
  virtual LPCSTR GetModelFileName() const = 0;
  BOOL           InitModelFileName(char *modelFileName, UINT size);
  void           ReportMissingAnimation(UINT sequence, LPCSTR modelName) const;
  void           ReportMissingBone(UINT objectID, LPCSTR modelName) const;
  void           ReportMissingAttachment(UINT objectID, LPCSTR modelName) const;

 public:
  void ReportMissingEventObject(UINT objectID, LPCSTR modelName) const;

 protected:
  void ReportNoAnimation(LPCSTR modelName);
  int  ObjectIsRendering() const;
  void ObjectSetNotRendering();

 public:
  BOOL IsDisabled() const;
  BOOL IsObjectModelLoaded() const;
  int  AreAttachmentsLoaded() const;

 private:
  void         ReportMissingAnimObj(LPCSTR message, UINT objectID, LPCSTR modelName) const;
  CGObject_C  &operator=(const CGObject_C &object);
  virtual BOOL GetSelectionHighlightColor(NTempest::CImVector *outPtr) const {
    FATALASSERT(outPtr);
    outPtr->Set(0xFFFFFFFF);
    return 1;
  }

 public:
  void         HideHighlightType(HIGHLIGHTTYPE type);
  void         ShowHighlightType(HIGHLIGHTTYPE type);
  virtual void RenderTargetSelection() const {
  }
  float GetRenderScale() const {
    return m_renderScale;
  }
  void SetRenderScale(float scale) {
    m_renderScale = scale;
  }

 private:
  float m_renderScale;

 public:
  virtual BOOL UpdateModelLoadStatus();
  virtual BOOL UpdateAttachmentLoadStatus();
  virtual BOOL UpdateTexComponentLoadStatus() {
    return 0;
  }
  virtual void PreRender(int currentTime, float elapsed) {
  }
  virtual void PreAnimate(CGWorldFrame *worldFrame);
  virtual void PostAnimate(CGWorldFrame *worldFrame) {
  }
  virtual void GetWorldMatrix(NTempest::C34Matrix *worldMatrix) const;
  void         Animate(const NTempest::C34Matrix &camRelativeMatrix);
  void         Animate();
  virtual BOOL ShouldRender(DWORD worldStatus);
  virtual void ObjectPostAnimate(float renderFacing, const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg) {
  }
  virtual void ObjectPostAnimate(const NTempest::C34Matrix &matrix, const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg) {
  }
  virtual void UpdateRenderFacing() {
  }
  virtual float GetRenderFacing() const {
    return GetFacing();
  }
  virtual void OnSpecialMountAnim() {
  }
  virtual void UpdatePlayerName() {
  }
  void         UpdateObjectHeight(HMODEL model);
  virtual BOOL IsSolidSelectable() const {
    return 1;
  }
  virtual BOOL IsSolidCollidable() const {
    return 1;
  }
  virtual BOOL CanHighlight() const {
    return 0;
  }
  virtual BOOL CanBeTargetted() const {
    return 0;
  }
  virtual BOOL FloatingTooltip() const {
    return 0;
  }
  virtual void OnLeftClick() {
  }
  virtual void                OnRightClick();
  virtual NTempest::C34Matrix GetMatrix() const {
    return NTempest::C34Matrix();
  }

 protected:
  virtual BOOL ShouldFadeIn() const {
    return 1;
  }

 public:
  void SetCircleRenderStates() const;

 private:
  HMODEL m_model;
  UINT   m_highlightTypes;
  float  m_objectHeight;

 protected:
  DWORD m_worldObject;

 private:
  UINT m_flags;

 public:
  virtual LPCSTR GetObjectName() const;
  virtual int    GetPageTextID(void (*)(int, const DWORDLONG &, LPVOID, bool)) const {
    return 0;
  }
  BYTE GetAlpha() const {
    return m_alpha;
  }
  void SetMaxAlpha(BYTE alpha) {
    m_maxAlpha = alpha;
  }
  void DoFade(BYTE alpha, UINT fadeTimeMs);

 protected:
  UINT m_fadeStartTime;
  UINT m_fadeDuration;
  BYTE m_alpha;
  BYTE m_startAlpha;
  BYTE m_endAlpha;
  BYTE m_maxAlpha;
};
