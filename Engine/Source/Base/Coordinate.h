#pragma once

#include <storm.h>

void __fastcall  NDCToDDC(float ndcx, float ndcy, float *ddcx, float *ddcy);
void __fastcall  NDCToDDC(const RECTF &ndcrect, RECTF *ddcrect);
void __fastcall  DDCToNDC(float ddcx, float ddcy, float *ndcx, float *ndcy);
void __fastcall  DDCToNDC(const RECTF &ddcrect, RECTF *ndcrect);
float __fastcall DDCToNDCWidth(float ddcx);
float __fastcall DDCToNDCHeight(float ddcy);
float __fastcall NDCToDDCWidth(float ndcx);
float __fastcall NDCToDDCHeight(float ndcy);
