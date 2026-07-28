#include <storm.h>
#include <stpl.h>

#include "../Zlib/zlib.h"

#define SOURCESYMBOLS   256
#define EOS             256
#define ESCAPE          257
#define SYMBOLS         258
#define ALGORITHMS      5
#define HUFFMAN_SYMBOLS SYMBOLS
#define HUFFMAN_EOF     EOS
#define HUFFMAN_ESCAPE  ESCAPE
#ifndef SCOMP_HINT_NONE
#define SCOMP_HINT_NONE 0
#endif
#ifndef SCOMP_HINT_TEXT
#define SCOMP_HINT_TEXT 2
#endif
#ifndef SCOMP_HINTS
#define SCOMP_HINTS 9
#endif

static const int s_ima_adjtable[32] = {
    -1, 0, -1, 4, -1, 2, -1, 6, -1, 1, -1, 5, -1, 3, -1, 7, -1, 1, -1, 5, -1, 3, -1, 7, -1, 2, -1, 4, -1, 6, -1, 8,
};

static const DWORD s_ima_steptable[89] = {
    7,    8,    9,    10,   11,    12,    13,    14,    16,    17,    19,    21,    23,    25,    28,    31,    34,    37,
    41,   45,   50,   55,   60,    66,    73,    80,    88,    97,    107,   118,   130,   143,   157,   173,   190,   209,
    230,  253,  279,  307,  337,   371,   408,   449,   494,   544,   598,   658,   724,   796,   876,   963,   1060,  1166,
    1282, 1411, 1552, 1707, 1878,  2066,  2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
};

/* WowClient.exe VA 0x0080EDA8.  The older Storm source supplies the original
 * table name and rows 0..5; WoW adds its three stereo ADPCM hint rows. */
