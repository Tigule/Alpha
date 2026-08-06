#include <storm.h>
#include <stpl.h>
#include "W32/ISThread.h"

#define SF_ADDING    0x00000001
#define SF_OVERLAPS  0x00000002
#define SF_TEMPMASK  0x00000003
#define SF_PARAMONLY 0x00010000

#define SRGN_SOURCE_ADDING    SF_ADDING
#define SRGN_SOURCE_OVERLAPS  SF_OVERLAPS
#define SRGN_SOURCE_TEMPMASK  SF_TEMPMASK
#define SRGN_SOURCE_PARAMONLY SF_PARAMONLY
#define SRGN_SENTINEL         3.402823466e+38F
#define SRGN_MIN_POSITIVE     1.175494351e-38F
typedef struct _SOURCE {
  RECTF  rect;
  LPVOID param;
  int    sequence;
  DWORD  flags;
} SRGNSOURCE, *SRGNSOURCEPTR;

typedef struct _FOUNDPARAM {
  LPVOID param;
  int    sequence;
} SRGNPARAM, *SRGNPARAMPTR;

typedef struct RGN : public TSHashObject<RGN, HASHKEY_NONE> {
  TSGrowableArray<SRGNSOURCE> source;
  TSGrowableArray<RECTF>      combined;
  TSGrowableArray<SRGNPARAM>  foundparams;
  RECTF                       foundparamsrect;
  int                         sequence;
  int                         dirty;
} RGN, *RGNPTR;

struct HLOCKEDRGN__;
typedef HLOCKEDRGN__ *HLOCKEDRGN;

typedef TSExportTableSync<RGN, HSRGN, HLOCKEDRGN, CCritSect, 1> SRGNEXPORTTABLE;

static SRGNEXPORTTABLE s_rgntable;

static void        AddCombinedRect(TSGrowableArray<RECTF> *combinedarray, const RECTF *rect);
static void        AddSourceRect(TSGrowableArray<SRGNSOURCE> *, const RECTF *, LPVOID, int, DWORD);
static int         CheckForIntersection(const RECTF *sourcerect, const RECTF *targetrect);
static void        ClearRegion(RGN *rgnptr);
static void        CombineRectangles(TSGrowableArray<RECTF> *combinedarray);
static int         CompareRects(const RECTF *rect1, const RECTF *rect2);
static void        DeleteCombinedRect(TSGrowableArray<RECTF> *combinedarray, DWORD index);
static void        DeleteRect(RECTF *rect);
static void        DeleteSourceRect(TSGrowableArray<SRGNSOURCE> *, DWORD);
static void        FindSourceParams(RGN *rgnptr, const RECTF *rect);
static void        FragmentCombinedRectangles(TSGrowableArray<RECTF> *combinedarray, DWORD firstindex, DWORD lastindex, const RECTF *rect);
static void        FragmentSourceRectangles(TSGrowableArray<SRGNSOURCE> *, DWORD, DWORD, BOOL, const RECTF *, LPVOID, int);
static void        InvalidateRegion(RGN *rgnptr);
static int         IsNullRect(const RECTF *rect);
static void        OptimizeSource(TSGrowableArray<SRGNSOURCE> *);
static void        ProcessBooleanOperation(TSGrowableArray<SRGNSOURCE> *, int);
static void        ProduceCombinedRectangles(RGN *rgnptr);
static int __cdecl SortFoundParamsCallback(LPCVOID elem1, LPCVOID elem2);
static int __cdecl SortRectCallback(LPCVOID elem1, LPCVOID elem2);

static inline void AddCombinedRect(TSGrowableArray<RECTF> *combinedarray, const RECTF *rect) {
  RECTF *entry;

  entry = combinedarray->NewElement();
  if (entry) {
    *entry = *rect;
  }
}

