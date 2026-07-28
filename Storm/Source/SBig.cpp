#include <storm.h>
#include <stpl.h>

#include <ctype.h>
#include <new>

class BigBuffer {
 private:
  TSGrowableArray<UINT> m_data;
  UINT                  m_offset;

  void GrowToFit(UINT index);

 public:
  BigBuffer();
  UINT      &operator[](UINT index);
  UINT       operator[](UINT index) const;
  void       Clear();
  UINT       Count() const;
  int        IsUsed(UINT index) const;
  void       SetCount(UINT count);
  void       SetOffset(UINT offset);
  void       Trim() const;
};

class BigStack {
 private:
  enum {
    SIZE = 16
  };

  BigBuffer m_buffer[SIZE];
  UINT      m_used;

 public:
  BigStack();
  BigBuffer &Alloc(UINT *count);
  void       Free(UINT count);
  BigBuffer &MakeDistinct(BigBuffer &orig, int required);
  void       UnmakeDistinct(BigBuffer &orig, BigBuffer &distinct);
};

typedef TSGrowableArray_<BYTE, 0x53424947, 102> SBigOutputArray;

class BigData {
 private:
  BigBuffer       m_primary;
  BigStack        m_stack;
  SBigOutputArray m_output;

 public:
  BigBuffer       &Primary();
  const BigBuffer &Primary() const;
  BigStack        &Stack() const;
  SBigOutputArray &Output() const;
};

static const DWORD SMALL_PRIMES[171] = {3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,  41,  43,  47,  53,  59,   61,   67,   71,
                                        73,  79,  83,  89,  97,  101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151,  157,  163,  167,
                                        173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257,  263,  269,  271,
                                        277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373,  379,  383,  389,
                                        397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487,  491,  499,  503,
                                        509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 613,  617,  619,  631,
                                        641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739,  743,  751,  757,
                                        761, 769, 773, 787, 797, 809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863,  877,  881,  883,
                                        887, 907, 911, 919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997, 1009, 1013, 1019, 1021};

static const DWORD FERMAT_WITNESS[1] = {2};
static const BYTE  initSeed[8] = {0xB5, 0x3B, 0x12, 0x1F, 0xE5, 0x55, 0x9A, 0x15};
static const BYTE  initMul[8] = {0x50, 0x46, 0x00, 0x00, 0x69, 0x90, 0x00, 0x00};

void TSSwap(BYTE &a, BYTE &b) {
  BYTE temp = a;
  a = b;
  b = temp;
}

static UINT ExtractLowPart(unsigned __int64 *b);
static UINT ExtractLowPartLargeSum(unsigned __int64 *carry, unsigned __int64 add);
static UINT ExtractLowPartSx(unsigned __int64 *b);
static void InsertLowPart(unsigned __int64 *b, UINT c);
static unsigned __int64 MakeLarge(UINT low, UINT high);
static void Add(BigBuffer &a, const BigBuffer &b, UINT c);
static void Add(BigBuffer &a, const BigBuffer &b, const BigBuffer &c);
static void And(BigBuffer &a, const BigBuffer &b, const BigBuffer &c);
static int Compare(const BigBuffer &a, UINT b);
static int Compare(const BigBuffer &a, const BigBuffer &b);
static void Div(BigBuffer &a, UINT *b, const BigBuffer &c, unsigned __int64 d);
static void Div(BigBuffer &a, BigBuffer &b, const BigBuffer &c, const BigBuffer &d, BigStack &stack);
static void FindPrime(BigBuffer &a, UINT b, const BigBuffer &c, const BigBuffer &d, BigStack &stack);
static void Gcd(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, BigStack &stack);
static UINT HighBitPos(const BigBuffer &a);
static void InvMod(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, BigStack &stack);
static int IsEven(const BigBuffer &a);
static int IsOdd(const BigBuffer &a);
static int IsOne(const BigBuffer &a);
static int IsPrime(const BigBuffer &a, BigStack &stack);
static int IsZero(const BigBuffer &a);
static UINT LowBitPos(const BigBuffer &a);
static void Mul(BigBuffer &a, const BigBuffer &b, unsigned __int64 c);
static void Mul(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, BigStack &stack);
static void MulMod(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, const BigBuffer &d, BigStack &stack);
static void Not(BigBuffer &a, const BigBuffer &b);
static void Or(BigBuffer &a, const BigBuffer &b, const BigBuffer &c);
static void Pow(BigBuffer &a, const BigBuffer &b, UINT c, BigStack &stack);
static void PowMod(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, const BigBuffer &d, BigStack &stack);
static void Rand(BigBuffer &a, const BigBuffer &b, BigBuffer &seed, BigStack &stack);
static void Set2Exp(BigBuffer &a, UINT b);
static void SetOne(BigBuffer &a);
static void SetZero(BigBuffer &a);
static void Shl(BigBuffer &a, const BigBuffer &b, UINT c);
static void Shr(BigBuffer &a, const BigBuffer &b, UINT c);
static void Square(BigBuffer &a, const BigBuffer &b, BigStack &stack);
static void Sub(BigBuffer &a, const BigBuffer &b, UINT c);
static void Sub(BigBuffer &a, const BigBuffer &b, const BigBuffer &c);
static void Xor(BigBuffer &a, const BigBuffer &b, const BigBuffer &c);
static void DecodeDataBytes(const void *data, UINT maxBytes, UINT *offset, UINT *dataBytes);
static void EncodeDataBytes(SBigOutputArray &output, UINT dataBytes);
static void FromBinary(BigBuffer &a, const void *data, UINT bytes);
static void FromStr(BigBuffer &a, const char *str);
static void FromStream(BigBuffer &a, const void *data, UINT maxBytes, UINT *bytes);
static void FromUnsigned(BigBuffer &a, UINT val);
static void ToBinary(SBigOutputArray &output, const BigBuffer &a);
static void ToBinaryAppend(SBigOutputArray &output, const BigBuffer &a);
static void ToStr(SBigOutputArray &output, const BigBuffer &a, BigStack &stack);
static void ToStream(SBigOutputArray &output, const BigBuffer &a);
static void ToUnsigned(UINT *val, const BigBuffer &a);

