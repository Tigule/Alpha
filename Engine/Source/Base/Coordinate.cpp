#include "Coordinate.h"

void __fastcall NDCToDDC(float ndcx, float ndcy, float *ddcx, float *ddcy) {
  if (ddcx) {
    *ddcx = ndcx * 0.8f;
  }

  if (ddcy) {
    *ddcy = ndcy * 0.6f;
  }
}

void __fastcall NDCToDDC(const RECTF &ndcrect, RECTF *ddcrect) {
  ddcrect->left = ndcrect.left * 0.8f;
  ddcrect->right = ndcrect.right * 0.8f;
  ddcrect->bottom = ndcrect.bottom * 0.6f;
  ddcrect->top = ndcrect.top * 0.6f;
}

void __fastcall DDCToNDC(float ddcx, float ddcy, float *ndcx, float *ndcy) {
  if (ndcx) {
    *ndcx = ddcx * 1.25f;
  }

  if (ndcy) {
    *ndcy = ddcy * 1.6666666f;
  }
}

void __fastcall DDCToNDC(const RECTF &ddcrect, RECTF *ndcrect) {
  ndcrect->left = ddcrect.left * 1.25f;
  ndcrect->right = ddcrect.right * 1.25f;
  ndcrect->bottom = ddcrect.bottom * 1.6666666f;
  ndcrect->top = ddcrect.top * 1.6666666f;
}

float __fastcall DDCToNDCWidth(float ddcx) {
  return ddcx * 1.25f;
}

float __fastcall DDCToNDCHeight(float ddcy) {
  return ddcy * 1.6666666f;
}

float __fastcall NDCToDDCWidth(float ndcx) {
  return ndcx * 0.8f;
}

float __fastcall NDCToDDCHeight(float ndcy) {
  return ndcy * 0.6f;
}
