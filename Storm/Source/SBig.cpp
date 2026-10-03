#include <storm.h>
#include <stpl.h>

#include <ctype.h>
#include <new>

class BigBuffer {
 private:
  mutable TSGrowableArray<UINT> m_data;
  UINT                          m_offset;

  void GrowToFit(UINT index) {
    m_data.GrowToFit(m_offset + index, 1);
  }

 public:
  BigBuffer() : m_offset(0) {
  }
  UINT &operator[](UINT index) {
    GrowToFit(index);
    return m_data[m_offset + index];
  }
  UINT operator[](UINT index) const {
    return IsUsed(index) ? m_data[m_offset + index] : 0;
  }
  void Clear() {
    m_data.SetCount(m_offset);
  }
  UINT Count() const {
    return m_data.Count() - m_offset;
  }
  BOOL IsUsed(UINT index) const {
    return m_offset + index < m_data.Count();
  }
  void SetCount(UINT count) {
    m_data.SetCount(m_offset + count);
  }
  void SetOffset(UINT offset) {
    m_offset = offset;
    if (m_offset) {
      GrowToFit(-1);
    }
  }
  void Trim() const {
    while (Count() && !*m_data.Top()) {
      m_data.SetCount(m_data.Count() - 1);
    }
  }
};

class BigStack {
 private:
  enum {
    SIZE = 16
  };

  BigBuffer m_buffer[SIZE];
  UINT      m_used;

 public:
  BigStack() : m_used(0) {
  }
  BigBuffer &Alloc(UINT *count) {
    if (m_used >= 0x10) {
      SErrDisplayError(0x85100000, __FILE__, __LINE__, "m_used < SIZE", NULL, 1);
    }
    if (count) {
      ++*count;
    }
    return m_buffer[m_used++];
  }
  void Free(UINT count) {
    if (count > m_used) {
      SErrDisplayError(0x85100000, __FILE__, __LINE__, "count <= m_used", NULL, 1);
    }
    m_used -= count;
  }
  BigBuffer &MakeDistinct(BigBuffer &orig, int required) {
    return required ? Alloc(NULL) : orig;
  }
  void UnmakeDistinct(BigBuffer &orig, BigBuffer &distinct) {
    if (&orig != &distinct) {
      orig = distinct;
      Free(1);
    }
  }
};

typedef TSGrowableArray_<BYTE, 'SBIG', __LINE__> SBigOutputArray;

class BigData {
 private:
  BigBuffer       m_primary;
  BigStack        m_stack;
  SBigOutputArray m_output;

 public:
  BigBuffer &Primary() {
    return m_primary;
  }
  const BigBuffer &Primary() const {
    return m_primary;
  }
  BigStack &Stack() const {
    return (BigStack &)m_stack;
  }
  SBigOutputArray &Output() const {
    return (SBigOutputArray &)m_output;
  }
};

static const UINT SMALL_PRIMES[171] = {3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,  41,  43,  47,  53,  59,   61,   67,   71,
                                       73,  79,  83,  89,  97,  101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151,  157,  163,  167,
                                       173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257,  263,  269,  271,
                                       277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373,  379,  383,  389,
                                       397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487,  491,  499,  503,
                                       509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 613,  617,  619,  631,
                                       641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739,  743,  751,  757,
                                       761, 769, 773, 787, 797, 809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863,  877,  881,  883,
                                       887, 907, 911, 919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997, 1009, 1013, 1019, 1021};

static const UINT FERMAT_WITNESS[1] = {2};
static const BYTE initSeed[8] = {0xB5, 0x3B, 0x12, 0x1F, 0xE5, 0x55, 0x9A, 0x15};
static const BYTE initMul[8] = {0x50, 0x46, 0x00, 0x00, 0x69, 0x90, 0x00, 0x00};

#define SMALL_BOUND ((DWORDLONG)1 << 32)

inline void TSSwap(BYTE &a, BYTE &b) {
  BYTE temp = a;
  a = b;
  b = temp;
}