void BigBuffer::GrowToFit(UINT index) {
  m_data.GrowToFit(m_offset + index, 1);
}

BigBuffer::BigBuffer() : m_offset(0) {
}

UINT &BigBuffer::operator[](UINT index) {
  GrowToFit(index);
  return m_data[m_offset + index];
}

UINT BigBuffer::operator[](UINT index) const {
  return IsUsed(index) ? m_data.Ptr()[m_offset + index] : 0;
}

void BigBuffer::Clear() {
  m_data.SetCount(m_offset);
}

UINT BigBuffer::Count() const {
  return m_data.Count() - m_offset;
}

int BigBuffer::IsUsed(UINT index) const {
  return m_offset + index < m_data.Count();
}

void BigBuffer::SetCount(UINT count) {
  m_data.SetCount(m_offset + count);
}

void BigBuffer::SetOffset(UINT offset) {
  m_offset = offset;
  if (m_offset) {
    GrowToFit(-1);
  }
}

void BigBuffer::Trim() const {
  UINT count = Count();
  while (count && !(*this)[count - 1]) {
    --count;
  }
  ((BigBuffer *)this)->SetCount(count);
}

BigStack::BigStack() : m_used(0) {
}

BigBuffer &BigStack::Alloc(UINT *count) {
  if (m_used >= 0x10) {
    SErrDisplayError(0x85100000, __FILE__, __LINE__, "m_used < SIZE", NULL, 1);
  }
  if (count) {
    ++*count;
  }
  return m_buffer[m_used++];
}

void BigStack::Free(UINT count) {
  if (count > m_used) {
    SErrDisplayError(0x85100000, __FILE__, __LINE__, "count <= m_used", NULL, 1);
  }
  m_used -= count;
}

BigBuffer &BigStack::MakeDistinct(BigBuffer &orig, int required) {
  return required ? Alloc(NULL) : orig;
}

void BigStack::UnmakeDistinct(BigBuffer &orig, BigBuffer &distinct) {
  if (&orig != &distinct) {
    orig = distinct;
    Free(1);
  }
}

BigBuffer &BigData::Primary() {
  return m_primary;
}

const BigBuffer &BigData::Primary() const {
  return m_primary;
}

BigStack &BigData::Stack() const {
  return (BigStack &)m_stack;
}

SBigOutputArray &BigData::Output() const {
  return (SBigOutputArray &)m_output;
}

static UINT ExtractLowPart(unsigned __int64 *b) {
  UINT result;
  result = (UINT)*b;
  *b >>= 32;
  return result;
}

static UINT ExtractLowPartLargeSum(unsigned __int64 *carry, unsigned __int64 add) {
  UINT result;
  *carry += add;
  add = (unsigned __int64)(UINT)(*carry < add);
  result = ExtractLowPart(carry);
  *carry += add << 32;
  return result;
}

static UINT ExtractLowPartSx(unsigned __int64 *b) {
  UINT result;
  result = (UINT)*b;
  *b >>= 32;
  if (*b >= 0x80000000) {
    *b |= 0xFFFFFFFF00000000ui64;
  }
  return result;
}

static void InsertLowPart(unsigned __int64 *b, UINT c) {
  *b = (*b << 32) | c;
}

static unsigned __int64 MakeLarge(UINT low, UINT high) {
  return ((unsigned __int64)high << 32) + low;
}

static void Add(BigBuffer &a, const BigBuffer &b, UINT c) {
  unsigned __int64 carry = c;
  UINT             index = 0;
  while (carry || b.IsUsed(index)) {
    carry += b[index];
    a[index++] = ExtractLowPart(&carry);
  }
  a.SetCount(index);
}