static const BYTE s_probability[SCOMP_HINTS][HUFFMAN_SYMBOLS] = {
    /* SCOMP_HINT_NONE */
    {10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},

    /* SCOMP_HINT_BINARY */
    {84, 22, 22, 13, 12, 8, 6, 5, 6, 5, 6, 3,  4, 4, 3, 5, 14, 11, 20, 19, 19, 9, 11, 6, 5, 4, 3, 2, 3, 2, 2, 2, 13, 7, 9, 6, 6, 4, 3, 2, 4, 3, 3,
     3,  3,  3,  2,  2,  9, 6, 4, 4, 4, 4, 3,  2, 3, 2, 2, 2,  2,  3,  2,  4,  8, 3,  4, 7, 9, 5, 3, 3, 3, 3, 2, 2,  2, 3, 2, 2, 3, 2, 2, 2, 2, 2,
     2,  2,  2,  1,  1,  1, 2, 1, 2, 2, 6, 10, 8, 8, 6, 7, 4,  3,  4,  4,  2,  2, 4,  2, 3, 3, 4, 3, 7, 7, 9, 6, 4,  3, 3, 2, 1, 2, 2, 2, 2, 2, 10,
     2,  2,  3,  2,  2,  1, 1, 2, 2, 2, 6, 3,  5, 2, 3, 2, 1,  1,  1,  1,  1,  1, 1,  1, 1, 1, 2, 3, 1, 1, 1, 2, 1,  1, 1, 1, 1, 1, 2, 4, 4, 4, 7,
     9,  8,  12, 2,  1,  1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 2,  1,  1,  3,  4,  1, 2,  4, 5, 1, 1, 1, 1, 1, 1, 1, 2,  1, 1, 1, 4, 1, 1, 1, 1, 1, 2,
     1,  1,  1,  1,  1,  1, 1, 1, 1, 2, 1, 1,  1, 1, 1, 1, 1,  3,  1,  1,  1,  1, 1,  1, 1, 2, 1, 1, 1, 1, 1, 1, 2,  2, 1, 1, 2, 2, 2, 6, 75},

    /* SCOMP_HINT_TEXT */
    {0,   0,  0, 0,  0,  0,  0,  0,  0,  3,  39, 0, 0,  35, 0,  0,  0,  0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     255, 1,  1, 1,  1,  1,  1,  1,  2,  2,  1,  1, 6,  14, 16, 4,  6,  8, 5,  4,  4,  3,  3, 2, 2, 3, 3, 1, 1, 2, 1, 1,
     1,   4,  2, 4,  2,  2,  2,  1,  1,  4,  1,  1, 2,  3,  3,  2,  3,  1, 3,  6,  4,  1,  1, 1, 1, 1, 1, 2, 1, 2, 1, 1,
     1,   41, 7, 22, 18, 64, 10, 10, 17, 37, 1,  3, 23, 16, 38, 42, 16, 1, 35, 35, 47, 16, 6, 7, 2, 9, 1, 1, 1, 1, 1, 0},

    /* SCOMP_HINT_EXECUTABLE */
    {255, 11, 7,  5, 11, 2, 2, 2, 6, 2, 2,  1, 4, 2, 1, 3,  9, 1, 1, 1, 3, 4,  1, 1, 2, 1, 1, 1, 2, 1, 1,  1, 5, 1, 1, 1, 13, 1, 1, 1, 1, 1, 1,
     1,   1,  1,  1, 1,  2, 1, 1, 3, 1, 1,  1, 1, 1, 1, 1,  2, 1, 1, 1, 1, 10, 4, 2, 1, 6, 3, 2, 1, 1, 1,  1, 1, 3, 1, 1, 1,  5, 2, 3, 4, 3, 3,
     3,   2,  1,  1, 1,  2, 1, 2, 3, 3, 1,  3, 1, 1, 2, 5,  1, 1, 4, 3, 5, 1,  3, 1, 3, 3, 2, 1, 4, 3, 10, 6, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 2,
     2,   1,  10, 2, 5,  1, 1, 2, 7, 2, 23, 1, 5, 1, 1, 14, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1,
     1,   1,  1,  1, 1,  1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1,  1, 1, 1, 1, 6, 2,  1, 4, 5, 1, 1, 2, 1, 1, 1,  1, 2, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1,
     1,   1,  1,  1, 1,  1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1,  1, 7, 1, 1, 2, 1,  1, 1, 1, 2, 1, 1, 1, 1, 1,  1, 1, 2, 1, 1, 1,  1, 1, 1, 17},

    /* SCOMP_HINT_ADPCM4 */
    {255, 251, 152, 154, 132, 133, 99, 100, 62, 62, 34, 34, 19, 19, 24, 23},

    /* SCOMP_HINT_ADPCM6 */
    {255, 241, 157, 158, 154, 155, 154, 151, 147, 147, 140, 142, 134, 136, 128, 130, 124, 124, 114, 115, 105, 107,
     95,  96,  85,  86,  74,  75,  64,  65,  55,  55,  47,  47,  39,  39,  33,  33,  27,  28,  23,  23,  19,  19,
     16,  16,  13,  13,  11,  11,  9,   9,   8,   8,   7,   7,   6,   5,   5,   4,   4,   4,   25,  24},

    /* WoW ADPCM, 4-bit */
    {195, 203, 245, 65, 255, 123, 247, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0,   0,  0,   0,   0,   0,  0, 0,
     0,   0,   0,   0,  0,   0,   0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 191, 204, 242, 64, 253, 124, 247, 34, 0, 0,
     0,   0,   0,   0,  0,   0,   0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0,   0,  0,   0,   0,   0,  0, 0,
     0,   0,   0,   0,  0,   0,   0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 122, 70, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0,   0,  0,   0,   0,   0,  0, 0,
     0,   0,   0,   0,  0,   0,   0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0,   0,  0,   0,   0,   0,  0, 0,
     0,   0,   0,   0,  0,   0,   0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0,   0,  0,   0,   0,   0,  0, 0,
     0,   0,   0,   0,  0,   0,   0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0,   0,  0,   0,   0},

    /* WoW ADPCM, 5-bit */
    {195, 217, 239, 61, 249, 124, 233, 30, 253, 171, 241, 44, 252, 91, 254, 23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0,   0,   0,   0,  0,   0,   0,   0,  0,   0,   0,   0,  0,   0,  0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     189, 217, 236, 61, 245, 125, 232, 29, 251, 174, 240, 44, 251, 92, 255, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0,   0,   0,   0,  0,   0,   0,   0,  0,   0,   0,   0,  0,   0,  0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     112, 108, 0,   0,  0,   0,   0,   0,  0,   0,   0,   0,  0,   0,  0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0,   0,   0,   0,  0,   0,   0,   0,  0,   0,   0,   0,  0,   0,  0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0,   0,   0,   0,  0,   0,   0,   0,  0,   0,   0,   0,  0,   0,  0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0,   0,   0,   0,  0,   0,   0,   0,  0,   0,   0,   0,  0,   0,  0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

    /* WoW ADPCM, 6-bit */
    {186, 197, 218, 51,  227, 109, 216, 24,  229, 148, 218, 35,  223, 74,  209, 16,  238, 175, 228, 44, 234, 90, 222, 21,  244, 135, 233, 33, 246,
     67,  252, 18,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,  0,   0,   0,   0,   0,   0,  0,
     0,   0,   0,   0,   0,   0,   176, 199, 216, 51,  227, 107, 214, 24,  231, 149, 216, 35,  219, 73, 208, 17, 233, 178, 226, 43,  232, 92, 221,
     21,  241, 135, 231, 32,  247, 68,  255, 19,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,  0,   0,   0,   0,   0,   0,  0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   95,  158, 0,   0,   0,   0,   0,   0,  0,   0,  0,   0,   0,   0,   0,   0,  0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,  0,   0,   0,   0,   0,   0,  0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,  0,   0,   0,   0,   0,   0,  0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,  0,   0,   0,   0,   0,   0,  0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,  0,   0}
};

static BYTE     *s_DecompressBuffer;
static SCritSect s_decompCrit;
static DWORD     s_DecompressBufferSize;
static DWORD     s_DecompressBufferTID;

NODEDECL(HUFFNODE) {
  int       symbol;
  DWORD     weight;
  HUFFNODE *parent;
  HUFFNODE *child;
};
namespace {

  struct _IMAHEADER {
    BYTE reserved;
    BYTE roundshift;
  };

  class CBitInput {
   public:
    CBitInput(const void *source);

    inline DWORD InputBits(DWORD count, DWORD mask);
    inline DWORD PeekBits(DWORD count, DWORD mask);
    inline void  RemoveBits(DWORD count);
    inline DWORD InputBit();

   private:
    const DWORD *m_currsource;
    DWORD        m_rack;
    DWORD        m_rackbits;
  };

  class CBitOutput {
   public:
    CBitOutput(void *dest, DWORD destsize);

    DWORD       GetTotalBytes();
    void        Pad();
    inline void OutputBits(DWORD value, DWORD count);

   private:
    BYTE *m_basedest;
    DWORD m_bytesleft;
    BYTE *m_currdest;
    DWORD m_rack;
    DWORD m_rackbits;
  };

}  // namespace

class CHuffman {
 public:
  CHuffman();

 protected:
  void             AddSymbol(int symbol);
  inline HUFFNODE *AllocNode(DWORD linktype);
  void             BuildTree(BYTE mode);
  inline void      IncrementWeight(HUFFNODE *node);

  int                                    m_adaptive;
  DWORD                                  m_changesequence;
  HUFFNODE                               m_nodebuffer[0x203];
  LISTDECL(HUFFNODE, m_nodefreelist);
  LISTDECL(HUFFNODE, m_nodelist);
  DWORD                                  m_nodesused;
  HUFFNODE                              *m_symbol[HUFFMAN_SYMBOLS];
};

class CHuffmanEncoder : public CHuffman {
 public:
  DWORD Compress(CBitOutput *output, const void *source, DWORD sourcebytes, BYTE mode);

 private:
  inline void EncodeSymbol(CBitOutput *output, int symbol);
};

struct _CACHEREC {
  DWORD sequence;
  DWORD bits;
  union {
    int       symbol;
    HUFFNODE *nodeptr;
  };
};

class CHuffmanDecoder : public CHuffman {
 public:
  CHuffmanDecoder();

  DWORD Decompress(void *dest, DWORD destbytes, CBitInput *input);

 private:
  inline int DecodeSymbol(CBitInput *input);

  _CACHEREC m_cache[0x80];
};

struct ZLIB_BUFFER {
  BYTE *current;
  DWORD size;
  BYTE *base;
};

struct ZlibUncompressAllocBuffer {
  ZLIB_BUFFER arena;
  BYTE        workspace[47000];
};

struct ZlibCompressAllocBuffer {
  ZLIB_BUFFER arena;
  BYTE        workspace[300000];
};

CBitInput::CBitInput(const void *source) {
  m_currsource = static_cast<const DWORD *>(source);
  m_rack = *m_currsource;
  ++m_currsource;
  m_rackbits = 32;
}

inline DWORD CBitInput::InputBit() {
  DWORD value;

  value = m_rack & 1;
  m_rack >>= 1;
  --m_rackbits;
  if (!m_rackbits) {
    m_rack = *m_currsource;
    ++m_currsource;
    m_rackbits = 32;
  }
  return value;
}

inline DWORD CBitInput::InputBits(DWORD count, DWORD mask) {
  DWORD value;

  value = PeekBits(count, mask);
  RemoveBits(count);
  return value;
}

inline DWORD CBitInput::PeekBits(DWORD count, DWORD mask) {
  if (m_rackbits <= count) {
    m_rack |= static_cast<DWORD>(*reinterpret_cast<const WORD *>(m_currsource)) << m_rackbits;
    m_currsource = reinterpret_cast<const DWORD *>(reinterpret_cast<const BYTE *>(m_currsource) + sizeof(WORD));
    m_rackbits += 16;
  }
  return m_rack & mask;
}

inline void CBitInput::RemoveBits(DWORD count) {
  m_rack >>= count;
  m_rackbits -= count;
}

CBitOutput::CBitOutput(void *dest, DWORD destsize) {
  m_basedest = (BYTE *)dest;
  m_bytesleft = destsize;
  m_currdest = (BYTE *)dest;
  m_rack = 0;
  m_rackbits = 0;
}

DWORD CBitOutput::GetTotalBytes() {
  return (DWORD)(m_currdest - m_basedest);
}

inline void CBitOutput::OutputBits(DWORD value, DWORD count) {
  m_rack |= value << m_rackbits;
  m_rackbits += count;

  while (m_rackbits >= 8) {
    if (m_bytesleft) {
      *m_currdest++ = (BYTE)m_rack;
      --m_bytesleft;
    }
    m_rack >>= 8;
    m_rackbits -= 8;
  }
}

void CBitOutput::Pad() {
  while (m_rackbits) {
    DWORD bits;

    if (m_bytesleft) {
      *m_currdest++ = (BYTE)m_rack;
      --m_bytesleft;
    }
    m_rack >>= 8;
    bits = m_rackbits > 8 ? 8 : m_rackbits;
    m_rackbits -= bits;
  }
}

inline void TSSwap(HUFFNODE *&a, HUFFNODE *&b) {
  HUFFNODE *temp;

  temp = a;
  a = b;
  b = temp;
}

CHuffman::CHuffman() {
  m_changesequence = 1;
  m_nodesused = 0;
}

void CHuffman::AddSymbol(int symbol) {
  HUFFNODE *parent;
  HUFFNODE *oldNode;
  HUFFNODE *newNode;

  parent = m_nodelist.Head();
  oldNode = AllocNode(LIST_HEAD);
  oldNode->symbol = parent->symbol;
  oldNode->weight = parent->weight;
  oldNode->parent = parent;
  m_symbol[oldNode->symbol] = oldNode;

  newNode = AllocNode(LIST_HEAD);
  newNode->symbol = symbol;
  newNode->weight = 0;
  newNode->parent = parent;
  m_symbol[symbol] = newNode;
  parent->child = newNode;

  IncrementWeight(newNode);
}

inline HUFFNODE *CHuffman::AllocNode(DWORD linktype) {
  HUFFNODE *node;

  node = m_nodefreelist.Head();
  if (!node) {
    node = &m_nodebuffer[m_nodesused++];
  }

  m_nodelist.LinkNode(node, linktype, NULL);
  node->parent = NULL;
  node->child = NULL;
  return node;
}

void CHuffman::BuildTree(BYTE hint) {
  HUFFNODE *node;
  DWORD     maxweight;
  int       symbol;

  node = m_nodelist.Head();
  while (node) {
    m_nodefreelist.LinkNode(node, LIST_HEAD, NULL);
    node = m_nodelist.Head();
  }

  memset(m_symbol, 0, sizeof(m_symbol));
  maxweight = 0;

  for (symbol = 0; symbol < SOURCESYMBOLS; ++symbol) {
    if (!s_probability[hint][symbol]) {
      continue;
    }

    node = AllocNode(LIST_TAIL);
    m_symbol[symbol] = node;
    node->symbol = symbol;
    node->weight = s_probability[hint][symbol];

    if (node->weight >= maxweight) {
      maxweight = node->weight;
    } else {
      HUFFNODE *scan;

      scan = m_nodelist.Head();
      while (scan && scan->weight < node->weight) {
        scan = scan->Next();
      }
      m_nodelist.LinkNode(node, LIST_LINK_BEFORE, scan);
    }
  }

  for (symbol = HUFFMAN_EOF; symbol < HUFFMAN_SYMBOLS; ++symbol) {
    node = AllocNode(LIST_HEAD);
    m_symbol[symbol] = node;
    node->symbol = symbol;
    node->weight = 1;
  }

  node = m_nodelist.Head();
  while (node) {
    HUFFNODE *right;
    HUFFNODE *parent;
    HUFFNODE *afterRight;

    right = node->Next();
    if (!right) {
      break;
    }

    parent = AllocNode(LIST_TAIL);
    parent->weight = node->weight + right->weight;
    parent->child = node;
    node->parent = parent;
    right->parent = parent;

    if (parent->weight >= maxweight) {
      maxweight = parent->weight;
    } else {
      afterRight = right->Next();
      while (afterRight && afterRight->weight < parent->weight) {
        afterRight = afterRight->Next();
      }
      m_nodelist.LinkNode(parent, LIST_LINK_BEFORE, afterRight);
    }

    node = right->Next();
  }

  m_changesequence = 1;
}

inline void CHuffman::IncrementWeight(HUFFNODE *node) {
  while (node) {
    HUFFNODE *swapNode;
    HUFFNODE *scan;

    ++node->weight;
    swapNode = node;
    scan = node->Next();
    while (scan && scan->weight < node->weight) {
      swapNode = scan;
      scan = scan->Next();
    }

    if (swapNode != node) {
      HUFFNODE *nodeParent;
      HUFFNODE *swapParentChild;

      m_nodelist.LinkNode(swapNode, LIST_LINK_BEFORE, node);
      m_nodelist.LinkNode(node, LIST_LINK_BEFORE, scan);

      swapParentChild = swapNode->parent->child;
      nodeParent = node->parent;

      if (nodeParent->child == node) {
        nodeParent->child = swapNode;
      }
      if (swapParentChild == swapNode) {
        swapNode->parent->child = node;
      }

      TSSwap(node->parent, swapNode->parent);
      ++m_changesequence;
    }

    node = node->parent;
  }
}

CHuffmanDecoder::CHuffmanDecoder() {
  DWORD i;

  for (i = 0; i < 0x80; ++i) {
    m_cache[i].sequence = 0;
  }
}

inline int CHuffmanDecoder::DecodeSymbol(CBitInput *input) {
  DWORD      cachebits;
  _CACHEREC *cacheslot;
  HUFFNODE  *cachenode;
  HUFFNODE  *currnode;
  DWORD      bits;
  DWORD      index;
  DWORD      stride;
  BOOL       cachevalid;

  cachebits = input->PeekBits(7, 0x7F);
  cacheslot = &m_cache[cachebits];
  cachevalid = cacheslot->sequence == m_changesequence;

  if (cachevalid) {
    if (cacheslot->bits > 7) {
      input->RemoveBits(7);
      currnode = cacheslot->nodeptr;
    } else {
      input->RemoveBits(cacheslot->bits);
      return cacheslot->symbol;
    }
  } else {
    currnode = m_nodelist.Tail();
  }

  bits = 0;
  cachenode = NULL;
  do {
    currnode = currnode->child;
    if (input->InputBit()) {
      currnode = currnode->RawNext();
    }
    if (++bits == 7) {
      cachenode = currnode;
    }
  } while (currnode->child);

  if (!cachevalid) {
    if (bits > 7) {
      cacheslot->bits = bits;
      cacheslot->nodeptr = cachenode;
      cacheslot->sequence = m_changesequence;
    } else {
      index = cachebits & (0xFFFFFFFFUL >> (32 - bits));
      stride = 1UL << bits;
      do {
        cacheslot = &m_cache[index];
        cacheslot->sequence = m_changesequence;
        cacheslot->bits = bits;
        cacheslot->symbol = currnode->symbol;
        index += stride;
      } while (index <= 0x7F);
    }
  }

  return currnode->symbol;
}

DWORD CHuffmanDecoder::Decompress(void *dest, DWORD destsize, CBitInput *input) {
  BYTE  hint;
  BYTE *currdest;

  if (!destsize) {
    return 0;
  }

  hint = (BYTE)input->InputBits(8, 0xFF);
  BuildTree(hint);
  m_adaptive = hint == SCOMP_HINT_NONE;
  currdest = (BYTE *)dest;

  while (1) {
    int symbol;

    symbol = DecodeSymbol(input);
    if (symbol == HUFFMAN_ESCAPE) {
      symbol = (int)input->InputBits(8, 0xFF);
      AddSymbol(symbol);
      if (!m_adaptive) {
        IncrementWeight(m_symbol[symbol]);
      }
    }
    if (symbol == HUFFMAN_EOF) {
      break;
    }
    *currdest++ = (BYTE)symbol;
    if (!--destsize) {
      break;
    }
    if (m_adaptive) {
      IncrementWeight(m_symbol[symbol]);
    }
  }

  return (DWORD)(currdest - (BYTE *)dest);
}

DWORD CHuffmanEncoder::Compress(CBitOutput *output, const void *source, DWORD sourcesize, BYTE hint) {
  const BYTE *currsource;

  BuildTree(hint);
  m_adaptive = hint == SCOMP_HINT_NONE;
  output->OutputBits(hint, 8);
  currsource = (const BYTE *)source;

  while (sourcesize) {
    int       symbol;
    HUFFNODE *node;

    symbol = *currsource++;
    node = m_symbol[symbol];
    if (!node) {
      EncodeSymbol(output, HUFFMAN_ESCAPE);
      output->OutputBits((DWORD)symbol, 8);
      AddSymbol(symbol);
      if (!m_adaptive) {
        IncrementWeight(m_symbol[symbol]);
      }
    } else {
      EncodeSymbol(output, symbol);
    }
    if (m_adaptive) {
      IncrementWeight(m_symbol[symbol]);
    }
    --sourcesize;
  }

  EncodeSymbol(output, HUFFMAN_EOF);
  output->Pad();
  return output->GetTotalBytes();
}

inline void CHuffmanEncoder::EncodeSymbol(CBitOutput *output, int symbol) {
  HUFFNODE *node;
  DWORD     bits;
  DWORD     bitcount;

  node = m_symbol[symbol];
  bits = 0;
  bitcount = 0;
  while (node->parent) {
    bits = (bits << 1) | (node->parent->child != node);
    ++bitcount;
    node = node->parent;
  }
  output->OutputBits(bits, bitcount);
}
namespace {

  static void ImaAdpcmCheckOptimization(DWORD optimization, BYTE *bitspersample, DWORD *hint) {
    switch (optimization) {
      case 1:
      case 2:
        *bitspersample = 4;
        *hint = 6;
        break;
      case 3:
        *bitspersample = 6;
        *hint = 8;
        break;
      default:
        *bitspersample = 5;
        *hint = 7;
        break;
    }
  }

  static DWORD ImaAdpcmCompress(BYTE *dest, DWORD destsize, const SHORT *source, DWORD sourcesize, DWORD channels, BYTE bitspersample) {
    int        val[2];
    int        last[2];
    int        index[2];
    DWORD      samples;
    DWORD      round;
    DWORD      loop;
    DWORD      highbit;
    DWORD      vpdiff;
    DWORD      delta;
    BYTE      *basedest;
    DWORD      escapebudget;
    DWORD      channel;
    _IMAHEADER header;
    DWORD      bit;

    if (destsize < 2) {
      return 2;
    }

    basedest = dest;
    header.roundshift = bitspersample - 1;
    header.reserved = 0;
    *(WORD *)dest = *(WORD *)&header;
    dest += sizeof(header);

    if ((DWORD)(dest - basedest) + channels * sizeof(SHORT) > destsize) {
      return (DWORD)(dest - basedest);
    }

    index[0] = index[1] = 44;

    for (channel = 0; channel < channels; ++channel) {
      last[channel] = *source++;
      *(SHORT *)dest = (SHORT)last[channel];
      dest += sizeof(SHORT);
    }

    escapebudget = (LONG)sourcesize / sizeof(SHORT) - (dest - basedest);
    if ((LONG)escapebudget < 0) {
      escapebudget = 0;
    }

    loop = channels;
    samples = sourcesize / sizeof(SHORT);
    if (loop >= samples) {
      return (DWORD)(dest - basedest);
    }

    channel = channels - 1;
    do {
      DWORD step;
      DWORD difference;
      DWORD sign;

      if ((DWORD)(dest - basedest + 2) > destsize) {
        return (DWORD)(dest - basedest + 2);
      }

      if (channels == 2) {
        channel = channel == 0;
      }

      val[channel] = *source++;
      difference = val[channel] - last[channel];
      sign = val[channel] < last[channel] ? 0x40 : 0;
      if ((LONG)difference < 0) {
        difference = -difference;
      }

      step = s_ima_steptable[index[channel]];
      if (difference < (step >> bitspersample)) {
        if (index[channel]) {
          --index[channel];
        }
        *dest++ = 0x80;
        continue;
      }

      if (difference > step * 2) {
        while (index[channel] < 88 && escapebudget) {
          index[channel] += 8;
          if (index[channel] > 88) {
            index[channel] = 88;
          }
          step = s_ima_steptable[index[channel]];
          *dest++ = 0x81;
          --escapebudget;
          if (difference <= step * 2) {
            break;
          }
        }
      }

      round = step >> header.roundshift;
      delta = 0;
      vpdiff = 0;
      bit = 1;
      highbit = 1UL << (bitspersample - 2);
      if (highbit > 0x20) {
        highbit = 0x20;
      }
      do {
        DWORD candidate;

        candidate = vpdiff + step;
        if (candidate <= difference) {
          delta |= bit;
          vpdiff = candidate;
        }
        if (bit != highbit) {
          step >>= 1;
          bit <<= 1;
        }
      } while (bit != highbit);

      vpdiff += round;

      if (sign) {
        last[channel] -= vpdiff;
        if (last[channel] < -32768) {
          last[channel] = -32768;
        }
      } else {
        last[channel] += vpdiff;
        if (last[channel] > 32767) {
          last[channel] = 32767;
        }
      }
      *dest++ = (BYTE)(delta | sign);
      index[channel] += s_ima_adjtable[delta & 0x1F];
      if (index[channel] < 0) {
        index[channel] = 0;
      } else if (index[channel] > 88) {
        index[channel] = 88;
      }
    } while (++loop < samples);

    return (DWORD)(dest - basedest);
  }

  static DWORD ImaAdpcmDecompress(SHORT *dest, DWORD destsize, const BYTE *source, DWORD sourcesize, DWORD channels) {
    int         last[2];
    int         index[2];
    _IMAHEADER *header;
    SHORT      *basedest;
    const BYTE *endsource;
    DWORD       channel;

    header = (_IMAHEADER *)source;
    basedest = dest;
    endsource = source + sourcesize;
    index[0] = index[1] = 44;
    source += sizeof(_IMAHEADER);

    for (channel = 0; channel < channels; ++channel) {
      last[channel] = *(const SHORT *)source;
      source += sizeof(SHORT);
      if (destsize < sizeof(SHORT)) {
        return (DWORD)((BYTE *)dest - (BYTE *)basedest);
      }
      *dest++ = (SHORT)last[channel];
      destsize -= sizeof(SHORT);
    }

    channel = channels - 1;
    while (source < endsource) {
      char code;

      if (channels == 2) {
        channel = channel == 0;
      }

      code = *source++;
      if (code < 0) {
        switch (code & 0x7F) {
          case 0:
            if (index[channel]) {
              --index[channel];
            }
            if (destsize < sizeof(SHORT)) {
              return (DWORD)((BYTE *)dest - (BYTE *)basedest);
            }
            *dest++ = (SHORT)last[channel];
            destsize -= sizeof(SHORT);
            break;
          case 1:
            index[channel] += 8;
            if (index[channel] > 88) {
              index[channel] = 88;
            }
            if (channels == 2) {
              channel = channel == 0;
            }
            break;
          case 2:
            index[channel] -= 8;
            if (index[channel] < 0) {
              index[channel] = 0;
            }
            if (channels == 2) {
              channel = channel == 0;
            }
            break;
        }
      } else {
        DWORD step;
        DWORD delta;

        step = s_ima_steptable[index[channel]];
        delta = step >> header->roundshift;
        if (code & 0x01) {
          delta += step;
        }
        if (code & 0x02) {
          delta += step >> 1;
        }
        if (code & 0x04) {
          delta += step >> 2;
        }
        if (code & 0x08) {
          delta += step >> 3;
        }
        if (code & 0x10) {
          delta += step >> 4;
        }
        if (code & 0x20) {
          delta += step >> 5;
        }
        if (code & 0x40) {
          int value;

          value = last[channel] - delta;
          if (value <= -32768) {
            value = -32768;
          }
          last[channel] = value;
        } else {
          int value;

          value = last[channel] + delta;
          if (value >= 32767) {
            value = 32767;
          }
          last[channel] = value;
        }

        if (destsize < sizeof(SHORT)) {
          return (DWORD)((BYTE *)dest - (BYTE *)basedest);
        }
        *dest++ = (SHORT)last[channel];
        destsize -= sizeof(SHORT);

        index[channel] += s_ima_adjtable[code & 0x1F];
        if (index[channel] < 0) {
          index[channel] = 0;
        } else if (index[channel] > 88) {
          index[channel] = 88;
        }
      }
    }

    return (DWORD)((BYTE *)dest - (BYTE *)basedest);
  }

}  // namespace

static void
HuffmanCompress(void *dest, unsigned long *destsize, const void *source, unsigned long sourcesize, unsigned long *hint, unsigned long optimization);

static unsigned int PkwareBufferRead(char *buffer, unsigned int *size, void *param) {
  (void)buffer;
  (void)size;
  (void)param;
  return 0;
}

static void PkwareBufferWrite(char *buffer, unsigned int *size, void *param) {
  (void)buffer;
  (void)size;
  (void)param;
}

static void
PkwareCompress(void *dest, unsigned long *destsize, const void *source, unsigned long sourcesize, unsigned long *hint, unsigned long optimization) {
  (void)dest;
  (void)source;
  (void)optimization;
  *destsize = sourcesize;
  if (hint) {
    *hint = SCOMP_HINT_NONE;
  }
}

static void PkwareDecompress(void *dest, unsigned long *destsize, const void *source, unsigned long sourcesize, const char *filename) {
  (void)dest;
  (void)source;
  (void)sourcesize;
  (void)filename;
  *destsize = 0;
}

static void *ZlibAlloc(void *opaque, unsigned int items, unsigned int size) {
  ZLIB_BUFFER *arena = (ZLIB_BUFFER *)opaque;
  DWORD        bytes;
  BYTE        *result;

  if (!arena) {
    SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "buffer", FALSE, 1);
  }
  bytes = items * size;
  if (bytes & 3) {
    bytes += 4 - (bytes & 3);
  }
  result = arena->current;
  if (bytes < arena->base + arena->size - result) {
    arena->current += bytes;
    return result;
  }
  return SMemAlloc(bytes, __FILE__, __LINE__, 0);
}

