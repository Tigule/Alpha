#pragma once

#include "../Object.h"

#include <Tempest/c3vector.h>
#include <Tempest/c34matrix.h>
#include <Tempest/cimvector.h>

class CGBag_C;
class CGWorldFrame;
struct HMODEL__;

HMODEL__ *ObjectModelCreate(LPCSTR filename, OBJECT_TYPE objectType, UINT mdlCreateFlags);

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
  int IsPostInited() const;

  static void Initialize();
  static void Shutdown();
  static void UpdateAllWorldObjects();

  virtual void     Disable(int shutdown);
  virtual void     Reenable();
  virtual void     PostReenable();
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
  void              SetAnimated(int animated);
  virtual HMODEL__ *GetCharacterModel(int *mounted) const;
  HMODEL__         *GetObjectModel() const {
    return m_model;
  }
  void SetObjectModel(HMODEL__ *model);
  int  IsObjectModelLoaded() const;
  int  AreAttachmentsLoaded() const;
  int  AddAttachment(HMODEL__ *parent, UINT parentIndex, HMODEL__ *child, float scale);
  int  IsDisabled() const;
  int  IsInReenable() const;

  int         SetBlock(UINT i, DWORD data);
  void        SetData(LPCVOID data, UINT bytes);
  static UINT OffsetOf(OBJECT_TYPE_ID type);

  void AddWorldObject();
  void UpdateWorldObject();
  void RemoveWorldObject();

  float GetObjectHeight() const {
    return m_objectHeight;
  }

  DWORD GetWorldObject() const {
    return m_worldObject;
  }

 protected:
  virtual LPCSTR GetModelFileName() const = 0;
  int            InitModelFileName(char *modelFileName, UINT size);
  void           ReportMissingAnimation(UINT sequence, LPCSTR modelName) const;
  void           ReportMissingBone(UINT objectID, LPCSTR modelName) const;
  void           ReportMissingAttachment(UINT objectID, LPCSTR modelName) const;
  void           ReportNoAnimation(LPCSTR modelName);
  int            ObjectModelSetSequence(HMODEL__ *model, UINT sequence, UINT flags, LPCSTR modelName);
  int            ObjectModelSetBoneSequence(HMODEL__ *model, UINT sequence, UINT objectID, UINT flags);
  int            ObjectIsRendering() const;

 public:
  void UpdateObjectHeight(HMODEL__ *model);

 private:
  void        ReportMissingAnimObj(LPCSTR message, UINT objectID, LPCSTR modelName) const;
  virtual int GetSelectionHighlightColor(NTempest::CImVector *outPtr) const {
    FATALASSERT(outPtr);
    outPtr->Set(0xFFFFFFFF);
    return 1;
  }

 public:
  float GetRenderScale() const {
    return m_renderScale;
  }
  void SetRenderScale(float scale) {
    m_renderScale = scale;
  }
  virtual void RenderTargetSelection() const {
  }
  virtual int UpdateModelLoadStatus();
  virtual int UpdateAttachmentLoadStatus();
  virtual int UpdateTexComponentLoadStatus() {
    return 0;
  }
  virtual void PreRender(int currentTime, float elapsed) {
  }
  virtual void PreAnimate(CGWorldFrame *worldFrame);
  virtual void PostAnimate(CGWorldFrame *worldFrame) {
  }
  virtual void GetWorldMatrix(NTempest::C34Matrix *worldMatrix) const;
  void         Animate();
  void         Animate(const NTempest::C34Matrix &camRelativeMatrix);
  virtual int  ShouldRender(DWORD worldStatus);
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
  virtual int IsSolidSelectable() const {
    return 1;
  }
  virtual int IsSolidCollidable() const {
    return 1;
  }
  virtual int CanHighlight() const {
    return 0;
  }
  virtual int CanBeTargetted() const {
    return 0;
  }
  virtual int FloatingTooltip() const {
    return 0;
  }
  virtual void OnLeftClick() {
  }
  virtual void                OnRightClick();
  virtual NTempest::C34Matrix GetMatrix() const {
    return NTempest::C34Matrix();
  }
  void SetCircleRenderStates() const;

  void HideHighlightType(HIGHLIGHTTYPE type);
  void ShowHighlightType(HIGHLIGHTTYPE type);

 protected:
  virtual int ShouldFadeIn() const {
    return 1;
  }
  void ObjectSetNotRendering();

 public:
  virtual LPCSTR GetObjectName() const;
  void           ReportMissingEventObject(UINT objectID, LPCSTR modelName) const;
  virtual int    GetPageTextID(void (*)(int, const DWORDLONG &, LPVOID, bool)) const {
    return 0;
  }
  void DoFade(BYTE alpha, UINT fadeTimeMs);
  BYTE GetAlpha() const {
    return m_alpha;
  }
  void SetMaxAlpha(BYTE alpha) {
    m_maxAlpha = alpha;
  }

 private:
  CGObject_C &operator=(const CGObject_C &object);

  float     m_renderScale;
  HMODEL__ *m_model;

  UINT  m_highlightTypes;
  float m_objectHeight;

 protected:
  DWORD m_worldObject;

 private:
  UINT m_flags;

 protected:
  UINT m_fadeStartTime;
  UINT m_fadeDuration;
  BYTE m_alpha;
  BYTE m_startAlpha;
  BYTE m_endAlpha;
  BYTE m_maxAlpha;
};