static void Add(BigBuffer &a, const BigBuffer &b, const BigBuffer &c) {
  unsigned __int64 carry = 0;
  UINT             index = 0;
  while (carry || b.IsUsed(index) || c.IsUsed(index)) {
    carry += (unsigned __int64)b[index] + c[index];
    a[index++] = ExtractLowPart(&carry);
  }
  a.SetCount(index);
}

static void And(BigBuffer &a, const BigBuffer &b, const BigBuffer &c) {
  UINT index = 0;
  while (b.IsUsed(index) || c.IsUsed(index)) {
    a[index] = b[index] & c[index];
    ++index;
  }
  a.SetCount(index);
  a.Trim();
}

static int Compare(const BigBuffer &a, UINT b) {
  a.Trim();
  if (a.Count() > 1) {
    return 1;
  }
  if (!a.Count()) {
    return b ? -1 : 0;
  }
  return a[0] < b ? -1 : a[0] > b;
}

static int Compare(const BigBuffer &a, const BigBuffer &b) {
  UINT index = 0;
  int  result = 0;
  while (a.IsUsed(index) || b.IsUsed(index)) {
    if (a[index] != b[index]) {
      result = a[index] < b[index] ? -1 : 1;
    }
    ++index;
  }
  return result;
}

static void Div(BigBuffer &a, UINT *b, const BigBuffer &c, unsigned __int64 d) {
  unsigned __int64 data = 0;
  UINT             index = c.Count();
  a.SetCount(index);
  while (index) {
    InsertLowPart(&data, c[--index]);
    a[index] = (UINT)(data / d);
    data %= d;
  }
  a.Trim();
  if (b) {
    *b = (UINT)data;
  }
}

static void Div(BigBuffer &a, BigBuffer &b, const BigBuffer &c, const BigBuffer &d, BigStack &stack) {
  UINT       allocCount = 0;
  BigBuffer &quotient = stack.Alloc(&allocCount);
  BigBuffer &remainder = stack.Alloc(&allocCount);
  BigBuffer &work = stack.Alloc(&allocCount);
  UINT       shift;
  int        comparison;

  d.Trim();
  if (!d.Count()) {
    SetZero(quotient);
    SetZero(remainder);
  } else {
    remainder = c;
    remainder.Trim();
    SetZero(quotient);
    comparison = Compare(remainder, d);
    if (comparison >= 0) {
      shift = HighBitPos(remainder) - HighBitPos(d);
      Shl(work, d, shift);
      for (;;) {
        if (Compare(remainder, work) >= 0) {
          Sub(remainder, remainder, work);
          quotient[shift >> 5] |= 1u << (shift & 31);
        }
        if (!shift) {
          break;
        }
        Shr(work, work, 1);
        --shift;
      }
    }
  }
  a = quotient;
  b = remainder;
  stack.Free(allocCount);
}

static void FindPrime(BigBuffer &a, UINT b, const BigBuffer &c, const BigBuffer &d, BigStack &stack) {
  UINT       allocCount = 0;
  BigBuffer &t = stack.Alloc(&allocCount);
  BigBuffer &seed = stack.Alloc(&allocCount);
  BigBuffer &one = stack.Alloc(&allocCount);

  SetOne(one);
  Rand(a, c, seed, stack);
  if (b) {
    Set2Exp(t, b - 1);
    Or(a, a, t);
  }
  Or(a, a, one);
  while (!IsPrime(a, stack)) {
    Add(a, a, 2);
    if (!IsOne(d)) {
      Sub(t, a, one);
      Gcd(t, t, d, stack);
      if (!IsOne(t)) {
        continue;
      }
    }
  }
  stack.Free(allocCount);
}

static void Gcd(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, BigStack &stack) {
  UINT       allocCount = 0;
  BigBuffer &aa = stack.Alloc(&allocCount);
  BigBuffer &bb = stack.Alloc(&allocCount);
  BigBuffer &q = stack.Alloc(&allocCount);
  BigBuffer &r = stack.Alloc(&allocCount);
  aa = b;
  bb = c;
  while (!IsZero(bb)) {
    Div(q, r, aa, bb, stack);
    aa = bb;
    bb = r;
  }
  a = aa;
  stack.Free(allocCount);
}

static UINT HighBitPos(const BigBuffer &a) {
  UINT index = a.Count();

  while (index) {
    if (a[--index]) {
      UINT mask = 0x80000000;
      UINT bit = 32;

      do {
        --bit;
        if (a[index] & mask) {
          return index * 32 + bit;
        }
        mask >>= 1;
      } while (bit);
    }
  }
  return 0;
}

static void InvMod(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, BigStack &stack) {
  UINT       allocCount = 0;
  BigBuffer &t3 = stack.Alloc(&allocCount);
  BigBuffer &u3 = stack.Alloc(&allocCount);
  BigBuffer &v1 = stack.Alloc(&allocCount);
  BigBuffer &quotient = stack.Alloc(&allocCount);
  BigBuffer &remainder = stack.Alloc(&allocCount);
  int        sign;

  u3 = b;
  v1 = c;
  SetOne(a);
  SetZero(t3);
  sign = 1;
  while (!IsZero(v1)) {
    Div(quotient, remainder, u3, v1, stack);
    Mul(quotient, quotient, t3, stack);
    Add(quotient, a, quotient);
    a = t3;
    t3 = quotient;
    u3 = v1;
    v1 = remainder;
    sign = -sign;
  }
  if (sign < 0) {
    Sub(a, c, a);
  }
  stack.Free(allocCount);
}

