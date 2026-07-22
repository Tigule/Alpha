#include <malloc.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char BYTE;
typedef unsigned int  DWORD;

typedef struct SHA1_CONTEXT {
  unsigned int  state[5];
  unsigned int  count[2];
  unsigned char buffer[64];
} SHA1_CONTEXT;

static const BYTE s_sha1Padding = 0x80;
static BYTE       s_sha1Zero;

#define SHA1_ROL(value, bits) _lrotl((value), (bits))
#define SHA1_BLK0(i)          (words[i] = (SHA1_ROL(((DWORD *)buffer)[i], 24) & 0xFF00FF00) | (SHA1_ROL(((DWORD *)buffer)[i], 8) & 0x00FF00FF))
#define SHA1_BLK(i) (words[(i) & 15] = SHA1_ROL(words[((i) + 13) & 15] ^ words[((i) + 8) & 15] ^ words[((i) + 2) & 15] ^ words[(i) & 15], 1))
#define SHA1_R0(v, w, x, y, z, i)                                        \
  z += ((w & (x ^ y)) ^ y) + SHA1_BLK0(i) + 0x5A827999 + SHA1_ROL(v, 5); \
  w = SHA1_ROL(w, 30)
#define SHA1_R1(v, w, x, y, z, i)                                       \
  z += ((w & (x ^ y)) ^ y) + SHA1_BLK(i) + 0x5A827999 + SHA1_ROL(v, 5); \
  w = SHA1_ROL(w, 30)
#define SHA1_R2(v, w, x, y, z, i)                               \
  z += (w ^ x ^ y) + SHA1_BLK(i) + 0x6ED9EBA1 + SHA1_ROL(v, 5); \
  w = SHA1_ROL(w, 30)
#define SHA1_R3(v, w, x, y, z, i)                                             \
  z += (((w | x) & y) | (w & x)) + SHA1_BLK(i) + 0x8F1BBCDC + SHA1_ROL(v, 5); \
  w = SHA1_ROL(w, 30)
#define SHA1_R4(v, w, x, y, z, i)                               \
  z += (w ^ x ^ y) + SHA1_BLK(i) + 0xCA62C1D6 + SHA1_ROL(v, 5); \
  w = SHA1_ROL(w, 30)

void __fastcall SHA1_Transform(unsigned int *const state, const unsigned char *const buffer) {
  DWORD words[16];
  DWORD a;
  DWORD b;
  DWORD c;
  DWORD d;
  DWORD e;

  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];

  SHA1_R0(a, b, c, d, e, 0);
  SHA1_R0(e, a, b, c, d, 1);
  SHA1_R0(d, e, a, b, c, 2);
  SHA1_R0(c, d, e, a, b, 3);
  SHA1_R0(b, c, d, e, a, 4);
  SHA1_R0(a, b, c, d, e, 5);
  SHA1_R0(e, a, b, c, d, 6);
  SHA1_R0(d, e, a, b, c, 7);
  SHA1_R0(c, d, e, a, b, 8);
  SHA1_R0(b, c, d, e, a, 9);
  SHA1_R0(a, b, c, d, e, 10);
  SHA1_R0(e, a, b, c, d, 11);
  SHA1_R0(d, e, a, b, c, 12);
  SHA1_R0(c, d, e, a, b, 13);
  SHA1_R0(b, c, d, e, a, 14);
  SHA1_R0(a, b, c, d, e, 15);
  SHA1_R1(e, a, b, c, d, 16);
  SHA1_R1(d, e, a, b, c, 17);
  SHA1_R1(c, d, e, a, b, 18);
  SHA1_R1(b, c, d, e, a, 19);
  SHA1_R2(a, b, c, d, e, 20);
  SHA1_R2(e, a, b, c, d, 21);
  SHA1_R2(d, e, a, b, c, 22);
  SHA1_R2(c, d, e, a, b, 23);
  SHA1_R2(b, c, d, e, a, 24);
  SHA1_R2(a, b, c, d, e, 25);
  SHA1_R2(e, a, b, c, d, 26);
  SHA1_R2(d, e, a, b, c, 27);
  SHA1_R2(c, d, e, a, b, 28);
  SHA1_R2(b, c, d, e, a, 29);
  SHA1_R2(a, b, c, d, e, 30);
  SHA1_R2(e, a, b, c, d, 31);
  SHA1_R2(d, e, a, b, c, 32);
  SHA1_R2(c, d, e, a, b, 33);
  SHA1_R2(b, c, d, e, a, 34);
  SHA1_R2(a, b, c, d, e, 35);
  SHA1_R2(e, a, b, c, d, 36);
  SHA1_R2(d, e, a, b, c, 37);
  SHA1_R2(c, d, e, a, b, 38);
  SHA1_R2(b, c, d, e, a, 39);
  SHA1_R3(a, b, c, d, e, 40);
  SHA1_R3(e, a, b, c, d, 41);
  SHA1_R3(d, e, a, b, c, 42);
  SHA1_R3(c, d, e, a, b, 43);
  SHA1_R3(b, c, d, e, a, 44);
  SHA1_R3(a, b, c, d, e, 45);
  SHA1_R3(e, a, b, c, d, 46);
  SHA1_R3(d, e, a, b, c, 47);
  SHA1_R3(c, d, e, a, b, 48);
  SHA1_R3(b, c, d, e, a, 49);
  SHA1_R3(a, b, c, d, e, 50);
  SHA1_R3(e, a, b, c, d, 51);
  SHA1_R3(d, e, a, b, c, 52);
  SHA1_R3(c, d, e, a, b, 53);
  SHA1_R3(b, c, d, e, a, 54);
  SHA1_R3(a, b, c, d, e, 55);
  SHA1_R3(e, a, b, c, d, 56);
  SHA1_R3(d, e, a, b, c, 57);
  SHA1_R3(c, d, e, a, b, 58);
  SHA1_R3(b, c, d, e, a, 59);
  SHA1_R4(a, b, c, d, e, 60);
  SHA1_R4(e, a, b, c, d, 61);
  SHA1_R4(d, e, a, b, c, 62);
  SHA1_R4(c, d, e, a, b, 63);
  SHA1_R4(b, c, d, e, a, 64);
  SHA1_R4(a, b, c, d, e, 65);
  SHA1_R4(e, a, b, c, d, 66);
  SHA1_R4(d, e, a, b, c, 67);
  SHA1_R4(c, d, e, a, b, 68);
  SHA1_R4(b, c, d, e, a, 69);
  SHA1_R4(a, b, c, d, e, 70);
  SHA1_R4(e, a, b, c, d, 71);
  SHA1_R4(d, e, a, b, c, 72);
  SHA1_R4(c, d, e, a, b, 73);
  SHA1_R4(b, c, d, e, a, 74);
  SHA1_R4(a, b, c, d, e, 75);
  SHA1_R4(e, a, b, c, d, 76);
  SHA1_R4(d, e, a, b, c, 77);
  SHA1_R4(c, d, e, a, b, 78);
  SHA1_R4(b, c, d, e, a, 79);

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
}

