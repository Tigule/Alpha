#define STORM_BIGNUM_METHOD_IMPLEMENTATION

#include <storm.h>

#include <malloc.h>

#define SIGNATURE_MAGIC 0x5349474E

class SSignatureData {
 public:
  unsigned long  modulusSize;
  unsigned long  pubExponentSize;
  unsigned long  buffered;
  unsigned long  signatureLength;
  unsigned char *tail;
  unsigned long  reserved;
  Sha1           hash;
};

namespace Signature {

  int HasMagic(const unsigned char *data, unsigned long size, unsigned long modulusSize, unsigned long &dataSize) {
    dataSize = size - modulusSize - sizeof(DWORD);
    return size >= modulusSize + sizeof(DWORD) && *(const DWORD *)(data + dataSize) == SIGNATURE_MAGIC;
  }

  void Hash(const unsigned char *data, unsigned long size, unsigned char *const digest) {
    Sha1 sha;

    sha.Initialize();
    sha.Append(data, size);
    sha.Finalize(digest);
  }

}  // namespace Signature

extern "C" void SSignatureVerifyStream_Begin(SSignatureData **token, unsigned long modulusSize, unsigned long pubExponentSize) {
  *token = new SSignatureData;
  (*token)->modulusSize = modulusSize;
  (*token)->pubExponentSize = pubExponentSize;
  (*token)->buffered = 0;
  (*token)->signatureLength = modulusSize + sizeof(DWORD);
  (*token)->tail = (unsigned char *)SMemAlloc((*token)->signatureLength, __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY);
  (*token)->hash.Initialize();
}

extern "C" unsigned long SSignatureVerifyStream_GetSignatureLength(SSignatureData *token) {
  return token->signatureLength;
}

extern "C" void SSignatureVerifyStream_ProvideData(SSignatureData *token, const unsigned char *data, unsigned long size) {
  long overflow;
  long hashBytes;

  ASSERT(token);

  hashBytes = size - token->signatureLength;
  if (hashBytes >= 0) {
    if (token->buffered) {
      token->hash.Append(token->tail, token->buffered);
    }
    if (hashBytes > 0) {
      token->hash.Append(data, hashBytes);
    }
    memcpy(token->tail, data + hashBytes, token->signatureLength);
    token->buffered = token->signatureLength;
    return;
  }

  overflow = token->buffered + size - token->signatureLength;
  if (overflow > 0) {
    token->hash.Append(token->tail, overflow);
    token->buffered -= overflow;
    memmove(token->tail, token->tail + overflow, token->buffered);
  }
  memcpy(token->tail + token->buffered, data, size);
  token->buffered += size;
}

extern "C" int SSignatureVerifyStream_Finish(SSignatureData *token, const unsigned char *modulus, const unsigned char *pubExponent) {
  int result;

  ASSERT(token);
  result = FALSE;

  if (token->buffered == token->signatureLength && *(DWORD *)token->tail == SIGNATURE_MAGIC) {
    unsigned char *generated;
    unsigned char *stored;

    generated = (unsigned char *)_alloca(token->modulusSize);
    memset(generated, 0xBB, token->modulusSize);
    generated[token->modulusSize - 1] = 0x0B;
    token->hash.Finalize(generated);
    stored = (unsigned char *)_alloca(token->modulusSize);
    memcpy(stored, token->tail + sizeof(DWORD), token->modulusSize);
    Crypt::RSA decoder;
    decoder.Prepare(modulus, token->modulusSize, pubExponent, token->pubExponentSize);
    decoder.Process(stored, token->modulusSize);
    result = memcmp(stored, generated, token->modulusSize) == 0;
  }

  SMemFree(token->tail, __FILE__, __LINE__, 0);
  delete token;
  return result;
}

extern "C" int SSignatureVerify(
    const unsigned char *data,
    unsigned long        size,
    const unsigned char *modulus,
    unsigned long        modulusSize,
    const unsigned char *pubExponent,
    unsigned long        pubExponentSize
) {
  SSignatureData *token;

  SSignatureVerifyStream_Begin(&token, modulusSize, pubExponentSize);
  SSignatureVerifyStream_ProvideData(token, data, size);
  return SSignatureVerifyStream_Finish(token, modulus, pubExponent);
}

extern "C" int SSignatureGenerate(
    unsigned char       *data,
    unsigned long       &size,
    const unsigned char *modulus,
    unsigned long        modulusSize,
    const unsigned char *privExponent,
    unsigned long        privExponentSize,
    const unsigned char *pubExponent,
    unsigned long        pubExponentSize
) {
  unsigned char *original;
  unsigned char *generated;
  unsigned long  dataSize;
  unsigned char *check;

  if (!Signature::HasMagic(data, size, modulusSize, dataSize)) {
    dataSize = size;
  }

  original = (unsigned char *)_alloca(modulusSize);
  memset(original, 0xBB, modulusSize);
  original[modulusSize - 1] = 0x0B;
  Signature::Hash(data, dataSize, original);
  generated = (unsigned char *)_alloca(modulusSize);
  memcpy(generated, original, modulusSize);

  Crypt::RSA encoder;
  encoder.Prepare(modulus, modulusSize, privExponent, privExponentSize);
  encoder.Process(generated, modulusSize);

  check = (unsigned char *)_alloca(modulusSize);
  memcpy(check, generated, modulusSize);
  Crypt::RSA decoder;
  decoder.Prepare(modulus, modulusSize, pubExponent, pubExponentSize);
  decoder.Process(check, modulusSize);
  if (memcmp(check, original, modulusSize) != 0) {
    return FALSE;
  }

  *(DWORD *)(data + dataSize) = SIGNATURE_MAGIC;
  memcpy(data + dataSize + sizeof(DWORD), generated, modulusSize);
  size = dataSize + sizeof(DWORD) + modulusSize;
  return TRUE;
}

Crypt::RSA::RSA() {
}

Crypt::RSA::~RSA() {
}
