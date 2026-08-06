#ifndef ENGINE_SOURCE_SRP_SHA_H
#define ENGINE_SOURCE_SRP_SHA_H

#include <storm.h>

typedef struct SHA1_CONTEXT {
  UINT state[5];
  UINT count[2];
  BYTE buffer[64];
} SHA1_CONTEXT;

void  SHA1_Init(SHA1_CONTEXT *context);
void  SHA1_Update(SHA1_CONTEXT *context, const BYTE *data, UINT len);
void  SHA1_Final(BYTE *digest, SHA1_CONTEXT *context);
BYTE *SHA1_InterleaveHash(BYTE *digest, const BYTE *data, UINT len);

#endif