static int IsEven(const BigBuffer &a) {
  return !a.Count() || !(a[0] & 1);
}

static int IsOdd(const BigBuffer &a) {
  return a.Count() && (a[0] & 1);
}

static int IsOne(const BigBuffer &a) {
  a.Trim();
  return a.Count() == 1 && a[0] == 1;
}

static int IsPrime(const BigBuffer &a, BigStack &stack) {
  UINT       b;
  UINT       remainder;
  UINT       j;
  UINT       allocCount = 0;
  BigBuffer &a1 = stack.Alloc(&allocCount);
  BigBuffer &m = stack.Alloc(&allocCount);
  BigBuffer &witness = stack.Alloc(&allocCount);
  BigBuffer &q = stack.Alloc(&allocCount);
  int        result = TRUE;

  a.Trim();
  if (IsZero(a) || IsEven(a)) {
    result = Compare(a, 2) == 0;
  }
  if (result) {
    for (b = 0; b < 171; ++b) {
      Div(q, &remainder, a, SMALL_PRIMES[b]);
      if (!remainder && Compare(a, SMALL_PRIMES[b])) {
        result = FALSE;
        break;
      }
    }
  }
  SetOne(a1);
  Sub(a1, a, a1);
  m = a1;
  b = LowBitPos(m);
  Shr(m, m, b);
  if (result) {
    for (j = 0; j < 6 && result; ++j) {
      FromUnsigned(witness, SMALL_PRIMES[j]);
      if (Compare(witness, a) >= 0) {
        break;
      }
      PowMod(witness, witness, m, a, stack);
      if (IsOne(witness) || !Compare(witness, a1)) {
        continue;
      }
      for (remainder = 1; remainder < b; ++remainder) {
        MulMod(witness, witness, witness, a, stack);
        if (!Compare(witness, a1)) {
          break;
        }
      }
      if (remainder == b) {
        result = FALSE;
      }
    }
  }
  if (result) {
    FromUnsigned(witness, FERMAT_WITNESS[0]);
    PowMod(witness, witness, a, a, stack);
    result = Compare(witness, FERMAT_WITNESS[0]) == 0;
  }
  stack.Free(allocCount);
  return result;
}

static int IsZero(const BigBuffer &a) {
  a.Trim();
  return !a.Count();
}

static UINT LowBitPos(const BigBuffer &a) {
  UINT index;
  UINT bit;
  for (index = 0; index < a.Count(); ++index) {
    if (a[index]) {
      for (bit = 0; bit < 32; ++bit) {
        if (a[index] & (1u << bit)) {
          return index * 32 + bit;
        }
      }
    }
  }
  return 0;
}

static void Mul(BigBuffer &a, const BigBuffer &b, unsigned __int64 c) {
  unsigned __int64 carry = 0;
  UINT             index = 0;
  c = MakeLarge((UINT)c, (UINT)(c >> 32));
  while (b.IsUsed(index) || carry) {
    carry += (unsigned __int64)b[index] * c;
    a[index++] = ExtractLowPart(&carry);
  }
  a.SetCount(index);
  a.Trim();
}

static void Mul(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, BigStack &stack) {
  unsigned __int64 carry;
  unsigned __int64 product;
  unsigned __int64 sum;
  UINT             bIndex;
  UINT             cIndex;
  UINT             allocCount = 0;
  BigBuffer       &aa = stack.MakeDistinct(a, &a == &b || &a == &c);

  aa.SetCount(b.Count() + c.Count());
  for (bIndex = 0; bIndex < aa.Count(); ++bIndex) {
    aa[bIndex] = 0;
  }
  for (bIndex = 0; bIndex < b.Count(); ++bIndex) {
    carry = 0;
    for (cIndex = 0; cIndex < c.Count(); ++cIndex) {
      product = (unsigned __int64)b[bIndex] * c[cIndex];
      sum = (unsigned __int64)aa[bIndex + cIndex] + carry;
      aa[bIndex + cIndex] = ExtractLowPartLargeSum(&product, sum);
      carry = product;
    }
    aa[bIndex + c.Count()] = (UINT)carry;
  }
  aa.Trim();
  stack.UnmakeDistinct(a, aa);
  (void)allocCount;
}

static void MulMod(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, const BigBuffer &d, BigStack &stack) {
  UINT       allocCount = 0;
  BigBuffer &product = stack.Alloc(&allocCount);
  BigBuffer &quotient = stack.Alloc(&allocCount);
  Mul(product, b, c, stack);
  Div(quotient, a, product, d, stack);
  stack.Free(allocCount);
}

