#ifndef ENGINE_SOURCE_FRAME_CLAYOUTFRAME_H
#define ENGINE_SOURCE_FRAME_CLAYOUTFRAME_H

#include "Tempest/c2vector.h"
#include "Tempest/crect.h"

#include <stpl.h>

class CFramePoint;
class CStatus;
class XMLNode;

enum FRAMEPOINT {
  FRAMEPOINT_TOPLEFT = 0,
  FRAMEPOINT_TOP = 1,
  FRAMEPOINT_TOPRIGHT = 2,
  FRAMEPOINT_LEFT = 3,
  FRAMEPOINT_CENTER = 4,
  FRAMEPOINT_RIGHT = 5,
  FRAMEPOINT_BOTTOMLEFT = 6,
  FRAMEPOINT_BOTTOM = 7,
  FRAMEPOINT_BOTTOMRIGHT = 8,
  FRAMEPOINT_NUMPOINTS = 9
};

class CLayoutFrame {
  friend class CSimpleFontString;
  friend class CSimpleTexture;

 private:
  TSFixedArray<CFramePoint *> m_points;
  struct {
    UINT left : 1;
    UINT top : 1;
    UINT right : 1;
    UINT bottom : 1;
    UINT centerX : 1;
    UINT centerY : 1;
  } m_guard;

  NODEDECL(FRAMENODE) {
    CLayoutFrame *frame;
    UINT          dep;

    virtual ~FRAMENODE() {
    }
  };

  typedef FRAMENODE       *PFRAMENODE;
  typedef const FRAMENODE *PCFRAMENODE;

  LISTDECL(FRAMENODE, m_resizeList);
  BYTE m_resizeCounter;

  float GetFirstPointX(const FRAMEPOINT pointarray[], int elements);
  float GetFirstPointY(const FRAMEPOINT pointarray[], int elements);
  void  FreePoints();

 protected:
  UINT            m_flags;
  NTempest::CRect m_rect;
  float           m_width;
  float           m_height;
  float           m_layoutScale;

  void         DestroyLayout();
  virtual void OnFrameSizeChanged(const NTempest::CRect &rect);
  virtual BOOL OnFrameResize();
  static void  RemoveFromResizeList(CLayoutFrame *pFrame);

 public:
  LINKDECLEX(CLayoutFrame, resizeLink);

  CLayoutFrame();
  virtual ~CLayoutFrame();
  virtual void          LoadXML(const XMLNode *node, CStatus *status);
  virtual CLayoutFrame *GetLayoutParent() {
    return 0;
  }

  float Left();
  float Top();
  float Right();
  float Bottom();
  float CenterY();
  float CenterX();
  BOOL  CalculateRect(NTempest::CRect *rect);

  BOOL IsRectValid() const {
    return (m_flags & 0x1) != 0;
  }

  void SetPoint(FRAMEPOINT point, float x, float y, int doResize);
  void SetPoint(FRAMEPOINT point, CLayoutFrame *relative, FRAMEPOINT relativePoint, float offsetX, float offsetY, int doResize);
  void SetAllPoints(CLayoutFrame *relative, int doResize);
  void Clear(CLayoutFrame *relative, int doResize);
  void ClearAllPoints(int doResize);

  CFramePoint *GetPoint(FRAMEPOINT whichPoint) {
    VALIDATEBEGIN;
    VALIDATE(whichPoint < FRAMEPOINT_NUMPOINTS);
    VALIDATEEND;

    return m_points[whichPoint];
  }

  BOOL HasPoints() {
    UINT count = m_points.Count();

    while (count) {
      if (m_points[--count]) {
        return 1;
      }
    }

    return 0;
  }

  void RegisterResize(CLayoutFrame *frame, UINT dependency);
  void UnregisterResize(const CLayoutFrame *frame);
  BOOL IsResizeDependency(CLayoutFrame *pNewDependentFrame);

  BOOL IsResizeDeferred() const {
    return m_flags & 0x2;
  }

  virtual void SetDeferredResize(int enable);
  void         Resize(int force);
  BOOL         IsResizePending();
  virtual int  SetRect(NTempest::CRect &rect);
  virtual BOOL GetRect(NTempest::CRect *rect) const;
  virtual void SetLayoutScale(float scale, bool force);

  float GetLayoutScale() const {
    return m_layoutScale;
  }

  void                  SetWidth(float width);
  void                  SetHeight(float height);
  virtual float         GetWidth();
  virtual float         GetHeight();
  BOOL                  FlattenFrame(CLayoutFrame *top, float width, float height, float delta_x, float delta_y, NTempest::CRect *finalrect);
  int                   ScaleBy(CLayoutFrame *top, float scale_x, float scale_y, FRAMEPOINT anchorpoint, NTempest::CRect *finalrect);
  int                   DragBy(CLayoutFrame *top, float delta_x, float delta_y, FRAMEPOINT dragpoint, NTempest::CRect *finalrect);
  BOOL                  PtInFrameRect(const NTempest::C2Vector &pt);
  void                  CageMouseInFrame(int enable);
  virtual BOOL          IsAttachmentOrigin() {
    return 0;
  }
  static UINT           ResizePending();
  static void           ClearResizePendingList();
  virtual CLayoutFrame *GetLayoutFrameByName(LPCSTR name);
};

#endif