static inline void AddSourceRect(TSGrowableArray<SRGNSOURCE> *sourcearray, const RECTF *rect, LPVOID param, int sequence, DWORD flags) {
  SRGNSOURCEPTR source;

  source = sourcearray->NewElement();
  if (!source) {
    return;
  }

  source->rect = *rect;
  source->param = param;
  source->sequence = sequence;
  source->flags = flags;
}

static int CheckForIntersection(const RECTF *sourcerect, const RECTF *targetrect) {
  return sourcerect->left < targetrect->right && sourcerect->bottom < targetrect->top && sourcerect->right > targetrect->left &&
         sourcerect->top > targetrect->bottom;
}

static void ClearRegion(RGN *rgnptr) {
  rgnptr->source.SetNumElements(0);
  rgnptr->combined.SetNumElements(0);
  rgnptr->foundparams.SetNumElements(0);
  DeleteRect(&rgnptr->foundparamsrect);
  rgnptr->sequence = 0;
  rgnptr->dirty = 0;
}

static void CombineRectangles(TSGrowableArray<RECTF> *combinedarray) {
  DWORD loop0;

  for (loop0 = 1; loop0 < combinedarray->NumElements(); ++loop0) {
    DWORD loop1;

    for (loop1 = 0; loop1 < loop0; ++loop1) {
      RECTF *current;
      RECTF *previous;

      current = &(*combinedarray)[loop0];
      previous = &(*combinedarray)[loop1];

      if (current->left == previous->left && current->right == previous->right &&
          (current->bottom == previous->top || previous->bottom == current->top))
      {
        current->bottom = min(current->bottom, previous->bottom);
        current->top = max(current->top, previous->top);
        DeleteRect(previous);
        break;
      }

      if (current->left == previous->right || previous->left == current->right) {
        if (current->bottom == previous->bottom && current->top == previous->top) {
          current->left = min(current->left, previous->left);
          current->right = max(current->right, previous->right);
          DeleteRect(previous);
          break;
        }

        if (current->bottom < previous->top && previous->bottom < current->top) {
          RECTF newrect[5];
          DWORD loop2;

          newrect[0].left = current->left;
          newrect[0].bottom = current->bottom;
          newrect[0].right = current->right;
          newrect[0].top = previous->bottom;

          newrect[1].left = previous->left;
          newrect[1].bottom = previous->bottom;
          newrect[1].right = previous->right;
          newrect[1].top = current->bottom;

          newrect[2].left = current->left;
          newrect[2].bottom = previous->top;
          newrect[2].right = current->right;
          newrect[2].top = current->top;

          newrect[3].left = previous->left;
          newrect[3].bottom = current->top;
          newrect[3].right = previous->right;
          newrect[3].top = previous->top;

          newrect[4].left = min(current->left, previous->left);
          newrect[4].bottom = max(current->bottom, previous->bottom);
          newrect[4].right = max(current->right, previous->right);
          newrect[4].top = min(current->top, previous->top);

          for (loop2 = 0; loop2 < 5; ++loop2) {
            if (!IsNullRect(&newrect[loop2])) {
              AddCombinedRect(combinedarray, &newrect[loop2]);
            }
          }

          DeleteCombinedRect(combinedarray, loop0);
          DeleteCombinedRect(combinedarray, loop1);
          break;
        }
      }
    }
  }
}

static int CompareRects(const RECTF *rect1, const RECTF *rect2) {
  return rect1->left == rect2->left && rect1->bottom == rect2->bottom && rect1->right == rect2->right && rect1->top == rect2->top;
}

static void DeleteCombinedRect(TSGrowableArray<RECTF> *combinedarray, DWORD index) {
  DeleteRect(&(*combinedarray)[index]);
}

static void DeleteRect(RECTF *rect) {
  rect->left = SRGN_SENTINEL;
  rect->bottom = SRGN_SENTINEL;
  rect->right = SRGN_SENTINEL;
  rect->top = SRGN_SENTINEL;
}

