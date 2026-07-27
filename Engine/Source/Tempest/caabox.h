#ifndef ENGINE_SOURCE_TEMPEST_CAABOX_H
#define ENGINE_SOURCE_TEMPEST_CAABOX_H

#include "Tempest/c3vector.h"

namespace NTempest {

  template <class T>
  class CDynTable;

  class CAaBox {
   public:
    CAaBox(float value = 0.0f) : b(value), t(value) {
    }
    CAaBox(const C3Vector &value) : b(value), t(value) {
    }
    CAaBox(const C3Vector &bottom, const C3Vector &top) : b(bottom), t(top) {
    }
    ~CAaBox() {
    }

    const float *Access() const;
    float       *Access();
    void         Get(C3Vector &bottom, C3Vector &top) const;
    void         Set(const C3Vector &bottom, const C3Vector &top);
    void         Set(const C3Vector &value);
    void         Set(float value);

    CAaBox &operator+=(float value);
    CAaBox &operator+=(const CAaBox &value);
    CAaBox &operator-=(float value);
    CAaBox &operator-=(const CAaBox &value);
    CAaBox &operator*=(float value);
    CAaBox &operator*=(const CAaBox &value);
    CAaBox &operator/=(float value);
    CAaBox &operator/=(const CAaBox &value);
    CAaBox  operator-() const;

    unsigned char NotEmpty() const;
    unsigned char Empty() const;
    unsigned char Encloses(const CAaBox &value) const;
    unsigned char Encloses(const C2Vector &value) const;
    unsigned char Encloses(const C3Vector &value) const;
    unsigned char Contains(const CAaBox &value) const;
    unsigned char Contains(const C2Vector &value) const;
    unsigned char Contains(const C3Vector &value) const;
    unsigned char InOpenR(const CAaBox &value) const;
    unsigned char InOpenR(const C3Vector &value) const;
    unsigned char Intersects(const CAaBox &value) const;
    unsigned char Intersects2d(const CAaBox &value) const;

    float    Width() const;
    float    Height() const;
    float    Depth() const;
    C3Vector Diagonal() const;
    C3Vector Center() const;
    void     CenterX(const CAaBox &value);
    float    CenterX() const;
    void     CenterY(const CAaBox &value);
    float    CenterY() const;
    void     CenterZ(const CAaBox &value);
    float    CenterZ() const;

    void Stretch(float value);
    void Stretch(const C3Vector &value);
    void StretchX(float value);
    void StretchY(float value);
    void StretchZ(float value);
    void Offset(const C3Vector &value);
    void OffsetX(float value);
    void OffsetY(float value);
    void OffsetZ(float value);
    void SetWidth(float value);
    void SetHeight(float value);
    void SetDepth(float value);
    void SetWidthTop(float value);
    void SetHeightTop(float value);
    void SetDepthTop(float value);
    void Enclose(const C3Vector &value) {
      b = C3Vector::Min(b, value);
      t = C3Vector::Max(t, value);
    }
    void SetWidthCenter(float value);
    void SetHeightCenter(float value);
    void SetDepthCenter(float value);
    void CenterAt(const CAaBox &value);
    void CenterAt(const C3Vector &value);
    void AlignBottom(const CAaBox &value);
    void AlignTop(const CAaBox &value);
    void AlignBottomX(const CAaBox &value);
    void AlignTopX(const CAaBox &value);
    void AlignBottomY(const CAaBox &value);
    void AlignTopY(const CAaBox &value);
    void AlignBottomZ(const CAaBox &value);
    void AlignTopZ(const CAaBox &value);

    static CAaBox Lerp(const CAaBox &a, const CAaBox &b, const CAaBox &t);
    static CAaBox Intersection(const CAaBox &a, const CAaBox &b, const CAaBox &c);
    static CAaBox Intersection(const CAaBox &a, const CAaBox &b);
    static CAaBox Union(const CAaBox &a, const CAaBox &b);
    CAaBox                  Intersect(const CAaBox &value);
    CAaBox                  Unite(const CAaBox &value);

    static CAaBox Bounding(const CDynTable<unsigned long> &indices, const CDynTable<C3Vector> &vectors);
    static CAaBox Bounding(const CDynTable<C3Vector> &vectors);
    static CAaBox Bounding(const C3Vector *vectors, unsigned long count);

    C3Vector b;
    C3Vector t;
  };

}  // namespace NTempest

#endif