static void Not(BigBuffer &a, const BigBuffer &b) {
  UINT index = 0;

  while (b.IsUsed(index)) {
    a[index] = ~b[index];
    ++index;
  }
  a.SetCount(index);
}

static void Or(BigBuffer &a, const BigBuffer &b, const BigBuffer &c) {
  UINT index = 0;

  while (b.IsUsed(index) || c.IsUsed(index)) {
    a[index] = b[index] | c[index];
    ++index;
  }
  a.SetCount(index);
}

static void Pow(BigBuffer &a, const BigBuffer &b, UINT c, BigStack &stack) {
  UINT bit = c;
  UINT scan;

  for (scan = c & (c - 1); scan; scan &= scan - 1) {
    bit = scan;
  }

  BigBuffer &aa = stack.MakeDistinct(a, &a == &b);
  aa = b;
  while (bit > 1) {
    Mul(aa, aa, aa, stack);
    bit >>= 1;
    if (bit & c) {
      Mul(aa, aa, b, stack);
    }
  }
  stack.UnmakeDistinct(a, aa);
}

static void PowMod(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, const BigBuffer &d, BigStack &stack) {
  BigBuffer *bPower[3];
  UINT       index;
  UINT       ciBits;
  UINT       allocCount = 0;

  c.Trim();
  if (!c.Count()) {
    SetOne(a);
    return;
  }

  BigBuffer &temp = stack.Alloc(&allocCount);
  BigBuffer &b2 = stack.Alloc(&allocCount);
  BigBuffer &b3 = stack.Alloc(&allocCount);
  bPower[0] = (BigBuffer *)&b;
  bPower[1] = &b2;
  bPower[2] = &b3;
  MulMod(b2, b, b, d, stack);
  MulMod(b3, b2, b, d, stack);

  BigBuffer &aa = stack.MakeDistinct(a, &a == &b || &a == &c || &a == &d);
  SetOne(aa);
  index = c.Count();
  while (index) {
    UINT ci = c[--index];
    ciBits = 32;
    if (index == c.Count() - 1) {
      while (!(ci & 0xC0000000)) {
        ci <<= 2;
        ciBits -= 2;
      }
    }
    ciBits = (ciBits + 1) / 2;
    while (ciBits) {
      Square(aa, aa, stack);
      Div(temp, aa, aa, d, stack);
      Square(aa, aa, stack);
      Div(temp, aa, aa, d, stack);
      if (ci >> 30) {
        MulMod(aa, aa, *bPower[(ci >> 30) - 1], d, stack);
      }
      ci <<= 2;
      --ciBits;
    }
  }
  stack.UnmakeDistinct(a, aa);
  stack.Free(allocCount);
}

static void Rand(BigBuffer &a, const BigBuffer &b, BigBuffer &seed, BigStack &stack) {
  UINT       allocCount = 0;
  int        reinit;
  UINT       loop;
  BigBuffer &mul = stack.Alloc(&allocCount);
  BigBuffer &work = stack.Alloc(&allocCount);
  BigBuffer &quotient = stack.Alloc(&allocCount);

  seed.Trim();
  reinit = seed.Count() < 2;
  if (reinit) {
    FromBinary(seed, initSeed, sizeof(initSeed));
  }
  FromBinary(mul, initMul, sizeof(initMul));
  a.SetCount(b.Count());
  for (loop = 0; loop < b.Count(); ++loop) {
    Mul(work, seed, mul, stack);
    Add(work, work, loop + 1);
    seed = work;
    a[loop] = seed[0] ^ seed[1];
  }
  Div(quotient, a, a, b, stack);
  stack.Free(allocCount);
}

static void Set2Exp(BigBuffer &a, UINT b) {
  UINT count = (b >> 5) + 1;
  UINT index;
  a.SetCount(count);
  for (index = 0; index < count; ++index) {
    a[index] = 0;
  }
  a[b >> 5] = 1u << (b & 31);
}

static void SetOne(BigBuffer &a) {
  a.SetCount(1);
  a[0] = 1;
}

static void SetZero(BigBuffer &a) {
  a.Clear();
}

static void Shl(BigBuffer &a, const BigBuffer &b, UINT c) {
  UINT aCount = b.Count() + (c >> 5) + 1;
  UINT cBits = c & 31;
  UINT index = aCount;

  while (index) {
    UINT out = --index;
    UINT source = out - (c >> 5);
    UINT value = source < b.Count() ? b[source] << cBits : 0;
    if (cBits && source && source - 1 < b.Count()) {
      value += b[source - 1] >> (32 - cBits);
    }
    a[out] = value;
  }
  a.SetCount(aCount);
  a.Trim();
}

static void Shr(BigBuffer &a, const BigBuffer &b, UINT c) {
  UINT word = c >> 5;
  UINT cBits = c & 31;
  UINT index = 0;

  while (b.IsUsed(index + word)) {
    UINT value = b[index + word] >> cBits;
    if (cBits) {
      value += b[index + word + 1] << (32 - cBits);
    }
    a[index++] = value;
  }
  a.SetCount(index);
}