static void DeleteSourceRect(TSGrowableArray<SRGNSOURCE> *sourcearray, DWORD index) {
  DeleteRect(&(*sourcearray)[index].rect);
  (*sourcearray)[index].param = NULL;
  (*sourcearray)[index].sequence = -1;
  (*sourcearray)[index].flags = 0;
}

static void FindSourceParams(RGN *rgnptr, const RECTF *rect) {
  DWORD loop;
  DWORD sourcerects;
  DWORD params;

  if (!CompareRects(rect, &rgnptr->foundparamsrect)) {
    rgnptr->foundparams.SetNumElements(0);
    sourcerects = rgnptr->source.NumElements();
    params = 0;
    for (loop = 0; loop < sourcerects; ++loop) {
      DWORD        found;
      int          sequence;
      SRGNPARAMPTR foundparam;

      if (!CheckForIntersection(rect, &rgnptr->source[loop].rect)) {
        continue;
      }

      sequence = rgnptr->source[loop].sequence;
      found = 0;
      while (found < params) {
        if (rgnptr->foundparams[found].sequence == sequence) {
          goto nextsource;
        }
        ++found;
      }

      foundparam = rgnptr->foundparams.NewElement();
      foundparam->param = rgnptr->source[loop].param;
      foundparam->sequence = sequence;
      ++params;

    nextsource:;
    }

    qsort(rgnptr->foundparams.Ptr(), rgnptr->foundparams.NumElements(), sizeof(SRGNPARAM), SortFoundParamsCallback);
    rgnptr->foundparamsrect = *rect;
  }
}

static void FragmentCombinedRectangles(TSGrowableArray<RECTF> *combinedarray, DWORD firstindex, DWORD lastindex, const RECTF *rect) {
  RECTF newrect[4];

  for (; firstindex < lastindex; ++firstindex) {
    RECTF *existing = &(*combinedarray)[firstindex];

    if (!CheckForIntersection(rect, existing)) {
      continue;
    }

    newrect[0].left = rect->left;
    newrect[0].bottom = rect->bottom;
    newrect[0].right = rect->right;
    newrect[0].top = existing->bottom;

    newrect[1].left = rect->left;
    newrect[1].bottom = existing->top;
    newrect[1].right = rect->right;
    newrect[1].top = rect->top;

    newrect[2].left = rect->left;
    newrect[2].bottom = max(rect->bottom, existing->bottom);
    newrect[2].right = existing->left;
    newrect[2].top = min(rect->top, existing->top);

    newrect[3].left = existing->right;
    newrect[3].bottom = max(rect->bottom, existing->bottom);
    newrect[3].right = rect->right;
    newrect[3].top = min(rect->top, existing->top);

    for (DWORD loop = 0; loop < 4; ++loop) {
      if (!IsNullRect(&newrect[loop])) {
        FragmentCombinedRectangles(combinedarray, firstindex + 1, lastindex, &newrect[loop]);
      }
    }
    return;
  }

  AddCombinedRect(combinedarray, rect);
}