static void ZlibFree(void *opaque, void *ptr) {
  ZLIB_BUFFER *arena = (ZLIB_BUFFER *)opaque;
  if ((BYTE *)ptr < arena->base || (BYTE *)ptr >= arena->base + arena->size) {
    SMemFree(ptr, __FILE__, __LINE__, 0);
  }
}

extern "C" int __stdcall
zlib_compress(unsigned char *dest, unsigned long *destLen, const unsigned char *source, unsigned long sourceLen, int level) {
  ZlibCompressAllocBuffer buffer;
  z_stream                stream;
  int                     result;

  stream.next_in = const_cast<Bytef *>(source);
  stream.avail_in = sourceLen;
  stream.next_out = dest;
  stream.avail_out = *destLen;
  buffer.arena.current = buffer.workspace;
  buffer.arena.size = sizeof(buffer.workspace);
  buffer.arena.base = buffer.workspace;
  stream.zalloc = (alloc_func)ZlibAlloc;
  stream.zfree = (free_func)ZlibFree;
  stream.opaque = &buffer.arena;
  result = deflateInit(&stream, level);
  if (result == Z_OK) {
    result = deflate(&stream, Z_FINISH);
    if (result == Z_STREAM_END) {
      *destLen = stream.total_out;
      return deflateEnd(&stream);
    }
    deflateEnd(&stream);
    if (result == Z_OK) {
      result = Z_BUF_ERROR;
    }
  }
  return result;
}

