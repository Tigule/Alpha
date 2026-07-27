#include <storm.h>

#include <malloc.h>
#include <stdlib.h>

static BYTE pads[64] = {0x80};

namespace Private {

  DWORD S(DWORD x, int n) {
    return _lrotl(x, n);
  }

  void R1(DWORD a, DWORD &b, DWORD c, DWORD d, DWORD &e, DWORD &w) {
    e += S(a, 5) + ((b & c) | (~b & d)) + w + 0x5A827999;
    b = S(b, 30);
    w = 0;
  }

  void R2(DWORD a, DWORD &b, DWORD c, DWORD d, DWORD &e, DWORD &w) {
    e += S(a, 5) + (b ^ c ^ d) + w + 0x6ED9EBA1;
    b = S(b, 30);
    w = 0;
  }

  void R3(DWORD a, DWORD &b, DWORD c, DWORD d, DWORD &e, DWORD &w) {
    e += S(a, 5) + ((b & c) | ((b | c) & d)) + w + 0x8F1BBCDC;
    b = S(b, 30);
    w = 0;
  }

  void R4(DWORD a, DWORD &b, DWORD c, DWORD d, DWORD &e, DWORD &w) {
    e += S(a, 5) + (b ^ c ^ d) + w + 0xCA62C1D6;
    b = S(b, 30);
    w = 0;
  }

  void Load(DWORD &a, const BYTE *const b) {
    a = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
  }

  void Save(unsigned __int64 a, BYTE *const b) {
    int i;

    for (i = 7; i >= 0; i--) {
      b[i] = (BYTE)a;
      a >>= 8;
    }
  }

  void Save(DWORD a, BYTE *const b) {
    int i;

    for (i = 3; i >= 0; i--) {
      b[i] = (BYTE)a;
      a >>= 8;
    }
  }

}  // namespace Private

void Sha1::Pump(unsigned long *const hash, const unsigned char *const data) {
  DWORD w[80];
  int   i;
  DWORD a;
  DWORD c;
  DWORD b;
  DWORD e;
  DWORD d;

  for (i = 0; i < 16; i++) {
    Private::Load(w[i], data + i * 4);
  }

  for (i = 16; i < 80; i++) {
    w[i] = Private::S(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
  }

  a = hash[0];
  b = hash[1];
  c = hash[2];
  d = hash[3];
  e = hash[4];

  for (i = 0; i < 20; i += 5) {
    Private::R1(a, b, c, d, e, w[i]);
    Private::R1(e, a, b, c, d, w[i + 1]);
    Private::R1(d, e, a, b, c, w[i + 2]);
    Private::R1(c, d, e, a, b, w[i + 3]);
    Private::R1(b, c, d, e, a, w[i + 4]);
  }
  for (; i < 40; i += 5) {
    Private::R2(a, b, c, d, e, w[i]);
    Private::R2(e, a, b, c, d, w[i + 1]);
    Private::R2(d, e, a, b, c, w[i + 2]);
    Private::R2(c, d, e, a, b, w[i + 3]);
    Private::R2(b, c, d, e, a, w[i + 4]);
  }
  for (; i < 60; i += 5) {
    Private::R3(a, b, c, d, e, w[i]);
    Private::R3(e, a, b, c, d, w[i + 1]);
    Private::R3(d, e, a, b, c, w[i + 2]);
    Private::R3(c, d, e, a, b, w[i + 3]);
    Private::R3(b, c, d, e, a, w[i + 4]);
  }
  for (; i < 80; i += 5) {
    Private::R4(a, b, c, d, e, w[i]);
    Private::R4(e, a, b, c, d, w[i + 1]);
    Private::R4(d, e, a, b, c, w[i + 2]);
    Private::R4(c, d, e, a, b, w[i + 3]);
    Private::R4(b, c, d, e, a, w[i + 4]);
  }

  hash[0] += a;
  hash[1] += b;
  hash[2] += c;
  hash[3] += d;
  hash[4] += e;
}

void Sha1::Initialize() {
  m_size = 0;
  m_hash[0] = 0x67452301;
  m_hash[1] = 0xEFCDAB89;
  m_hash[2] = 0x98BADCFE;
  m_hash[3] = 0x10325476;
  m_hash[4] = 0xC3D2E1F0;
}

void Sha1::Append(const void *_data, unsigned long size) {
  const BYTE *cursor;
  DWORD       offset;

  cursor = (const BYTE *)_data;
  offset = ((DWORD)m_size >> 3) & DATA_MASK;
  m_size += size * 8ui64;

  if (offset) {
    size += offset;
    cursor -= offset;
    if (size >= DATA_SIZE) {
      while (offset < DATA_SIZE) {
        m_data[offset] = cursor[offset];
        offset++;
      }
      Pump(m_hash, m_data);
      cursor += DATA_SIZE;
      size -= DATA_SIZE;
      offset = 0;
    }
  }

  while (size >= DATA_SIZE) {
    Pump(m_hash, cursor);
    cursor += DATA_SIZE;
    size -= DATA_SIZE;
  }

  while (offset < size) {
    m_data[offset] = cursor[offset];
    offset++;
  }
}

void Sha1::Finalize(unsigned char *const hash) {
  BYTE          size[8];
  unsigned long padBytes;
  int           i;

  Private::Save(m_size, size);
  padBytes = (0xFFFFFFF7 - ((DWORD)m_size >> 3)) & DATA_MASK;
  padBytes++;
  Append(pads, padBytes);
  Append(size, sizeof(size));

  for (i = 0; i < 5; i++) {
    Private::Save(m_hash[i], hash + i * 4);
  }
}