static void FragmentSourceRectangles(
    TSGrowableArray<SRGNSOURCE> *sourcearray,
    DWORD                        firstindex,
    DWORD                        lastindex,
    BOOL                         previousoverlap,
    const RECTF                 *rect,
    LPVOID                       param,
    int                          sequence
) {
  BOOL         overlaps[5][2];
  RECTF        newrect[5];
  int          minleft;
  int          maxleft;
  BOOL         overlapsexisting;
  const RECTF *overlaprect[2];
  int          minright;
  int          minbottom;
  DWORD        loop;
  DWORD        index;

  overlapsexisting = previousoverlap;
  for (index = firstindex; index < lastindex; ++index) {
    if (CheckForIntersection(rect, &(*sourcearray)[index].rect)) {
      if (CompareRects(rect, &(*sourcearray)[index].rect)) {
        (*sourcearray)[index].flags |= SRGN_SOURCE_OVERLAPS;
        overlapsexisting = TRUE;
        continue;
      }

      overlaprect[0] = rect;
      overlaprect[1] = &(*sourcearray)[index].rect;
      minleft = overlaprect[0]->left > overlaprect[1]->left;
      maxleft = overlaprect[1]->left > overlaprect[0]->left;
      minright = overlaprect[0]->right > overlaprect[1]->right;
      minbottom = overlaprect[0]->bottom > overlaprect[1]->bottom;

      newrect[0].left = overlaprect[minbottom]->left;
      newrect[0].bottom = overlaprect[minbottom]->bottom;
      newrect[0].right = overlaprect[minbottom]->right;
      newrect[0].top = overlaprect[overlaprect[1]->bottom > overlaprect[0]->bottom]->bottom;

      newrect[1].left = overlaprect[overlaprect[1]->top > overlaprect[0]->top]->left;
      newrect[1].bottom = overlaprect[overlaprect[0]->top > overlaprect[1]->top]->top;
      newrect[1].right = overlaprect[overlaprect[1]->top > overlaprect[0]->top]->right;
      newrect[1].top = overlaprect[overlaprect[1]->top > overlaprect[0]->top]->top;

      newrect[2].left = overlaprect[minleft]->left;
      newrect[2].bottom = overlaprect[overlaprect[1]->bottom > overlaprect[0]->bottom]->bottom;
      newrect[2].right = overlaprect[maxleft]->left;
      newrect[2].top = overlaprect[overlaprect[0]->top > overlaprect[1]->top]->top;

      newrect[3].left = overlaprect[minright]->right;
      newrect[3].bottom = overlaprect[overlaprect[1]->bottom > overlaprect[0]->bottom]->bottom;
      newrect[3].right = overlaprect[overlaprect[1]->right > overlaprect[0]->right]->right;
      newrect[3].top = overlaprect[overlaprect[0]->top > overlaprect[1]->top]->top;

      newrect[4].left = overlaprect[maxleft]->left;
      newrect[4].bottom = overlaprect[overlaprect[1]->bottom > overlaprect[0]->bottom]->bottom;
      newrect[4].right = overlaprect[minright]->right;
      newrect[4].top = overlaprect[overlaprect[0]->top > overlaprect[1]->top]->top;

      for (loop = 0; loop < 5; ++loop) {
        if (IsNullRect(&newrect[loop])) {
          overlaps[loop][0] = FALSE;
          overlaps[loop][1] = FALSE;
        } else {
          overlaps[loop][0] = CheckForIntersection(&newrect[loop], overlaprect[0]);
          overlaps[loop][1] = CheckForIntersection(&newrect[loop], overlaprect[1]);
        }
      }

      for (loop = 0; loop < 5; ++loop) {
        if (overlaps[loop][0]) {
          FragmentSourceRectangles(sourcearray, index + 1, lastindex, overlapsexisting || overlaps[loop][1], &newrect[loop], param, sequence);
        }
        if (overlaps[loop][1]) {
          AddSourceRect(
              sourcearray, &newrect[loop], (*sourcearray)[index].param, (*sourcearray)[index].sequence,
              ((*sourcearray)[index].flags & ~SRGN_SOURCE_TEMPMASK) | (overlaps[loop][0] ? SRGN_SOURCE_OVERLAPS : 0)
          );
        }
      }

      DeleteSourceRect(sourcearray, index);
      return;
    }
  }

  AddSourceRect(sourcearray, rect, param, sequence, SRGN_SOURCE_ADDING | (overlapsexisting ? SRGN_SOURCE_OVERLAPS : 0));
}

static void InvalidateRegion(RGN *rgnptr) {
  rgnptr->dirty = 1;
  DeleteRect(&rgnptr->foundparamsrect);
}

static int IsNullRect(const RECTF *rect) {
  return !(rect->left < rect->right && rect->bottom < rect->top);
}