static void
ZlibCompress(void *dest, unsigned long *destsize, const void *source, unsigned long sourcesize, unsigned long *hint, unsigned long optimization) {
  DWORD finalsize;
  int   result;
  int   level;

  switch (optimization) {
    case 1:
      level = Z_BEST_COMPRESSION;
      break;
    case 2:
      level = Z_BEST_SPEED;
      break;
    default:
      level = Z_DEFAULT_COMPRESSION;
      break;
  }
  finalsize = *destsize;
  result = zlib_compress(
      static_cast<unsigned char *>(dest),
      &finalsize,
      static_cast<const unsigned char *>(source),
      sourcesize,
      level
  );
  if (result == Z_OK) {
    *destsize = finalsize;
  }
  *hint = SCOMP_HINT_NONE;
}

extern "C" int __stdcall
zlib_uncompress(unsigned char *dest, unsigned long *destLen, const unsigned char *source, unsigned long sourceLen) {
  ZlibUncompressAllocBuffer buffer;
  z_stream                  stream;
  int                       result;

  stream.next_in = const_cast<Bytef *>(source);
  stream.avail_in = sourceLen;
  stream.next_out = dest;
  stream.avail_out = *destLen;
  buffer.arena.current = buffer.workspace;
  buffer.arena.size = sizeof(buffer.workspace);
  buffer.arena.base = buffer.workspace;
  stream.zalloc = (alloc_func)ZlibAlloc;
  stream.zfree = (free_func)ZlibFree;
  stream.opaque = &buffer.arena;
  result = inflateInit(&stream);
  if (result != Z_OK) {
    return result;
  }
  result = inflate(&stream, Z_FINISH);
  if (result != Z_STREAM_END) {
    inflateEnd(&stream);
    return result == Z_OK ? Z_BUF_ERROR : result;
  }
  *destLen = stream.total_out;
  return inflateEnd(&stream);
}

