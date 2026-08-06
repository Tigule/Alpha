#pragma once

#include <storm.h>

void  NDCToDDC(float ndcx, float ndcy, float *ddcx, float *ddcy);
void  NDCToDDC(const RECTF &ndcrect, RECTF *ddcrect);
void  DDCToNDC(float ddcx, float ddcy, float *ndcx, float *ndcy);
void  DDCToNDC(const RECTF &ddcrect, RECTF *ndcrect);
float DDCToNDCWidth(float ddcx);
float DDCToNDCHeight(float ddcy);
float NDCToDDCWidth(float ndcx);
float NDCToDDCHeight(float ndcy);