static void OptimizeSource(TSGrowableArray<SRGNSOURCE> *sourcearray) {
  DWORD index;

  index = 0;
  while (index < sourcearray->NumElements()) {
    if (IsNullRect(&(*sourcearray)[index].rect)) {
      (*sourcearray)[index] = (*sourcearray)[sourcearray->NumElements() - 1];
      sourcearray->SetNumElements(sourcearray->NumElements() - 1);
    } else {
      ++index;
    }
  }
}

static void ProcessBooleanOperation(TSGrowableArray<SRGNSOURCE> *sourcearray, int combinemode) {
  DWORD index;
  DWORD flags;
  BOOL  remove;

  index = 0;
  while (index < sourcearray->NumElements()) {
    flags = (*sourcearray)[index].flags;
    remove = FALSE;

    switch (combinemode) {
      case 1:
        remove = !(flags & 2);
        break;
      case 2:
        remove = FALSE;
        break;
      case 3:
        remove = (flags & 2) != 0;
        break;
      case 4:
        remove = (flags & 3) != 0;
        break;
      case 5:
        remove = (flags & 1) != 0;
        break;
    }

    if (remove) {
      DeleteSourceRect(sourcearray, index);
    }
    (*sourcearray)[index].flags = 0;
    ++index;
  }
}

static void ProduceCombinedRectangles(RGN *rgnptr) {
  DWORD         count;
  SRGNSOURCEPTR source;

  count = rgnptr->source.NumElements();
  rgnptr->combined.SetNumElements(0);
  source = rgnptr->source.Ptr();
  while (count) {
    if (!(source->flags & SRGN_SOURCE_PARAMONLY)) {
      FragmentCombinedRectangles(&rgnptr->combined, 0, rgnptr->combined.NumElements(), &source->rect);
    }
    ++source;
    --count;
  }

  CombineRectangles(&rgnptr->combined);

  qsort(rgnptr->combined.Ptr(), rgnptr->combined.NumElements(), sizeof(RECTF), SortRectCallback);

  count = rgnptr->combined.NumElements();
  while (count) {
    DWORD last = count - 1;
    if (!IsNullRect(&rgnptr->combined.Ptr()[last])) {
      break;
    }
    rgnptr->combined.SetNumElements(last);
    count = rgnptr->combined.NumElements();
  }
}

static int __cdecl SortFoundParamsCallback(LPCVOID elem1, LPCVOID elem2) {
  const SRGNPARAM *param1 = (const SRGNPARAM *)elem1;
  const SRGNPARAM *param2 = (const SRGNPARAM *)elem2;

  return param1->sequence - param2->sequence;
}

static int __cdecl SortRectCallback(LPCVOID elem1, LPCVOID elem2) {
  const RECTF *rect1 = (const RECTF *)elem1;
  const RECTF *rect2 = (const RECTF *)elem2;
  float        delta;

  if (rect1->top == rect2->top) {
    delta = rect1->left - rect2->left;
  } else {
    delta = rect1->top - rect2->top;
  }

  if (delta > 0.0F) {
    return 1;
  }
  if (delta < 0.0F) {
    return -1;
  }
  return 0;
}

extern "C" void APIENTRY SRgnClear(HSRGN handle) {
  HLOCKEDRGN lockedhandle;
  RGN       *rgnptr;

  FATALASSERT(handle);

  rgnptr = s_rgntable.Lock(handle, &lockedhandle, 0);
  if (rgnptr) {
    ClearRegion(rgnptr);
    s_rgntable.Unlock(lockedhandle);
  }
}