static UINT      ExtractLowPart(DWORDLONG *b);
static UINT      ExtractLowPartLargeSum(DWORDLONG *carry, DWORDLONG add);
static UINT      ExtractLowPartSx(DWORDLONG *b);
static void      InsertLowPart(DWORDLONG *b, UINT c);
static DWORDLONG MakeLarge(UINT low, UINT high);
static void      Add(BigBuffer &a, const BigBuffer &b, UINT c);
static void      Add(BigBuffer &a, const BigBuffer &b, const BigBuffer &c);
static void      And(BigBuffer &a, const BigBuffer &b, const BigBuffer &c);
static int       Compare(const BigBuffer &a, UINT b);
static int       Compare(const BigBuffer &a, const BigBuffer &b);
static void      Div(BigBuffer &a, UINT *b, const BigBuffer &c, DWORDLONG d);
static void      Div(BigBuffer &a, BigBuffer &b, const BigBuffer &c, const BigBuffer &d, BigStack &stack);
static void      FindPrime(BigBuffer &a, UINT b, const BigBuffer &c, const BigBuffer &d, BigStack &stack);
static void      Gcd(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, BigStack &stack);
static UINT      HighBitPos(const BigBuffer &a);
static void      InvMod(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, BigStack &stack);
static BOOL      IsEven(const BigBuffer &a);
static BOOL      IsOdd(const BigBuffer &a);
static BOOL      IsOne(const BigBuffer &a);
static BOOL      IsPrime(const BigBuffer &a, BigStack &stack);
static BOOL      IsZero(const BigBuffer &a);
static UINT      LowBitPos(const BigBuffer &a);
static void      Mul(BigBuffer &a, const BigBuffer &b, DWORDLONG c);
static void      Mul(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, BigStack &stack);
static void      MulMod(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, const BigBuffer &d, BigStack &stack);
static void      Not(BigBuffer &a, const BigBuffer &b);
static void      Or(BigBuffer &a, const BigBuffer &b, const BigBuffer &c);
static void      Pow(BigBuffer &a, const BigBuffer &b, UINT c, BigStack &stack);
static void      PowMod(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, const BigBuffer &d, BigStack &stack);
static void      Rand(BigBuffer &a, const BigBuffer &b, BigBuffer &seed, BigStack &stack);
static void      Set2Exp(BigBuffer &a, UINT b);
static void      SetOne(BigBuffer &a);
static void      SetZero(BigBuffer &a);
static void      Shl(BigBuffer &a, const BigBuffer &b, UINT c);
static void      Shr(BigBuffer &a, const BigBuffer &b, UINT c);
static void      Square(BigBuffer &a, const BigBuffer &b, BigStack &stack);
static void      Sub(BigBuffer &a, const BigBuffer &b, UINT c);
static void      Sub(BigBuffer &a, const BigBuffer &b, const BigBuffer &c);
static void      Xor(BigBuffer &a, const BigBuffer &b, const BigBuffer &c);
static void      DecodeDataBytes(LPCVOID data, UINT maxBytes, UINT *offset, UINT *dataBytes);
static void      EncodeDataBytes(SBigOutputArray &output, UINT dataBytes);
static void      FromBinary(BigBuffer &a, LPCVOID data, UINT bytes);
static void      FromStr(BigBuffer &a, LPCSTR str);
static void      FromStream(BigBuffer &a, LPCVOID data, UINT maxBytes, UINT *bytes);
static void      FromUnsigned(BigBuffer &a, UINT val);
static void      ToBinary(SBigOutputArray &output, const BigBuffer &a);
static void      ToBinaryAppend(SBigOutputArray &output, const BigBuffer &a);
static void      ToStr(SBigOutputArray &output, const BigBuffer &a, BigStack &stack);
static void      ToStream(SBigOutputArray &output, const BigBuffer &a);
static void      ToUnsigned(UINT *val, const BigBuffer &a);

static UINT ExtractLowPart(DWORDLONG *b) {
  UINT result;
  result = (UINT)*b;
  *b >>= 32;
  return result;
}

static UINT ExtractLowPartLargeSum(DWORDLONG *carry, DWORDLONG add) {
  *carry += add;
  add = (*carry < add) ? (DWORDLONG)1 : (DWORDLONG)0;
  UINT result = ExtractLowPart(carry);
  *carry += add * SMALL_BOUND;
  return result;
}

static UINT ExtractLowPartSx(DWORDLONG *b) {
  UINT result;
  result = (UINT)*b;
  *b >>= 32;
  if (*b >= 0x80000000) {
    *b |= 0xFFFFFFFF00000000ui64;
  }
  return result;
}

static void InsertLowPart(DWORDLONG *b, UINT c) {
  *b = *b * SMALL_BOUND + c;
}