static void ZlibDecompress(void *dest, unsigned long *destsize, const void *source, unsigned long sourcesize, const char *filename) {
  DWORD size;

  size = *destsize;
  if (zlib_uncompress(
          static_cast<unsigned char *>(dest),
          &size,
          static_cast<const unsigned char *>(source),
          sourcesize
      )
      != Z_OK) {
    SErrDisplayError(0x85100083, filename, -4, NULL, TRUE, 0);
  }
  *destsize = size;
}

static void ImaAdpcmMonoCompress(
    void          *dest,
    unsigned long *destsize,
    const void    *source,
    unsigned long  sourcesize,
    unsigned long *hint,
    unsigned long  optimization
) {
  BYTE bitspersample;

  ImaAdpcmCheckOptimization(optimization, &bitspersample, hint);
  *destsize = ImaAdpcmCompress((BYTE *)dest, *destsize, (const SHORT *)source, sourcesize, 1, bitspersample);
}

static void
ImaAdpcmMonoDecompress(void *dest, unsigned long *destsize, const void *source, unsigned long sourcesize, const char *filename) {
  (void)filename;
  *destsize = ImaAdpcmDecompress((SHORT *)dest, *destsize, (const BYTE *)source, sourcesize, 1);
}

static void ImaAdpcmStereoCompress(
    void          *dest,
    unsigned long *destsize,
    const void    *source,
    unsigned long  sourcesize,
    unsigned long *hint,
    unsigned long  optimization
) {
  BYTE bitspersample;

  ImaAdpcmCheckOptimization(optimization, &bitspersample, hint);
  *destsize = ImaAdpcmCompress((BYTE *)dest, *destsize, (const SHORT *)source, sourcesize, 2, bitspersample);
}