extern "C" void APIENTRY SRgnCombineRectf(HSRGN handle, const RECTF *rect, LPVOID param, int combinemode) {
  HLOCKEDRGN lockedhandle;
  RGN       *rgnptr;

  FATALASSERT(handle);
  FATALASSERT(rect);
  FATALASSERT(combinemode >= 1);
  FATALASSERT(combinemode <= 6);

  rgnptr = s_rgntable.Lock(handle, &lockedhandle, 0);
  if (rgnptr) {
    if (combinemode == 2 || combinemode == 6) {
      if (!IsNullRect(rect)) {
        AddSourceRect(&rgnptr->source, const_cast<RECTF *>(rect), param, ++rgnptr->sequence, combinemode == 6 ? SRGN_SOURCE_PARAMONLY : 0);
      }
    } else {
      if (!IsNullRect(rect)) {
        FragmentSourceRectangles(&rgnptr->source, 0, rgnptr->source.NumElements(), FALSE, const_cast<RECTF *>(rect), param, ++rgnptr->sequence);
      }
      ProcessBooleanOperation(&rgnptr->source, combinemode);
      OptimizeSource(&rgnptr->source);
    }

    InvalidateRegion(rgnptr);
    s_rgntable.Unlock(lockedhandle);
  }
}

extern "C" void APIENTRY SRgnCombineRecti(HSRGN handle, const RECT *rect, LPVOID param, int combinemode) {
  RECTF rectf;

  FATALASSERT(rect);

  rectf.left = (float)rect->left;
  rectf.bottom = (float)rect->top;
  rectf.right = (float)rect->right;
  rectf.top = (float)rect->bottom;
  SRgnCombineRectf(handle, &rectf, param, combinemode);
}

extern "C" void APIENTRY SRgnCreate(HSRGN *handle, DWORD reserved) {
  HLOCKEDRGN lockedhandle;
  RGN       *rgnptr;

  FATALASSERT(handle);

  *handle = NULL;
  FATALASSERT(!reserved);

  rgnptr = s_rgntable.NewLock(handle, &lockedhandle);
  ClearRegion(rgnptr);
  s_rgntable.Unlock(lockedhandle);
}

extern "C" void APIENTRY SRgnDelete(HSRGN handle) {
  FATALASSERT(handle);

  s_rgntable.Delete(handle);
}

extern "C" void APIENTRY SRgnDestroy() {
  s_rgntable.Destroy();
}

extern "C" void APIENTRY SRgnDuplicate(HSRGN orighandle, HSRGN *handle, DWORD reserved) {
  RGN *original;
  RGN *copy;

  FATALASSERT(handle);

  *handle = NULL;
  FATALASSERT(orighandle);
  FATALASSERT(!reserved);

  original = s_rgntable.Lock(orighandle, reinterpret_cast<HLOCKEDRGN *>(&orighandle), 0);
  if (!original) {
    return;
  }

  copy = s_rgntable.NewLock(handle, reinterpret_cast<HLOCKEDRGN *>(&handle));
  *copy = *original;
  s_rgntable.Unlock(reinterpret_cast<HLOCKEDRGN>(handle));
  s_rgntable.Unlock(reinterpret_cast<HLOCKEDRGN>(orighandle));
}

extern "C" void APIENTRY SRgnGetBoundingRectf(HSRGN handle, RECTF *rect) {
  HLOCKEDRGN    lockedhandle;
  RGN          *rgnptr;
  SRGNSOURCEPTR source;
  DWORD         count;

  FATALASSERT(handle);
  FATALASSERT(rect);

  rect->left = SRGN_SENTINEL;
  rect->bottom = SRGN_SENTINEL;
  rect->right = SRGN_MIN_POSITIVE;
  rect->top = SRGN_MIN_POSITIVE;

  rgnptr = s_rgntable.Lock(handle, &lockedhandle, 0);
  if (rgnptr) {
    source = rgnptr->source.Ptr();
    count = rgnptr->source.NumElements();
    while (count) {
      if (!(source->flags & SRGN_SOURCE_PARAMONLY)) {
        rect->left = min(rect->left, source->rect.left);
        rect->bottom = min(rect->bottom, source->rect.bottom);
        rect->right = max(rect->right, source->rect.right);
        rect->top = max(rect->top, source->rect.top);
      }
      ++source;
      --count;
    }
    s_rgntable.Unlock(lockedhandle);
  }

  if (IsNullRect(rect)) {
    rect->left = 0.0F;
    rect->bottom = 0.0F;
    rect->right = 0.0F;
    rect->top = 0.0F;
  }
}