static DWORDLONG MakeLarge(UINT low, UINT high) {
  return ((DWORDLONG)high << 32) + low;
}

static void Add(BigBuffer &a, const BigBuffer &b, UINT c) {
  DWORDLONG carry = c;
  UINT      index;
  for (index = 0; carry || b.IsUsed(index); ++index) {
    carry += b[index];
    a[index] = ExtractLowPart(&carry);
  }
  a.SetCount(index);
}

static void Add(BigBuffer &a, const BigBuffer &b, const BigBuffer &c) {
  DWORDLONG carry = 0;
  UINT      index;
  for (index = 0; carry || b.IsUsed(index) || c.IsUsed(index); ++index) {
    carry += (DWORDLONG)b[index] + (DWORDLONG)c[index];
    a[index] = ExtractLowPart(&carry);
  }
  a.SetCount(index);
}

static void And(BigBuffer &a, const BigBuffer &b, const BigBuffer &c) {
  UINT index;
  for (index = 0; b.IsUsed(index) || c.IsUsed(index); ++index) {
    a[index] = b[index] & c[index];
  }
  a.SetCount(index);
}

static int Compare(const BigBuffer &a, UINT b) {
  a.Trim();
  if (a.Count() > 1) {
    return 1;
  }
  if (a[0]) {
    return (a[0] > b) ? 1 : -1;
  }
  return 0;
}

static int Compare(const BigBuffer &a, const BigBuffer &b) {
  int result = 0;
  for (UINT index = 0; a.IsUsed(index) || b.IsUsed(index); ++index) {
    if (a[index] != b[index]) {
      result = (a[index] > b[index]) ? 1 : -1;
    }
  }
  return result;
}

static void Div(BigBuffer &a, UINT *b, const BigBuffer &c, DWORDLONG d) {
  ASSERT(d <= SMALL_BOUND);
  a.SetCount(c.Count());
  DWORDLONG data = 0;
  UINT      index = c.Count();
  while (index--) {
    InsertLowPart(&data, c[index]);
    a[index] = (UINT)(data / d);
    data %= d;
  }
  a.Trim();
  *b = (UINT)data;
}

static void Div(BigBuffer &a, BigBuffer &b, const BigBuffer &c, const BigBuffer &d, BigStack &stack) {
  c.Trim();
  d.Trim();
  UINT cCount = c.Count();
  UINT dCount = d.Count();
  if (dCount > cCount) {
    b = c;
    SetZero(a);
    return;
  }
  if (dCount <= 1) {
    Div(a, &b[0], c, d[0]);
    b.SetCount(1);
    return;
  }

  UINT       allocCount = 0;
  BigBuffer &cc = stack.Alloc(&allocCount);
  BigBuffer &dd = stack.Alloc(&allocCount);
  BigBuffer &work = stack.Alloc(&allocCount);
  UINT       shift = 31 - (HighBitPos(d) & 31);
  Shl(cc, c, shift);
  Shl(dd, d, shift);
  UINT t = dd[dCount - 1] + 1;
  UINT index = cCount - dCount + 1;
  a.SetCount(index);
  while (index--) {
    a.SetOffset(index);
    cc.SetOffset(index);
    if (!t) {
      a[0] = cc[dCount];
    } else {
      a[0] = (UINT)(MakeLarge(cc[dCount - 1], cc[dCount]) / t);
    }
    if (a[0]) {
      Mul(work, dd, a[0]);
      Sub(cc, cc, work);
    }
    while (cc[dCount] || Compare(cc, dd) >= 0) {
      a[0]++;
      Sub(cc, cc, dd);
    }
  }
  Shr(b, cc, shift);
  b.Trim();
  stack.Free(allocCount);
}

static void FindPrime(BigBuffer &a, UINT b, const BigBuffer &c, const BigBuffer &d, BigStack &stack) {
  UINT       allocCount = 0;
  BigBuffer &t = stack.Alloc(&allocCount);
  BigBuffer &hi = stack.Alloc(&allocCount);
  BigBuffer &tt = stack.Alloc(&allocCount);

  Set2Exp(t, b - 2);
  Set2Exp(hi, b - 1);
  Add(hi, hi, t);
  Div(tt, a, c, t, stack);
  Add(a, a, hi);
  if (IsEven(a)) {
    Add(a, a, 1);
  }
  Add(hi, hi, t);
  Sub(hi, hi, 2);
  for (;;) {
    if (IsPrime(a, stack)) {
      if (IsOne(d)) {
        break;
      }
      Sub(tt, a, 1);
      Gcd(tt, tt, d, stack);
      if (IsOne(tt)) {
        break;
      }
    }
    Add(a, a, 2);
    if (Compare(a, hi) > 0) {
      Sub(a, a, t);
    }
  }
  stack.Free(allocCount);
}