static void
ImaAdpcmStereoDecompress(void *dest, unsigned long *destsize, const void *source, unsigned long sourcesize, const char *filename) {
  (void)filename;
  *destsize = ImaAdpcmDecompress((SHORT *)dest, *destsize, (const BYTE *)source, sourcesize, 2);
}

static void
HuffmanCompress(void *dest, unsigned long *destsize, const void *source, unsigned long sourcesize, unsigned long *hint, unsigned long optimization) {
  CBitOutput      output(dest, *destsize);
  CHuffmanEncoder huff;

  (void)optimization;
  *destsize = huff.Compress(&output, source, sourcesize, (BYTE)*hint);
}

static void HuffmanDecompress(void *dest, unsigned long *destsize, const void *source, unsigned long sourcesize, const char *filename) {
  CBitInput       input(source);
  CHuffmanDecoder huff;

  (void)filename;
  (void)sourcesize;
  *destsize = huff.Decompress(dest, *destsize, &input);
}

static int BuffersOverlap(const void *buf1, const void *buf2, unsigned long length) {
  const BYTE *leftBytes;
  const BYTE *rightBytes;

  leftBytes = (const BYTE *)buf1;
  rightBytes = (const BYTE *)buf2;
  return leftBytes + length > rightBytes && rightBytes + length > leftBytes;
}

