#ifndef ENGINE_SOURCE_SRP_SHA_H
#define ENGINE_SOURCE_SRP_SHA_H

struct SHA1_CONTEXT {
  unsigned int  state[5];
  unsigned int  count[2];
  unsigned char buffer[64];
};

void __fastcall SHA1_Init(SHA1_CONTEXT *context);
void __fastcall SHA1_Update(SHA1_CONTEXT *context, const unsigned char *data, unsigned int len);
void __fastcall SHA1_Final(unsigned char *const digest, SHA1_CONTEXT *context);
unsigned char *__fastcall SHA1_InterleaveHash(unsigned char *digest, const unsigned char *data, unsigned int len);

#endif