static void Square(BigBuffer &a, const BigBuffer &b, BigStack &stack) {
  unsigned __int64 add;
  unsigned __int64 mul;
  unsigned __int64 carry;
  BigBuffer       &aa = stack.MakeDistinct(a, &a == &b);

  aa.Clear();
  for (UINT bIndex = 0; b.IsUsed(bIndex); ++bIndex) {
    carry = 0;
    for (UINT cIndex = 0; cIndex <= bIndex; ++cIndex) {
      mul = (unsigned __int64)b[cIndex] * b[bIndex];
      add = (unsigned __int64)aa[bIndex + cIndex] + mul;
      if (cIndex < bIndex) {
        carry += mul;
      }
      aa[bIndex + cIndex] = ExtractLowPartLargeSum(&carry, add);
    }
    aa[bIndex + bIndex + 1] = ExtractLowPart(&carry);
  }
  stack.UnmakeDistinct(a, aa);
}

static void Sub(BigBuffer &a, const BigBuffer &b, UINT c) {
  unsigned __int64 borrow = 0 - static_cast<unsigned __int64>(c);
  UINT             index = 0;

  while (b.IsUsed(index)) {
    borrow += b[index];
    a[index++] = ExtractLowPartSx(&borrow);
  }
  a.SetCount(index);
  if (borrow) {
    SErrDisplayError(0x85100000, __FILE__, __LINE__, "!borrow", NULL, 1);
  }
}

static void Sub(BigBuffer &a, const BigBuffer &b, const BigBuffer &c) {
  unsigned __int64 borrow = 0;
  UINT             index = 0;
  while (borrow || b.IsUsed(index) || c.IsUsed(index)) {
    borrow = (unsigned __int64)b[index] - c[index] - (UINT)borrow;
    a[index++] = ExtractLowPartSx(&borrow);
  }
  a.SetCount(index);
  a.Trim();
}

static void Xor(BigBuffer &a, const BigBuffer &b, const BigBuffer &c) {
  UINT index = 0;

  while (b.IsUsed(index) || c.IsUsed(index)) {
    a[index] = b[index] ^ c[index];
    ++index;
  }
  a.SetCount(index);
}

static void DecodeDataBytes(const void *data, UINT maxBytes, UINT *offset, UINT *dataBytes) {
  *offset = 0;
  *dataBytes = 0;
  while (*offset < maxBytes) {
    UINT value = ((const BYTE *)data)[(*offset)++];
    if (value == 0xFF) {
      break;
    }
    *dataBytes = value + 0xFF * *dataBytes;
  }
  if (*dataBytes >= maxBytes - *offset) {
    *dataBytes = maxBytes - *offset;
  }
}

static void EncodeDataBytes(SBigOutputArray &output, UINT dataBytes) {
  while (dataBytes) {
    *output.New() = (BYTE)(dataBytes % 0xFF);
    dataBytes /= 0xFF;
  }
  *output.New() = 0xFF;
}

static void FromBinary(BigBuffer &a, const void *data, UINT bytes) {
  UINT byte;
  a.Clear();
  for (byte = 0; byte < bytes; ++byte) {
    UINT index = byte >> 2;
    if (!(byte & 3)) {
      a[index] = 0;
    }
    a[index] |= (UINT)((const BYTE *)data)[byte] << ((byte & 3) * 8);
  }
  a.Trim();
}

static void FromStr(BigBuffer &a, const char *str) {
  a.Clear();
  while (*str) {
    Mul(a, a, (unsigned __int64)10);
    Add(a, a, (UINT)(*str++ - '0'));
  }
}

static void FromStream(BigBuffer &a, const void *data, UINT maxBytes, UINT *bytes) {
  UINT offset;
  UINT dataBytes;
  DecodeDataBytes(data, maxBytes, &offset, &dataBytes);
  FromBinary(a, (const BYTE *)data + offset, dataBytes);
  if (bytes) {
    *bytes = offset + dataBytes;
  }
}

static void FromUnsigned(BigBuffer &a, UINT val) {
  a[0] = val;
  a.SetCount(1);
}

static void ToBinary(SBigOutputArray &output, const BigBuffer &a) {
  output.SetCount(0);
  ToBinaryAppend(output, a);
}

static void ToBinaryAppend(SBigOutputArray &output, const BigBuffer &a) {
  UINT byte;

  for (byte = 0; byte < a.Count() * 4; ++byte) {
    UINT value = a[byte >> 2] >> ((byte & 3) * 8);
    if (value || (byte >> 2) + 1 < a.Count()) {
      *output.New() = (BYTE)value;
    }
  }
}