static void *s_AllocDecompressBuffer(unsigned long size, int *global) {
  DWORD tid;
  void *result;

  s_decompCrit.Enter();
  tid = GetCurrentThreadId();
  if (!tid) {
    SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "myTID != 0", FALSE, 1);
  }

  if (!s_DecompressBuffer) {
    s_DecompressBufferSize = size;
    s_DecompressBuffer = (BYTE *)SMemAlloc(size, __FILE__, __LINE__, 0);
    s_DecompressBufferTID = 0;
  } else if (s_DecompressBufferTID) {
    result = SMemAlloc(size, __FILE__, __LINE__, 0);
    *global = 0;
    s_decompCrit.Leave();
    return result;
  }

  if (s_DecompressBufferSize < size) {
    SMemFree(s_DecompressBuffer, __FILE__, __LINE__, 0);
    s_DecompressBufferSize = size;
    s_DecompressBuffer = (BYTE *)SMemAlloc(size, __FILE__, __LINE__, 0);
  }

  *global = 1;
  s_DecompressBufferTID = tid;
  result = s_DecompressBuffer;
  s_decompCrit.Leave();
  return result;
}

static void s_FreeDecompressBuffer(void *buf, int global) {
  if (buf) {
    s_decompCrit.Enter();
    if (global) {
      s_DecompressBufferTID = 0;
    } else {
      SMemFree(buf, __FILE__, __LINE__, 0);
    }
    s_decompCrit.Leave();
  }
}

typedef void(*SCOMP_COMPRESS_CALLBACK)(void *, unsigned long *, const void *, unsigned long, unsigned long *, unsigned long);
typedef void(*SCOMP_DECOMPRESS_CALLBACK)(void *, unsigned long *, const void *, unsigned long, const char *);

struct _COMPRESSALGORITHM {
  DWORD                   codec;
  SCOMP_COMPRESS_CALLBACK callback;
};

struct _DECOMPRESSALGORITHM {
  DWORD                     codec;
  SCOMP_DECOMPRESS_CALLBACK callback;
};

static _COMPRESSALGORITHM s_compressalgorithm[ALGORITHMS] = {
    {  SCOMP_IMA_ADPCM_MONO,   ImaAdpcmMonoCompress},
    {SCOMP_IMA_ADPCM_STEREO, ImaAdpcmStereoCompress},
    {         SCOMP_HUFFMAN,        HuffmanCompress},
    {            SCOMP_ZLIB,           ZlibCompress},
    {          SCOMP_PKWARE,         PkwareCompress},
};