static void Gcd(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, BigStack &stack) {
  UINT       allocCount = 0;
  BigBuffer &aa = stack.Alloc(&allocCount);
  BigBuffer &bb = stack.Alloc(&allocCount);
  if (Compare(b, c) > 0) {
    a = b;
    bb = c;
  } else {
    a = c;
    bb = b;
  }
  while (!IsZero(bb)) {
    Div(a, aa, a, bb, stack);
    a = bb;
    bb = aa;
  }
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
        if (mask & a[index]) {
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

static BOOL IsEven(const BigBuffer &a) {
  return !a.Count() || !(a[0] & 1);
}

static BOOL IsOdd(const BigBuffer &a) {
  a.Trim();
  return a.Count() >= 1 && (a[0] & 1);
}

static BOOL IsOne(const BigBuffer &a) {
  a.Trim();
  return a.Count() == 1 && a[0] == 1;
}

static BOOL IsPrime(const BigBuffer &a, BigStack &stack) {
  UINT       allocCount = 0;
  BigBuffer &a1 = stack.Alloc(&allocCount);
  BigBuffer &m = stack.Alloc(&allocCount);
  BigBuffer &z = stack.Alloc(&allocCount);
  BigBuffer &witness = stack.Alloc(&allocCount);
  int        result = TRUE;
  UINT       index;

  a.Trim();
  if (IsZero(a) || !(a[0] & 1)) {
    result = FALSE;
  }
  for (index = 0; result && index < 171; ++index) {
    if ((DWORDLONG)SMALL_PRIMES[index] < SMALL_BOUND) {
      UINT remainder;
      Div(z, &remainder, a, SMALL_PRIMES[index]);
      if (!remainder && !IsOne(z)) {
        result = FALSE;
      }
    } else {
      FromUnsigned(witness, SMALL_PRIMES[index]);
      Div(z, m, a, witness, stack);
      if (IsZero(m) && !IsOne(z)) {
        result = FALSE;
      }
    }
  }

  Sub(a1, a, 1);
  m = a1;
  UINT b = LowBitPos(m);
  if (b) {
    Shr(m, m, b);
  }
  for (index = 0; result && index < 6; ++index) {
    FromUnsigned(witness, SMALL_PRIMES[index]);
    if (Compare(witness, a) >= 0) {
      break;
    }
    PowMod(z, witness, m, a, stack);
    if (!IsOne(z)) {
      UINT j = 0;
      while (Compare(z, a1)) {
        if (++j == b) {
          result = FALSE;
          break;
        }
        MulMod(z, z, z, a, stack);
        if (IsOne(z)) {
          result = FALSE;
          break;
        }
      }
    }
  }

  for (index = 0; result && index < 1; ++index) {
    FromUnsigned(witness, FERMAT_WITNESS[index]);
    PowMod(z, witness, a, a, stack);
    if (Compare(z, witness)) {
      result = FALSE;
    }
  }
  stack.Free(allocCount);
  return result;
}

static BOOL IsZero(const BigBuffer &a) {
  a.Trim();
  return !a.Count();
}

static UINT LowBitPos(const BigBuffer &a) {
  for (UINT index = 0; index < a.Count(); ++index) {
    if (a[index]) {
      UINT mask = 1;
      for (UINT bit = 0; bit < 32; ++bit, mask <<= 1) {
        if (mask & a[index]) {
          return index * 32 + bit;
        }
      }
    }
  }
  return 0;
}

static void Mul(BigBuffer &a, const BigBuffer &b, DWORDLONG c) {
  ASSERT(c <= SMALL_BOUND);
  DWORDLONG carry = 0;
  UINT      index;
  for (index = 0; carry || b.IsUsed(index); ++index) {
    carry += b[index] * c;
    a[index] = ExtractLowPart(&carry);
  }
  a.SetCount(index);
}

static void Mul(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, BigStack &stack) {
  BigBuffer &aa = stack.MakeDistinct(a, &a == &b || &a == &c);
  aa.Clear();
  for (UINT bIndex = 0; b.IsUsed(bIndex); ++bIndex) {
    DWORDLONG carry = 0;
    for (UINT cIndex = 0; c.IsUsed(cIndex); ++cIndex) {
      carry += aa[cIndex + bIndex] + (DWORDLONG)c[cIndex] * b[bIndex];
      aa[cIndex + bIndex] = ExtractLowPart(&carry);
    }
    aa[cIndex + bIndex] = ExtractLowPart(&carry);
  }
  stack.UnmakeDistinct(a, aa);
}

static void MulMod(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, const BigBuffer &d, BigStack &stack) {
  UINT       allocCount = 0;
  BigBuffer &temp = stack.Alloc(&allocCount);
  Mul(temp, b, c, stack);
  Div(temp, a, temp, d, stack);
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
  while (bit & (bit - 1)) {
    bit &= bit - 1;
  }

  BigBuffer &aa = stack.MakeDistinct(a, &a == &b);
  aa = b;
  while (bit > 1) {
    Mul(aa, aa, aa, stack);
    bit >>= 1;
    if (c & bit) {
      Mul(aa, aa, b, stack);
    }
  }
  stack.UnmakeDistinct(a, aa);
}

static void PowMod(BigBuffer &a, const BigBuffer &b, const BigBuffer &c, const BigBuffer &d, BigStack &stack) {
  c.Trim();
  if (!c.Count()) {
    SetOne(a);
    return;
  }

  UINT       allocCount = 0;
  BigBuffer &temp = stack.Alloc(&allocCount);
  BigBuffer &b2 = stack.Alloc(&allocCount);
  BigBuffer &b3 = stack.Alloc(&allocCount);
  BigBuffer &aa = stack.MakeDistinct(a, &a == &b || &a == &c || &a == &d);
  MulMod(b2, b, b, d, stack);
  MulMod(b3, b2, b, d, stack);
  const BigBuffer *bPower[3] = {&b, &b2, &b3};
  SetOne(aa);
  UINT index = c.Count();
  while (index--) {
    UINT ci = c[index];
    UINT ciBits = 32;
    if (index + 1 == c.Count()) {
      while (!(ci & 0xC0000000)) {
        ci <<= 2;
        ciBits -= 2;
      }
    }
    for (UINT bit = 0; bit < ciBits; bit += 2) {
      Square(aa, aa, stack);
      Div(temp, aa, aa, d, stack);
      Square(aa, aa, stack);
      Div(temp, aa, aa, d, stack);
      if (ci >> 30) {
        MulMod(aa, aa, *bPower[(ci >> 30) - 1], d, stack);
      }
      ci <<= 2;
    }
  }
  stack.UnmakeDistinct(a, aa);
  stack.Free(allocCount);
}

static void Rand(BigBuffer &a, const BigBuffer &b, BigBuffer &seed, BigStack &stack) {
  UINT       allocCount = 0;
  BigBuffer &mul = stack.Alloc(&allocCount);
  BigBuffer &tt = stack.Alloc(&allocCount);
  BigBuffer &aa = stack.MakeDistinct(a, &a == &b || &a == &seed);
  BigBuffer &ss = stack.MakeDistinct(seed, &seed == &b);

  ss.Trim();
  int reinit = ss.Count() < 2;
  ss.SetCount(2);
  if (reinit) {
    for (UINT loop = 0; loop < 8; ++loop) {
      ((BYTE *)&ss[0])[loop] = initSeed[loop];
    }
  }
  mul.SetCount(2);
  {
    for (UINT loop = 0; loop < 8; ++loop) {
      ((BYTE *)&mul[0])[loop] = initMul[loop];
    }
  }
  b.Trim();
  aa.Clear();
  {
    for (UINT loop = 0; loop < (b.Count() * 32 + 31) / 32; ++loop) {
      Mul(ss, ss, mul, stack);
      Shr(tt, ss, 16);
      Add(ss, tt, ss);
      ss.SetCount(2);
      Shl(aa, aa, 32);
      Add(aa, aa, ss);
      Shr(tt, ss, 32);
      Add(aa, aa, tt);
    }
  }
  Div(tt, aa, aa, b, stack);
  stack.UnmakeDistinct(seed, ss);
  stack.UnmakeDistinct(a, aa);
  stack.Free(allocCount);
}

static void Set2Exp(BigBuffer &a, UINT b) {
  UINT index;
  for (index = 0; index < b / 32; ++index) {
    a[index] = 0;
  }
  a[index++] = 1 << (b % 32);
  a.SetCount(index);
}

static void SetOne(BigBuffer &a) {
  a.SetCount(1);
  a[0] = 1;
}

static void SetZero(BigBuffer &a) {
  a.Clear();
}

static void Shl(BigBuffer &a, const BigBuffer &b, UINT c) {
  UINT cOffset = c / 32;
  UINT cBits = c % 32;
  UINT aCount = b.Count() + cOffset + 1;
  UINT index = aCount;
  while (index--) {
    UINT data = 0;
    if (index >= cOffset) {
      data = b[index - cOffset] << cBits;
    }
    if (index > cOffset && cBits) {
      data += b[index - cOffset - 1] >> (32 - cBits);
    }
    a[index] = data;
  }
  a.SetCount(aCount);
  a.Trim();
}

static void Shr(BigBuffer &a, const BigBuffer &b, UINT c) {
  UINT cOffset = c / 32;
  UINT cBits = c % 32;
  UINT index;
  for (index = 0; b.IsUsed(index + cOffset); ++index) {
    UINT data = b[index + cOffset] >> cBits;
    if (cBits) {
      data += b[index + cOffset + 1] << (32 - cBits);
    }
    a[index] = data;
  }
  a.SetCount(index);
}

static void Square(BigBuffer &a, const BigBuffer &b, BigStack &stack) {
  DWORDLONG  add;
  DWORDLONG  mul;
  DWORDLONG  carry;
  BigBuffer &aa = stack.MakeDistinct(a, &a == &b);

  aa.Clear();
  for (UINT bIndex = 0; b.IsUsed(bIndex); ++bIndex) {
    carry = 0;
    for (UINT cIndex = 0; cIndex <= bIndex; ++cIndex) {
      mul = (DWORDLONG)b[cIndex] * b[bIndex];
      add = (DWORDLONG)aa[bIndex + cIndex] + mul;
      if (cIndex < bIndex) {
        carry += mul;
      }
      aa[bIndex + cIndex] = ExtractLowPartLargeSum(&carry, add);
    }
    aa[cIndex + bIndex] = ExtractLowPart(&carry);
  }
  stack.UnmakeDistinct(a, aa);
}

static void Sub(BigBuffer &a, const BigBuffer &b, UINT c) {
  DWORDLONG borrow = ~(DWORDLONG)c + 1;
  UINT      index;
  for (index = 0; b.IsUsed(index); ++index) {
    borrow += b[index];
    a[index] = ExtractLowPartSx(&borrow);
  }
  a.SetCount(index);
  ASSERT(!borrow);
}

static void Sub(BigBuffer &a, const BigBuffer &b, const BigBuffer &c) {
  DWORDLONG borrow = 0;
  UINT      index;
  for (index = 0; b.IsUsed(index) || c.IsUsed(index); ++index) {
    borrow += (DWORDLONG)b[index] - (DWORDLONG)c[index];
    a[index] = ExtractLowPartSx(&borrow);
  }
  a.SetCount(index);
  ASSERT(!borrow);
}

static void Xor(BigBuffer &a, const BigBuffer &b, const BigBuffer &c) {
  UINT index = 0;

  while (b.IsUsed(index) || c.IsUsed(index)) {
    a[index] = b[index] ^ c[index];
    ++index;
  }
  a.SetCount(index);
}

static void DecodeDataBytes(LPCVOID data, UINT maxBytes, UINT *offset, UINT *dataBytes) {
  *dataBytes = 0;
  *offset = 0;
  while (*offset < maxBytes) {
    UINT value = ((const BYTE *)data)[(*offset)++];
    if (value == 0xFF) {
      break;
    }
    *dataBytes = *dataBytes * 0xFF + value;
  }
  *dataBytes = min(*dataBytes, maxBytes - *offset);
}

static void EncodeDataBytes(SBigOutputArray &output, UINT dataBytes) {
  while (dataBytes) {
    *output.New() = (BYTE)(dataBytes % 0xFF);
    dataBytes /= 0xFF;
  }
  *output.New() = 0xFF;
}

static void FromBinary(BigBuffer &a, LPCVOID data, UINT bytes) {
  a.Clear();
  for (UINT index = 0; index < bytes; ++index) {
    UINT word = index / 4;
    UINT byte = index % 4;
    UINT value = ((const BYTE *)data)[index];
    a[word] = (byte ? a[word] : 0) + (value << (byte * 8));
  }
}

static void FromStr(BigBuffer &a, LPCSTR str) {
  a.Clear();
  while (*str) {
    Mul(a, a, (DWORDLONG)10);
    Add(a, a, (UINT)(*str++ - '0'));
  }
}

static void FromStream(BigBuffer &a, LPCVOID data, UINT maxBytes, UINT *bytes) {
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
  *val = 0;
  *val = a[0];
}

extern "C" void APIENTRY SBigAdd(BigData *a, const BigData &b, const BigData &c) {
  Add(a->Primary(), b.Primary(), c.Primary());
}

extern "C" void APIENTRY SBigAnd(BigData *a, const BigData &b, const BigData &c) {
  And(a->Primary(), b.Primary(), c.Primary());
}

extern "C" void APIENTRY SBigBitLen(BigData *a, UINT *bits) {
  BigBuffer &aa = a->Primary();
  aa.Trim();
  UINT index = aa.Count() - 1;
  UINT data = aa[index];
  for (UINT bit = 31; bit; --bit) {
    if ((1 << bit) & data) {
      break;
    }
  }
  *bits = index * 32 + bit + 1;
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

extern "C" void APIENTRY SBigFromBinary(BigData *num, LPCVOID data, UINT bytes) {
  FromBinary(num->Primary(), data, bytes);
}

extern "C" void APIENTRY SBigFromStr(BigData *num, LPCSTR str) {
  FromStr(num->Primary(), str);
}

extern "C" void APIENTRY SBigFromStream(BigData *num, LPCVOID data, UINT maxBytes, UINT *bytes) {
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

extern "C" void APIENTRY SBigToBinaryBuffer(const BigData &num, LPVOID data, UINT maxBytes, UINT *bytes) {
  UINT count;
  ToBinary(num.Output(), num.Primary());
  count = min(num.Output().Count(), maxBytes);
  memcpy(data, num.Output().Ptr(), count);
  if (bytes) {
    *bytes = count;
  }
}

extern "C" void APIENTRY SBigToBinaryPtr(const BigData &num, LPCVOID *data, UINT *bytes) {
  ToBinary(num.Output(), num.Primary());
  *data = num.Output().Ptr();
  if (bytes) {
    *bytes = num.Output().Count();
  }
}

extern "C" void APIENTRY SBigToStrArray(const BigData &num, TSGrowableArray<char> *array, int append) {
  ToStr(num.Output(), num.Primary(), num.Stack());
  if (append) {
    array->Add(num.Output().Count(), (LPCSTR)num.Output().Ptr());
  } else {
    array->Set(num.Output().Count(), (LPCSTR)num.Output().Ptr());
  }
}

extern "C" void APIENTRY SBigToStrBuffer(const BigData &num, char *str, UINT chars) {
  ToStr(num.Output(), num.Primary(), num.Stack());
  SStrCopy(str, (LPCSTR)num.Output().Ptr(), chars);
}

extern "C" void APIENTRY SBigToStrPtr(const BigData &num, LPCSTR *str) {
  ToStr(num.Output(), num.Primary(), num.Stack());
  *str = (LPCSTR)num.Output().Ptr();
}

extern "C" void APIENTRY SBigToStreamArray(const BigData &num, TSGrowableArray<BYTE> *array, int append) {
  ToStream(num.Output(), num.Primary());
  if (append) {
    array->Add(num.Output().Count(), num.Output().Ptr());
  } else {
    array->Set(num.Output().Count(), num.Output().Ptr());
  }
}

extern "C" void APIENTRY SBigToStreamBuffer(const BigData &num, LPVOID data, UINT maxBytes, UINT *bytes) {
  UINT count;
  ToStream(num.Output(), num.Primary());
  count = min(num.Output().Count(), maxBytes);
  memcpy(data, num.Output().Ptr(), count);
  if (bytes) {
    *bytes = count;
  }
}

extern "C" void APIENTRY SBigToStreamPtr(const BigData &num, LPCVOID *data, UINT *bytes) {
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