extern "C" void APIENTRY SRgnGetBoundingRecti(HSRGN handle, RECT *rect) {
  RECTF rectf;

  FATALASSERT(rect);

  SRgnGetBoundingRectf(handle, &rectf);
  rect->left = (LONG)rectf.left;
  rect->top = (LONG)rectf.bottom;
  rect->right = (LONG)rectf.right;
  rect->bottom = (LONG)rectf.top;
}

extern "C" void APIENTRY SRgnGetRectParamsf(HSRGN handle, const RECTF *rect, DWORD *numparams, LPVOID *buffer) {
  HLOCKEDRGN lockedhandle;
  RGN       *rgnptr;
  DWORD      count;
  DWORD      loop;

  FATALASSERT(handle);
  FATALASSERT(rect);
  FATALASSERT(numparams);

  if (IsNullRect(rect)) {
    *numparams = 0;
    return;
  }

  rgnptr = s_rgntable.Lock(handle, &lockedhandle, 0);
  if (!rgnptr) {
    *numparams = 0;
    return;
  }

  if (rgnptr->dirty) {
    ProduceCombinedRectangles(rgnptr);
    rgnptr->dirty = 0;
  }

  FindSourceParams(rgnptr, rect);
  if (buffer) {
    count = *numparams;
    if (count >= rgnptr->foundparams.NumElements()) {
      count = rgnptr->foundparams.NumElements();
    }
    *numparams = count;
    SRGNPARAMPTR params = rgnptr->foundparams.Ptr();
    for (loop = 0; loop < count; ++loop) {
      buffer[loop] = params[loop].param;
    }
  } else {
    *numparams = rgnptr->foundparams.NumElements();
  }
  s_rgntable.Unlock(lockedhandle);
}

extern "C" void APIENTRY SRgnGetRectParamsi(HSRGN handle, const RECT *rect, DWORD *numparams, LPVOID *buffer) {
  RECTF rectf;

  FATALASSERT(rect);

  rectf.left = (float)rect->left;
  rectf.bottom = (float)rect->top;
  rectf.right = (float)rect->right;
  rectf.top = (float)rect->bottom;
  SRgnGetRectParamsf(handle, &rectf, numparams, buffer);
}

extern "C" void APIENTRY SRgnGetRectsf(HSRGN handle, DWORD *numrects, RECTF *buffer) {
  HLOCKEDRGN lockedhandle;
  RGN       *rgnptr;
  DWORD      count;

  FATALASSERT(handle);
  FATALASSERT(numrects);

  rgnptr = s_rgntable.Lock(handle, &lockedhandle, 0);
  if (!rgnptr) {
    *numrects = 0;
    return;
  }
  if (rgnptr->dirty) {
    ProduceCombinedRectangles(rgnptr);
    rgnptr->dirty = 0;
  }

  if (buffer) {
    count = *numrects;
    if (count >= rgnptr->combined.NumElements()) {
      count = rgnptr->combined.NumElements();
    }
    *numrects = count;
    memcpy(buffer, rgnptr->combined.Ptr(), count * sizeof(RECTF));
  } else {
    *numrects = rgnptr->combined.NumElements();
  }
  s_rgntable.Unlock(lockedhandle);
}