static _DECOMPRESSALGORITHM s_decompressalgorithm[ALGORITHMS] = {
    {          SCOMP_PKWARE,         PkwareDecompress},
    {            SCOMP_ZLIB,           ZlibDecompress},
    {         SCOMP_HUFFMAN,        HuffmanDecompress},
    {SCOMP_IMA_ADPCM_STEREO, ImaAdpcmStereoDecompress},
    {  SCOMP_IMA_ADPCM_MONO,   ImaAdpcmMonoDecompress},
};

extern "C" int APIENTRY
SCompCompress(void *dest, DWORD *destsize, const void *source, DWORD sourcesize, DWORD compressiontypes, DWORD hint, DWORD optimization) {
  BYTE       *work;
  const BYTE *current;
  DWORD       targetsize;
  DWORD       remainingTypes;
  DWORD       operations;
  DWORD       i;
  int         global;

  FATALASSERT(dest);
  FATALASSERT(destsize);
  FATALASSERT(*destsize >= sourcesize);
  FATALASSERT(source);

  operations = 0;
  remainingTypes = compressiontypes;
  for (i = 0; i < ALGORITHMS; ++i) {
    if (compressiontypes & s_compressalgorithm[i].codec) {
      ++operations;
    }
    remainingTypes &= ~s_compressalgorithm[i].codec;
  }
  if (remainingTypes) {
    return FALSE;
  }

  work = NULL;
  global = 0;
  if (operations >= 2 || (operations && BuffersOverlap(dest, source, sourcesize))) {
    work = (BYTE *)s_AllocDecompressBuffer(sourcesize, &global);
  }

  current = (const BYTE *)source;
  targetsize = sourcesize;

  for (i = 0; i < ALGORITHMS; ++i) {
    DWORD codec = s_compressalgorithm[i].codec;
    DWORD outSize;
    BYTE *target;

    if ((compressiontypes & codec) == 0) {
      continue;
    }

    --operations;
    target = (operations & 1) ? work : (BYTE *)dest + 1;
    if (BuffersOverlap(target, current, targetsize)) {
      target = BuffersOverlap(target, work, targetsize) ? (BYTE *)dest + 1 : work;
    }

    outSize = targetsize - 1;
    s_compressalgorithm[i].callback(target, &outSize, current, targetsize, &hint, optimization);

    if (outSize + 1 < targetsize) {
      current = target;
      targetsize = outSize;
    } else {
      compressiontypes &= ~codec;
    }
  }

  if (current != (const BYTE *)dest) {
    if (current == (const BYTE *)dest + 1) {
      *(BYTE *)dest = (BYTE)compressiontypes;
      ++targetsize;
    } else if (compressiontypes) {
      memcpy((BYTE *)dest + 1, current, targetsize);
      *(BYTE *)dest = (BYTE)compressiontypes;
      ++targetsize;
    } else {
      memcpy(dest, current, targetsize);
    }
  }
  *destsize = targetsize;
  s_FreeDecompressBuffer(work, global);
  return TRUE;
}

extern "C" int APIENTRY SCompDecompress(void *dest, DWORD *destsize, const void *source, DWORD sourcesize) {
  return SCompDecompress2(dest, destsize, source, sourcesize, NULL);
}

int APIENTRY SCompDecompress2(void *dest, DWORD *destsize, const void *source, DWORD sourcesize, const char *filename) {
  BYTE        compressiontypes;
  BYTE       *work;
  const BYTE *current;
  DWORD       destbuffersize;
  DWORD       targetsize;
  DWORD       remainingTypes;
  DWORD       operations;
  DWORD       i;
  int         global;

  destbuffersize = *destsize;
  FATALASSERT(dest);
  FATALASSERT(destbuffersize >= sourcesize);
  FATALASSERT(source);
  FATALASSERT(sourcesize >= sizeof(BYTE));

  if (destbuffersize == sourcesize) {
    if (dest != source) {
      memcpy(dest, source, sourcesize);
    }
    return TRUE;
  }

  current = (const BYTE *)source;
  compressiontypes = *current++;
  targetsize = sourcesize - 1;

  operations = 0;
  remainingTypes = compressiontypes;
  for (i = 0; i < ALGORITHMS; ++i) {
    if (compressiontypes & s_decompressalgorithm[i].codec) {
      ++operations;
    }
    remainingTypes &= ~s_decompressalgorithm[i].codec;
  }
  if (remainingTypes) {
    return FALSE;
  }

  work = NULL;
  global = 0;
  if (operations >= 2 || (operations && BuffersOverlap(dest, current, targetsize))) {
    work = (BYTE *)s_AllocDecompressBuffer(destbuffersize, &global);
  }

  for (i = 0; i < ALGORITHMS; ++i) {
    DWORD codec = s_decompressalgorithm[i].codec;
    DWORD outSize;
    BYTE *target;

    if ((compressiontypes & codec) == 0) {
      continue;
    }

    --operations;
    target = (operations & 1) ? work : (BYTE *)dest;
    if (BuffersOverlap(target, current, targetsize)) {
      target = BuffersOverlap(target, work, targetsize) ? (BYTE *)dest : work;
    }

    outSize = destbuffersize;
    s_decompressalgorithm[i].callback(target, &outSize, current, targetsize, filename);
    current = target;
    targetsize = outSize;
  }

  if ((const void *)dest != (const void *)current && targetsize) {
    memcpy(dest, current, targetsize);
  }
  *destsize = targetsize;
  s_FreeDecompressBuffer(work, global);
  return TRUE;
}

extern "C" int APIENTRY SCompDestroy() {
  if (s_DecompressBuffer) {
    SMemFree(s_DecompressBuffer, __FILE__, __LINE__, 0);
    s_DecompressBuffer = NULL;
  }

  return TRUE;
}