static void ToStr(SBigOutputArray &output, const BigBuffer &a, BigStack &stack) {
  UINT       count;
  UINT       remainder;
  UINT       allocCount = 0;
  UINT       ch;
  BigBuffer &work = stack.Alloc(&allocCount);

  work = a;
  output.SetCount(0);
  do {
    BYTE digit;

    Div(work, &remainder, work, 10);
    digit = (BYTE)('0' + remainder);
    output.Add(&digit);
  } while (Compare(work, 0));

  count = output.Count();
  for (ch = 0; ch < count / 2; ++ch) {
    TSSwap(output[ch], output[count - ch - 1]);
  }
  *output.New() = 0;
  stack.Free(allocCount);
}

static void ToStream(SBigOutputArray &output, const BigBuffer &a) {
  UINT dataBytes;
  ToBinary(output, a);
  dataBytes = output.Count();
  EncodeDataBytes(output, dataBytes);
  ToBinaryAppend(output, a);
}

static void ToUnsigned(UINT *val, const BigBuffer &a) {
  if (val) {
    *val = a[0];
  }
}

extern "C" void APIENTRY SBigAdd(BigData *a, const BigData &b, const BigData &c) {
  Add(a->Primary(), b.Primary(), c.Primary());
}

extern "C" void APIENTRY SBigAnd(BigData *a, const BigData &b, const BigData &c) {
  And(a->Primary(), b.Primary(), c.Primary());
}

extern "C" void APIENTRY SBigBitLen(BigData *a, UINT *bits) {
  *bits = IsZero(a->Primary()) ? 0 : HighBitPos(a->Primary()) + 1;
}

extern "C" int APIENTRY SBigCompare(const BigData &b, const BigData &c) {
  return Compare(b.Primary(), c.Primary());
}

extern "C" void APIENTRY SBigCopy(BigData *a, const BigData &b) {
  a->Primary() = b.Primary();
}

extern "C" void APIENTRY SBigDec(BigData *a, const BigData &b) {
  Sub(a->Primary(), b.Primary(), 1);
}

extern "C" void APIENTRY SBigDel(BigData *num) {
  delete num;
}

extern "C" void APIENTRY SBigDiv(BigData *a, const BigData &b, const BigData &c) {
  UINT       allocCount = 0;
  BigBuffer &remainder = a->Stack().Alloc(&allocCount);
  Div(a->Primary(), remainder, b.Primary(), c.Primary(), a->Stack());
  a->Stack().Free(allocCount);
}

extern "C" void APIENTRY SBigFindPrime(BigData *a, UINT b, const BigData &c, const BigData &d) {
  FindPrime(a->Primary(), b, c.Primary(), d.Primary(), a->Stack());
}

extern "C" void APIENTRY SBigFromBinary(BigData *num, const void *data, UINT bytes) {
  FromBinary(num->Primary(), data, bytes);
}

extern "C" void APIENTRY SBigFromStr(BigData *num, const char *str) {
  FromStr(num->Primary(), str);
}

extern "C" void APIENTRY SBigFromStream(BigData *num, const void *data, UINT maxBytes, UINT *bytes) {
  FromStream(num->Primary(), data, maxBytes, bytes);
}

extern "C" void APIENTRY SBigFromUnsigned(BigData *num, UINT val) {
  FromUnsigned(num->Primary(), val);
}

extern "C" void APIENTRY SBigGcd(BigData *a, const BigData &b, const BigData &c) {
  Gcd(a->Primary(), b.Primary(), c.Primary(), a->Stack());
}

extern "C" void APIENTRY SBigInc(BigData *a, const BigData &b) {
  Add(a->Primary(), b.Primary(), 1);
}

extern "C" void APIENTRY SBigInvMod(BigData *a, const BigData &b, const BigData &c) {
  InvMod(a->Primary(), b.Primary(), c.Primary(), a->Stack());
}

extern "C" int APIENTRY SBigIsEven(const BigData &a) {
  return IsEven(a.Primary());
}

extern "C" int APIENTRY SBigIsOdd(const BigData &a) {
  return IsOdd(a.Primary());
}

extern "C" int APIENTRY SBigIsOne(const BigData &a) {
  return IsOne(a.Primary());
}

extern "C" int APIENTRY SBigIsPrime(const BigData &a) {
  return IsPrime(a.Primary(), a.Stack());
}

extern "C" int APIENTRY SBigIsZero(const BigData &a) {
  return IsZero(a.Primary());
}

extern "C" void APIENTRY SBigMod(BigData *a, const BigData &b, const BigData &c) {
  UINT       allocCount = 0;
  BigBuffer &quotient = a->Stack().Alloc(&allocCount);
  Div(quotient, a->Primary(), b.Primary(), c.Primary(), a->Stack());
  a->Stack().Free(allocCount);
}

extern "C" void APIENTRY SBigMul(BigData *a, const BigData &b, const BigData &c) {
  Mul(a->Primary(), b.Primary(), c.Primary(), a->Stack());
}

extern "C" void APIENTRY SBigMulMod(BigData *a, const BigData &b, const BigData &c, const BigData &d) {
  MulMod(a->Primary(), b.Primary(), c.Primary(), d.Primary(), a->Stack());
}

extern "C" void APIENTRY SBigNew(BigData **num) {
  *num = NEW(BigData);
}