extern "C" void APIENTRY SRgnGetRectsi(HSRGN handle, DWORD *numrects, RECT *buffer) {
  DWORD loop;

  SRgnGetRectsf(handle, numrects, (RECTF *)buffer);
  if (buffer && numrects) {
    for (loop = 0; loop < *numrects; ++loop) {
      RECTF *rectf = (RECTF *)&buffer[loop];
      buffer[loop].left = (LONG)rectf->left;
      buffer[loop].top = (LONG)rectf->bottom;
      buffer[loop].right = (LONG)rectf->right;
      buffer[loop].bottom = (LONG)rectf->top;
    }
  }
}

extern "C" BOOL APIENTRY SRgnIsPointInRegionf(HSRGN handle, float x, float y) {
  HLOCKEDRGN    lockedhandle;
  RGN          *rgnptr;
  SRGNSOURCEPTR sourcearray;
  DWORD         count;
  DWORD         loop;
  BOOL          result;

  FATALASSERT(handle);

  result = FALSE;
  rgnptr = s_rgntable.Lock(handle, &lockedhandle, 0);
  if (rgnptr) {
    sourcearray = rgnptr->source.Ptr();
    count = rgnptr->source.NumElements();
    for (loop = 0; loop < count; ++loop) {
      SRGNSOURCEPTR source = &sourcearray[loop];

      if (!(source->flags & SRGN_SOURCE_PARAMONLY) && x >= source->rect.left && y >= source->rect.bottom && x < source->rect.right &&
          y < source->rect.top)
      {
        result = TRUE;
        break;
      }
    }
    s_rgntable.Unlock(lockedhandle);
  }
  return result;
}

extern "C" BOOL APIENTRY SRgnIsPointInRegioni(HSRGN handle, int x, int y) {
  return SRgnIsPointInRegionf(handle, (float)x, (float)y);
}

extern "C" BOOL APIENTRY SRgnIsRectInRegionf(HSRGN handle, const RECTF *rect) {
  HLOCKEDRGN    lockedhandle;
  RGN          *rgnptr;
  SRGNSOURCEPTR source;
  DWORD         count;
  DWORD         loop;
  BOOL          result;

  FATALASSERT(handle);
  FATALASSERT(rect);

  rgnptr = s_rgntable.Lock(handle, &lockedhandle, 0);
  if (rgnptr) {
    result = FALSE;
    source = rgnptr->source.Ptr();
    count = rgnptr->source.NumElements();
    for (loop = 0; loop < count; ++loop) {
      if (!(source[loop].flags & SRGN_SOURCE_PARAMONLY) && CheckForIntersection(rect, &source[loop].rect)) {
        result = TRUE;
        break;
      }
    }
    s_rgntable.Unlock(lockedhandle);
    return result;
  }
  return FALSE;
}

extern "C" BOOL APIENTRY SRgnIsRectInRegioni(HSRGN handle, const RECT *rect) {
  RECTF rectf;

  FATALASSERT(rect);

  rectf.left = (float)rect->left;
  rectf.bottom = (float)rect->top;
  rectf.right = (float)rect->right;
  rectf.top = (float)rect->bottom;
  return SRgnIsRectInRegionf(handle, &rectf);
}

extern "C" void APIENTRY SRgnOffsetf(HSRGN handle, float xoffset, float yoffset) {
  HLOCKEDRGN    lockedhandle;
  RGN          *rgnptr;
  SRGNSOURCEPTR source;
  DWORD         count;

  FATALASSERT(handle);

  rgnptr = s_rgntable.Lock(handle, &lockedhandle, 0);
  if (rgnptr) {
    source = rgnptr->source.Ptr();
    count = rgnptr->source.NumElements();
    while (count) {
      source->rect.left += xoffset;
      source->rect.bottom += yoffset;
      source->rect.right += xoffset;
      source->rect.top += yoffset;
      source++;
      count--;
    }
    InvalidateRegion(rgnptr);
    s_rgntable.Unlock(lockedhandle);
  }
}

extern "C" void APIENTRY SRgnOffseti(HSRGN handle, int xoffset, int yoffset) {
  SRgnOffsetf(handle, (float)xoffset, (float)yoffset);
}