void __fastcall SHA1_Init(SHA1_CONTEXT *context) {
  context->state[0] = 0x67452301;
  context->state[1] = 0xEFCDAB89;
  context->state[2] = 0x98BADCFE;
  context->state[3] = 0x10325476;
  context->state[4] = 0xC3D2E1F0;
  context->count[1] = 0;
  context->count[0] = 0;
}

void __fastcall SHA1_Update(SHA1_CONTEXT *context, const unsigned char *data, unsigned int len) {
  DWORD i;
  DWORD j;

  j = (context->count[0] >> 3) & 63;
  context->count[0] += len << 3;
  if (context->count[0] < (len << 3)) {
    context->count[1]++;
  }
  context->count[1] += len >> 29;

  if (j + len > 63) {
    i = 64 - j;
    memcpy(&context->buffer[j], data, i);
    SHA1_Transform(context->state, context->buffer);
    for (; i + 63 < len; i += 64) {
      SHA1_Transform(context->state, data + i);
    }
    j = 0;
  } else {
    i = 0;
  }

  memcpy(&context->buffer[j], data + i, len - i);
}

void __fastcall SHA1_Final(unsigned char *const digest, SHA1_CONTEXT *context) {
  BYTE  finalcount[8];
  DWORD i;

  for (i = 0; i < 8; i++) {
    finalcount[i] = (BYTE)((context->count[(i >= 4) ? 0 : 1] >> ((3 - (i & 3)) * 8)) & 0xFF);
  }

  SHA1_Update(context, &s_sha1Padding, 1);
  while ((context->count[0] & 0x1F8) != 0x1C0) {
    SHA1_Update(context, &s_sha1Zero, 1);
  }
  SHA1_Update(context, finalcount, 8);

  for (i = 0; i < 20; i++) {
    digest[i] = (BYTE)((context->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 0xFF);
  }
}

unsigned char *__fastcall SHA1_InterleaveHash(unsigned char *const digest, const unsigned char *data, unsigned int len) {
  SHA1_CONTEXT context;
  BYTE         localDigest[20];
  BYTE        *scratch;
  BYTE        *result;
  DWORD        half;
  DWORD        i;

  result = NULL;
  while (len && !*data) {
    ++data;
    --len;
  }
  if (len & 1) {
    ++data;
    --len;
  }

  half = len >> 1;
  scratch = (BYTE *)_alloca(half);
  i = 0;
  if (scratch) {
    for (; i < half; ++i) {
      scratch[i] = data[i * 2];
    }
    SHA1_Init(&context);
    SHA1_Update(&context, scratch, half);
    SHA1_Final(localDigest, &context);
    for (i = 0; i < 20; ++i) {
      digest[i * 2] = localDigest[i];
    }

    for (i = 0; i < half; ++i) {
      scratch[i] = data[i * 2 + 1];
    }
    SHA1_Init(&context);
    SHA1_Update(&context, scratch, half);
    SHA1_Final(localDigest, &context);
    for (i = 0; i < 20; ++i) {
      digest[i * 2 + 1] = localDigest[i];
    }
    result = digest;
  }

  return result;
}