extern "C" void APIENTRY SBigNot(BigData *a, const BigData &b) {
  Not(a->Primary(), b.Primary());
}

extern "C" void APIENTRY SBigOr(BigData *a, const BigData &b, const BigData &c) {
  Or(a->Primary(), b.Primary(), c.Primary());
}

extern "C" void APIENTRY SBigPow(BigData *a, const BigData &b, UINT c) {
  Pow(a->Primary(), b.Primary(), c, a->Stack());
}

extern "C" void APIENTRY SBigPowMod(BigData *a, const BigData &b, const BigData &c, const BigData &d) {
  PowMod(a->Primary(), b.Primary(), c.Primary(), d.Primary(), a->Stack());
}

extern "C" void APIENTRY SBigRand(BigData *a, const BigData &b, BigData *seed) {
  Rand(a->Primary(), b.Primary(), seed->Primary(), a->Stack());
}

extern "C" void APIENTRY SBigSet2Exp(BigData *a, UINT b) {
  Set2Exp(a->Primary(), b);
}

extern "C" void APIENTRY SBigSetOne(BigData *a) {
  SetOne(a->Primary());
}

extern "C" void APIENTRY SBigSetZero(BigData *a) {
  SetZero(a->Primary());
}

extern "C" void APIENTRY SBigShl(BigData *a, const BigData &b, UINT c) {
  Shl(a->Primary(), b.Primary(), c);
}

extern "C" void APIENTRY SBigShr(BigData *a, const BigData &b, UINT c) {
  Shr(a->Primary(), b.Primary(), c);
}

extern "C" void APIENTRY SBigSquare(BigData *a, const BigData &b) {
  Square(a->Primary(), b.Primary(), a->Stack());
}

extern "C" void APIENTRY SBigSub(BigData *a, const BigData &b, const BigData &c) {
  Sub(a->Primary(), b.Primary(), c.Primary());
}

extern "C" void APIENTRY SBigToBinaryArray(const BigData &num, TSGrowableArray<BYTE> *array, int append) {
  ToBinary(num.Output(), num.Primary());
  if (append) {
    array->Add(num.Output().Count(), num.Output().Ptr());
  } else {
    array->Set(num.Output().Count(), num.Output().Ptr());
  }
}

extern "C" void APIENTRY SBigToBinaryBuffer(const BigData &num, void *data, UINT maxBytes, UINT *bytes) {
  UINT count;
  ToBinary(num.Output(), num.Primary());
  count = min(num.Output().Count(), maxBytes);
  memcpy(data, num.Output().Ptr(), count);
  if (bytes) {
    *bytes = count;
  }
}

extern "C" void APIENTRY SBigToBinaryPtr(const BigData &num, const void **data, UINT *bytes) {
  ToBinary(num.Output(), num.Primary());
  *data = num.Output().Ptr();
  if (bytes) {
    *bytes = num.Output().Count();
  }
}

extern "C" void APIENTRY SBigToStrArray(const BigData &num, TSGrowableArray<char> *array, int append) {
  ToStr(num.Output(), num.Primary(), num.Stack());
  if (append) {
    array->Add(num.Output().Count(), (const char *)num.Output().Ptr());
  } else {
    array->Set(num.Output().Count(), (const char *)num.Output().Ptr());
  }
}

extern "C" void APIENTRY SBigToStrBuffer(const BigData &num, char *str, UINT chars) {
  ToStr(num.Output(), num.Primary(), num.Stack());
  SStrCopy(str, (const char *)num.Output().Ptr(), chars);
}

extern "C" void APIENTRY SBigToStrPtr(const BigData &num, const char **str) {
  ToStr(num.Output(), num.Primary(), num.Stack());
  *str = (const char *)num.Output().Ptr();
}

extern "C" void APIENTRY SBigToStreamArray(const BigData &num, TSGrowableArray<BYTE> *array, int append) {
  ToStream(num.Output(), num.Primary());
  if (append) {
    array->Add(num.Output().Count(), num.Output().Ptr());
  } else {
    array->Set(num.Output().Count(), num.Output().Ptr());
  }
}

extern "C" void APIENTRY SBigToStreamBuffer(const BigData &num, void *data, UINT maxBytes, UINT *bytes) {
  UINT count;
  ToStream(num.Output(), num.Primary());
  count = min(num.Output().Count(), maxBytes);
  memcpy(data, num.Output().Ptr(), count);
  if (bytes) {
    *bytes = count;
  }
}

extern "C" void APIENTRY SBigToStreamPtr(const BigData &num, const void **data, UINT *bytes) {
  ToStream(num.Output(), num.Primary());
  *data = num.Output().Ptr();
  *bytes = num.Output().Count();
}

extern "C" void APIENTRY SBigToUnsigned(const BigData &num, UINT *val) {
  ToUnsigned(val, num.Primary());
}

extern "C" void APIENTRY SBigXor(BigData *a, const BigData &b, const BigData &c) {
  Xor(a->Primary(), b.Primary(), c.Primary());
}
