#pragma once

#include "../Object.h"

#include <Tempest/c3vector.h>

class CGBag_C;
class CGWorldFrame;
struct HMODEL__;

HMODEL__ *__fastcall ObjectModelCreate(const char *filename, OBJECT_TYPE objectType, unsigned int mdlCreateFlags);

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

 public:
  CGObject_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init);
  virtual ~CGObject_C();

  void         SetStorage(unsigned long *storage);
  void         SetTypeID(OBJECT_TYPE_ID typeID);
  void         PostInit(const CClientObjCreate &init);
  virtual void PostMovementUpdate();
  int          IsPostInited();

  static void __fastcall Initialize();
  static void __fastcall Shutdown();
  static void __fastcall UpdateAllWorldObjects();

  virtual void               Disable(int shutdown);
  virtual void               Reenable();
  virtual void               PostReenable();
  virtual CGBag_C           *GetBag();
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
  virtual float              GetScale() const;
  virtual NTempest::C3Vector GetGroundNormal() const;
  void                       SetAnimated(int animated);
  virtual HMODEL__          *GetCharacterModel(int *mounted) const;
  HMODEL__                  *GetObjectModel() const {
    return m_model;
  }
  void SetObjectModel(HMODEL__ *model);
  int  IsObjectModelLoaded();
  int  AreAttachmentsLoaded() const;
  int  ObjectModelSetSequence(HMODEL__ *model, unsigned int sequence, unsigned int flags, const char *modelName);
  int  ObjectModelSetBoneSequence(HMODEL__ *model, unsigned int sequence, unsigned int objectID, unsigned int flags);
  int  AddAttachment(HMODEL__ *parent, unsigned int parentIndex, HMODEL__ *child, float scale);
  int  ObjectIsRendering();
  int  IsDisabled();
  int  IsInReenable();

  int                            SetBlock(unsigned int i, unsigned long data);
  void                           SetData(const void *data, unsigned int bytes);
  static unsigned int __fastcall OffsetOf(OBJECT_TYPE_ID type);

  void AddWorldObject();
  void UpdateWorldObject();
  void RemoveWorldObject();

  float GetObjectHeight() const {
    return m_objectHeight;
  }

  unsigned long GetWorldObject() const {
    return m_worldObject;
  }

 protected:
  virtual const char *GetModelFileName() const;
  int                 InitModelFileName(char *modelFileName, unsigned int size);
  void                ReportMissingAnimation(unsigned int sequence, const char *modelName) const;
  void                ReportMissingAnimObj(const char *message, unsigned int objectID, const char *modelName) const;
  void                ReportMissingBone(unsigned int objectID, const char *modelName) const;
  void                ReportMissingAttachment(unsigned int objectID, const char *modelName) const;
  void                ReportNoAnimation(const char *modelName);

 protected:
  void        UpdateObjectHeight(HMODEL__ *model);

 private:
  virtual int GetSelectionHighlightColor(NTempest::CImVector *outPtr) const;

 public:
  float GetRenderScale() const {
    return m_renderScale;
  }
  void SetRenderScale(float scale) {
    m_renderScale = scale;
  }
  virtual void  RenderTargetSelection() const;
  virtual int   UpdateModelLoadStatus();
  virtual int   UpdateAttachmentLoadStatus();
  virtual int   UpdateTexComponentLoadStatus();
  virtual void  PreRender(int currentTime, float elapsed);
  virtual void  PreAnimate(CGWorldFrame *worldFrame);
  virtual void  PostAnimate(CGWorldFrame *worldFrame);
  virtual void  GetWorldMatrix(NTempest::C34Matrix *worldMatrix) const;
  void          Animate();
  void          Animate(const NTempest::C34Matrix &camRelativeMatrix);
  virtual int   ShouldRender(unsigned long worldStatus);
  virtual void  ObjectPostAnimate(float renderFacing, const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg);
  virtual void  ObjectPostAnimate(const NTempest::C34Matrix &matrix, const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg);
  virtual void  UpdateRenderFacing();
  virtual float GetRenderFacing() const;
  virtual void  OnSpecialMountAnim();
  virtual void  UpdatePlayerName() {
  }
  virtual int                 IsSolidSelectable() const;
  virtual int                 IsSolidCollidable() const;
  virtual int                 CanHighlight() const;
  virtual int                 CanBeTargetted() const;
  virtual int                 FloatingTooltip() const;
  virtual void                OnLeftClick();
  virtual void                OnRightClick();
  virtual NTempest::C34Matrix GetMatrix() const;
  void                        SetCircleRenderStates() const;

  void HideHighlightType(HIGHLIGHTTYPE type);
  void ShowHighlightType(HIGHLIGHTTYPE type);

 protected:
  virtual int ShouldFadeIn() const;
  void        ObjectSetNotRendering();

 public:
  virtual const char *GetObjectName() const;
  void                ReportMissingEventObject(unsigned int objectID, const char *modelName) const;
  virtual int         GetPageTextID(void(__fastcall *func)(int, const unsigned __int64 &, void *, bool)) const;
  void                DoFade(unsigned char alpha, unsigned int fadeTimeMs);
  unsigned int        GetAlpha() const {
    return m_alpha;
  }
  void SetMaxAlpha(unsigned int alpha) {
    m_maxAlpha = static_cast<unsigned char>(alpha);
  }

 private:
  float        m_renderScale;

 protected:
  HMODEL__    *m_model;

 private:
  unsigned int m_highlightTypes;
  float        m_objectHeight;

 protected:
  unsigned long m_worldObject;

 private:
  unsigned int m_flags;

 protected:
  unsigned int  m_fadeStartTime;
  unsigned int  m_fadeDuration;
  unsigned char m_alpha;
  unsigned char m_startAlpha;
  unsigned char m_endAlpha;
  unsigned char m_maxAlpha;
};
