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

 public:
  NODEDECL(FRAMENODE) {
    virtual ~FRAMENODE() {
    }

    CLayoutFrame *frame;
    UINT          dep;
  };

  typedef FRAMENODE       *PFRAMENODE;
  typedef const FRAMENODE *PCFRAMENODE;

  CLayoutFrame();

 protected:
  virtual void OnFrameSizeChanged(const NTempest::CRect &rect);
  virtual int  OnFrameResize();

 public:
  virtual ~CLayoutFrame();
  virtual void          LoadXML(const XMLNode *node, CStatus *status);
  virtual CLayoutFrame *GetLayoutParent() {
    return 0;
  }
  virtual void          SetDeferredResize(int enable);
  virtual int           SetRect(NTempest::CRect &rect);
  virtual int           GetRect(NTempest::CRect *rect) const;
  virtual void          SetLayoutScale(float scale, bool force);
  virtual float         GetWidth();
  virtual float         GetHeight();
  virtual int           IsAttachmentOrigin();
  virtual CLayoutFrame *GetLayoutFrameByName(LPCSTR name);

  float       Left();
  float       Top();
  float       Right();
  float       Bottom();
  float       CenterY();
  float       CenterX();
  int         CalculateRect(NTempest::CRect *rect);
  void        SetPoint(FRAMEPOINT point, float x, float y, int doResize);
  void        SetPoint(FRAMEPOINT point, CLayoutFrame *relative, FRAMEPOINT relativePoint, float offsetX, float offsetY, int doResize);
  void        SetAllPoints(CLayoutFrame *relative, int doResize);
  void        Clear(CLayoutFrame *relative, int doResize);
  void        ClearAllPoints(int doResize);
  void        RegisterResize(CLayoutFrame *frame, UINT dependency);
  void        UnregisterResize(const CLayoutFrame *frame);
  int         IsResizeDependency(CLayoutFrame *pNewDependentFrame);
  int         FlattenFrame(CLayoutFrame *top, float width, float height, float delta_x, float delta_y, NTempest::CRect *finalrect);
  int         ScaleBy(CLayoutFrame *top, float scale_x, float scale_y, FRAMEPOINT anchorpoint, NTempest::CRect *finalrect);
  int         DragBy(CLayoutFrame *top, float delta_x, float delta_y, FRAMEPOINT dragpoint, NTempest::CRect *finalrect);
  int         IsResizePending();
  int         PtInFrameRect(const NTempest::C2Vector &pt);
  void        Resize(int force);
  static UINT ResizePending();
  static void ClearResizePendingList();
  void        SetWidth(float width);
  void        SetHeight(float height);
  void        CageMouseInFrame(int enable);

  CFramePoint *GetPoint(FRAMEPOINT whichPoint) {
    FATALASSERT(whichPoint < FRAMEPOINT_NUMPOINTS);

    return m_points[whichPoint];
  }

  float GetLayoutScale() const {
    return m_layoutScale;
  }

  int IsResizeDeferred() const {
    return (m_flags & 0x2) != 0;
  }

  int IsRectValid() const {
    return (m_flags & 0x1) != 0;
  }

  int HasPoints() {
    return m_points.Count() != 0;
  }

 protected:
  void        DestroyLayout();
  static void RemoveFromResizeList(CLayoutFrame *pFrame);

 private:
  float GetFirstPointX(const FRAMEPOINT *pointarray, int elements);
  float GetFirstPointY(const FRAMEPOINT *pointarray, int elements);
  void  FreePoints();

  TSFixedArray<CFramePoint *> m_points;
  struct {
    UINT left : 1;
    UINT top : 1;
    UINT right : 1;
    UINT bottom : 1;
    UINT centerX : 1;
    UINT centerY : 1;
  } m_guard;
  LISTDECL(FRAMENODE, m_resizeList);
  BYTE m_resizeCounter;

 protected:
  UINT            m_flags;
  NTempest::CRect m_rect;
  float           m_width;
  float           m_height;
  float           m_layoutScale;

 public:
  LINKDECLEX(CLayoutFrame, resizeLink);
};

#endif
