#pragma once

#ifndef _WIN32
#include <otherwin32.h>
#else
// clang-format off
#include <windows.h>
// clang-format on

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#endif

#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sapibase.h>

#if defined(_MSC_VER) && _MSC_VER == 1200
const UINT  INFINITY_ENCODING = 0x7F800000;
const float INFINITY = *reinterpret_cast<const float *>(&INFINITY_ENCODING);
#endif

#define DECLARE_STRICT_HANDLE(name) \
  typedef struct name##__ {         \
    int unused;                     \
  } *name
#define DECLARE_DERIVED_HANDLE(name, base)    \
  typedef struct name##__ : public base##__ { \
    int unused;                               \
  } *name

#ifdef _MSC_VER
#ifdef _INC_TYPEINFO
#define INTERNALRAWNAME raw_name
#else
#define INTERNALRAWNAME internal_raw_name
class type_info {
 public:
  virtual ~type_info();
  LPCSTR internal_raw_name() const {
    return _m_d_name;
  };
  LPCSTR name() const;

 private:
  LPVOID _m_data;
  char   _m_d_name[1];
  type_info(const type_info &rhs);
  type_info &operator=(const type_info &rhs);
};
#endif
#else
#if defined(MAC) && !defined(__typeinfo__)
#include <typeinfo>
#endif
#define INTERNALRAWNAME name
#endif

#define LIST_UNLINKED 0
#define LIST_HEAD     1
#define LIST_TAIL     2

#define LIST_LINK_AFTER  LIST_HEAD
#define LIST_LINK_BEFORE LIST_TAIL

// --------------------------------
// Error codes
// --------------------------------

#define STORMFAC         0x510
#define STORMERROR(code) (0x80000000 | (STORMFAC << 16) | ((code) & 0xFFFF))

#define STORM_ERROR_ASSERTION               STORMERROR(0)
#define STORM_ERROR_BAD_ARGUMENT            STORMERROR(101)
#define STORM_ERROR_GAME_ALREADY_STARTED    STORMERROR(102)
#define STORM_ERROR_GAME_FULL               STORMERROR(103)
#define STORM_ERROR_GAME_NOT_FOUND          STORMERROR(104)
#define STORM_ERROR_GAME_TERMINATED         STORMERROR(105)
#define STORM_ERROR_INVALID_PLAYER          STORMERROR(106)
#define STORM_ERROR_NO_MESSAGES_WAITING     STORMERROR(107)
#define STORM_ERROR_NOT_ARCHIVE             STORMERROR(108)
#define STORM_ERROR_NOT_ENOUGH_ARGUMENTS    STORMERROR(109)
#define STORM_ERROR_NOT_IMPLEMENTED         STORMERROR(110)
#define STORM_ERROR_NOT_IN_ARCHIVE          STORMERROR(111)
#define STORM_ERROR_NOT_IN_GAME             STORMERROR(112)
#define STORM_ERROR_NOT_INITIALIZED         STORMERROR(113)
#define STORM_ERROR_NOT_PLAYING             STORMERROR(114)
#define STORM_ERROR_NOT_REGISTERED          STORMERROR(115)
#define STORM_ERROR_REQUIRES_CODEC          STORMERROR(116)
#define STORM_ERROR_REQUIRES_DDRAW          STORMERROR(117)
#define STORM_ERROR_REQUIRES_DSOUND         STORMERROR(118)
#define STORM_ERROR_REQUIRES_UPGRADE        STORMERROR(119)
#define STORM_ERROR_STILL_ACTIVE            STORMERROR(120)
#define STORM_ERROR_VERSION_MISMATCH        STORMERROR(121)
#define STORM_ERROR_MEMORY_ALREADY_FREED    STORMERROR(122)
#define STORM_ERROR_MEMORY_CORRUPT          STORMERROR(123)
#define STORM_ERROR_MEMORY_INVALID_BLOCK    STORMERROR(124)
#define STORM_ERROR_MEMORY_MANAGER_INACTIVE STORMERROR(125)
#define STORM_ERROR_MEMORY_NEVER_RELEASED   STORMERROR(126)
#define STORM_ERROR_HANDLE_NEVER_RELEASED   STORMERROR(127)
#define STORM_ERROR_ACCESS_OUT_OF_BOUNDS    STORMERROR(128)
#define STORM_ERROR_MEMORY_NULL_POINTER     STORMERROR(129)

// --------------------------------
// Error handling functions
// --------------------------------

#define SERR_LINECODE_FUNCTION  -1
#define SERR_LINECODE_OBJECT    -2
#define SERR_LINECODE_HANDLE    -3
#define SERR_LINECODE_FILE      -4
#define SERR_LINECODE_EXCEPTION -5

#define REPORTRESOURCELEAK(handle)

// --------------------------------
// Error handling functions
// --------------------------------

typedef BOOL(APIENTRY *SERRHANDLER)(DWORD errorcode, LPCSTR errorstr, LPCSTR filename, int linenumber, LPCSTR description);
typedef int(APIENTRY *SERRLOGCALLBACK)(LPSTR buffer, DWORD bufferchars);

void                      SErrInitialize();
extern "C" BOOL APIENTRY  SErrDestroy();
extern "C" BOOL APIENTRY  SErrCheckDebugSymbolLibrary(BOOL warnnotfound);
extern "C" void APIENTRY  SErrPrepareAppFatal(LPCSTR filename, int linenumber);
extern "C" void __cdecl   SErrDisplayAppFatal(LPCSTR format, ...);
extern "C" BOOL APIENTRY  SErrDisplayError(DWORD errorcode, LPCSTR filename, int linenumber, LPCSTR description, BOOL recoverable, UINT exitcode = 1);
extern "C" BOOL __cdecl   SErrDisplayErrorFmt(DWORD errorcode, LPCSTR filename, int linenumber, BOOL recoverable, UINT exitcode, LPCSTR format, ...);
extern "C" BOOL APIENTRY  SErrGetErrorStr(DWORD errorcode, char *buffer, DWORD bufferchars);
extern "C" DWORD APIENTRY SErrGetLastError();
extern "C" BOOL APIENTRY  SErrIsDisplayingError();
extern "C" void APIENTRY  SErrRegisterHandler(SERRHANDLER handler);
extern "C" BOOL APIENTRY  SErrRegisterMessageSource(WORD facility, HINSTANCE module, LPVOID reserved);
extern "C" void APIENTRY  SErrReportResourceLeak(LPCSTR handlename);
extern "C" void APIENTRY  SErrReportNamedResourceLeak(LPCSTR handlename, LPCSTR name);
extern "C" void APIENTRY  SErrSetLastError(DWORD errorcode);
extern "C" void APIENTRY  SErrSetLogCallback(SERRLOGCALLBACK cb);
extern "C" void APIENTRY  SErrSetLogTitleString(LPCSTR title);
extern "C" BOOL APIENTRY  SErrGetLogLastPath(char *buf, int size);
extern "C" void APIENTRY  SErrSuppressErrors(BOOL suppress);
extern "C" void APIENTRY  SErrUnregisterHandler(SERRHANDLER handler);
extern "C" void APIENTRY  SErrRegisterThread(HANDLE thread, DWORD threadid);
extern "C" void APIENTRY  SErrUnregisterThread(HANDLE thread, DWORD threadid);
extern "C" void APIENTRY  SErrLogRegisteredThreads(LPCSTR description, LPCSTR suffix);
extern "C" void APIENTRY  SErrLogThreads(HANDLE *threads, LPDWORD threadids, int numthreads, LPCSTR description, LPCSTR suffix);
extern "C" void APIENTRY  SErrStartWatchdog(DWORD freezeSeconds, BOOL checkKeyboard);
extern "C" void APIENTRY  SErrPauseWatchdog();
extern "C" void APIENTRY  SErrResumeWatchdog();
extern "C" void APIENTRY  SErrStopWatchdog();
extern "C" void APIENTRY  SErrPingWatchdog();
extern "C" void APIENTRY  SErrCatchUnhandledExceptions();

// --------------------------------
// Event functions
// --------------------------------

typedef void(APIENTRY *SEVTHANDLER)(LPVOID data);

extern "C" BOOL APIENTRY SEvtBreakHandlerChain(LPVOID data);
extern "C" BOOL APIENTRY SEvtDestroy();
extern "C" BOOL APIENTRY SEvtDispatch(DWORD type, DWORD subtype, DWORD id, LPVOID data);
extern "C" BOOL APIENTRY SEvtPopState(DWORD type, DWORD subtype);
extern "C" BOOL APIENTRY SEvtPushState(DWORD type, DWORD subtype);
extern "C" BOOL APIENTRY SEvtRegisterHandler(DWORD type, DWORD subtype, DWORD id, DWORD flags, SEVTHANDLER handler);
extern "C" BOOL APIENTRY SEvtUnregisterHandler(DWORD type, DWORD subtype, DWORD id, SEVTHANDLER handler);
extern "C" BOOL APIENTRY SEvtUnregisterType(DWORD type, DWORD subtype);

// --------------------------------
// Message functions
// --------------------------------

typedef BOOL(APIENTRY *SMSGIDLEPROC)(DWORD count);

typedef struct _PARAMS {
  HWND   window;
  UINT   message;
  UINT   wparam;
  LONG   lparam;
  UINT   notifycode;
  LPVOID extra;
  BOOL   useresult;
  LONG   result;
} SMSGPARAMS, *LPSMSGPARAMS;

typedef void(APIENTRY *SMSGHANDLER)(SMSGPARAMS *params);

extern "C" BOOL APIENTRY    SMsgBreakHandlerChain(SMSGPARAMS *params);
extern "C" BOOL APIENTRY    SMsgDestroy();
extern "C" BOOL APIENTRY    SMsgDispatchMessage(HWND window, UINT message, UINT wparam, LONG lparam, BOOL *useresult, LONG *result);
extern "C" int APIENTRY     SMsgDoMessageLoop(SMSGIDLEPROC idleproc, BOOL cleanuponquit);
extern "C" HWND APIENTRY    SMsgGetDefaultWindow();
extern "C" BOOL APIENTRY    SMsgGetDefaultWindowRect(LPRECT rect);
extern "C" WNDPROC APIENTRY SMsgGetGenericWndProc(DWORD id);
extern "C" BOOL APIENTRY    SMsgPopRegisterState(HWND window);
extern "C" BOOL APIENTRY    SMsgPushRegisterState(HWND window);
extern "C" BOOL APIENTRY    SMsgRegisterCommand(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgRegisterSysCommand(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgRegisterKeyDown(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgRegisterKeyUp(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgRegisterMessage(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgSetDefaultWindow(HWND window);
extern "C" void APIENTRY    SMsgSetDefaultWindowRect(const RECT *rect);
extern "C" BOOL APIENTRY    SMsgUnregisterCommand(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgUnregisterSysCommand(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgUnregisterKeyDown(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgUnregisterKeyUp(HWND window, UINT id, SMSGHANDLER handler);
extern "C" BOOL APIENTRY    SMsgUnregisterMessage(HWND window, UINT id, SMSGHANDLER handler);

// --------------------------------
// Command-line functions
// --------------------------------

#define SCMD_ARG_FLAGGED  (0 << 24)
#define SCMD_ARG_OPTIONAL (1 << 24)
#define SCMD_ARG_REQUIRED (2 << 24)
#define SCMD_ARG_MASK     (SCMD_ARG_FLAGGED | SCMD_ARG_OPTIONAL | SCMD_ARG_REQUIRED)

#define SCMD_BOOL_SET   0
#define SCMD_BOOL_CLEAR 1
#define SCMD_BOOL_MASK  (SCMD_BOOL_SET | SCMD_BOOL_CLEAR)

#define SCMD_CASESENSITIVE (0x01 << 8)

#define SCMD_NUM_UNSIGNED 0
#define SCMD_NUM_SIGNED   1
#define SCMD_NUM_MASK     (SCMD_NUM_UNSIGNED | SCMD_NUM_SIGNED)

#define SCMD_TYPE_BOOL    (0 << 16)
#define SCMD_TYPE_NUMERIC (1 << 16)
#define SCMD_TYPE_STRING  (2 << 16)
#define SCMD_TYPE_MASK    (SCMD_TYPE_BOOL | SCMD_TYPE_NUMERIC | SCMD_TYPE_STRING)

typedef struct _CMDERROR {
  DWORD  errorcode;
  LPCSTR itemstr;
  LPCSTR errorstr;
} CMDERROR, *CMDERRORPTR;

typedef CMDERROR  SCMDERROR;
typedef CMDERROR *LPSCMDERROR;

typedef struct _CMDPARAMS {
  DWORD  flags;
  DWORD  id;
  LPCSTR name;
  LPVOID variable;
  DWORD  setvalue;
  DWORD  setmask;
  union {
    BOOL   boolvalue;
    LONG   signedvalue;
    DWORD  unsignedvalue;
    LPCSTR stringvalue;
  };
} CMDPARAMS, *CMDPARAMSPTR;

typedef BOOL(APIENTRY *SCMDPROCESSCALLBACK)(LPCSTR value);
typedef BOOL(APIENTRY *SCMDCALLBACK)(CMDPARAMSPTR params, LPCSTR value);
typedef void(APIENTRY *SCMDERRORCALLBACK)(CMDERRORPTR error);
typedef SCMDCALLBACK SCMDARGCALLBACK;

typedef struct _ARGLIST {
  DWORD        flags;
  DWORD        id;
  LPCSTR       name;
  SCMDCALLBACK callback;
} ARGLIST, *LPARGLIST;

extern "C" BOOL APIENTRY  SCmdCheckId(DWORD id);
extern "C" BOOL APIENTRY  SCmdDestroy();
extern "C" BOOL APIENTRY  SCmdGetBool(DWORD id);
extern "C" DWORD APIENTRY SCmdGetNum(DWORD id);
extern "C" BOOL APIENTRY  SCmdGetString(DWORD id, char *buffer, DWORD bufferchars);
extern "C" BOOL APIENTRY  SCmdGetStringAlloc(DWORD id, char **buffer);
extern "C" BOOL APIENTRY  SCmdProcess(LPCSTR cmdline, int skipprogname, SCMDPROCESSCALLBACK extracallback, SCMDERRORCALLBACK errorcallback);
extern "C" BOOL APIENTRY  SCmdProcessCommandLine(SCMDPROCESSCALLBACK extracallback, SCMDERRORCALLBACK errorcallback);
extern "C" BOOL APIENTRY  SCmdRegisterArgList(const ARGLIST *listptr, DWORD numargs);
extern "C" BOOL APIENTRY  SCmdRegisterArgument(
    DWORD        flags,
    DWORD        id,
    LPCSTR       name,
    LPVOID       variableptr,
    DWORD        variablebytes,
    DWORD        setvalue,
    DWORD        setmask,
    SCMDCALLBACK callback
);

// --------------------------------
// Compression functions
// --------------------------------

#define SCOMP_HUFFMAN          0x00000001
#define SCOMP_ZLIB             0x00000002
#define SCOMP_PKWARE           0x00000008
#define SCOMP_IMA_ADPCM_MONO   0x00000040
#define SCOMP_IMA_ADPCM_STEREO 0x00000080

extern "C" int APIENTRY
SCompCompress(LPVOID dest, DWORD *destsize, LPCVOID source, DWORD sourcesize, DWORD compressiontypes, DWORD hint, DWORD optimization);
extern "C" int APIENTRY SCompDecompress(LPVOID dest, DWORD *destsize, LPCVOID source, DWORD sourcesize);
int APIENTRY            SCompDecompress2(LPVOID dest, DWORD *destsize, LPCVOID source, DWORD sourcesize, LPCSTR filename);
BOOL                    SCompPkwareDecompressBuffer(BYTE *dest, DWORD *destsize, const BYTE *source, DWORD sourcesize);
extern "C" int APIENTRY SCompDestroy();

// --------------------------------
// Big integer functions
// --------------------------------

class BigData;
template <class T>
class TSGrowableArray;

extern "C" void APIENTRY SBigAdd(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigAnd(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigBitLen(BigData *a, UINT *bits);
extern "C" int APIENTRY  SBigCompare(const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigCopy(BigData *a, const BigData &b);
extern "C" void APIENTRY SBigDec(BigData *a, const BigData &b);
extern "C" void APIENTRY SBigDel(BigData *num);
extern "C" void APIENTRY SBigDiv(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigFindPrime(BigData *a, UINT bits, const BigData &c, const BigData &d);
extern "C" void APIENTRY SBigFromBinary(BigData *num, LPCVOID data, UINT bytes);
extern "C" void APIENTRY SBigFromStr(BigData *num, LPCSTR str);
extern "C" void APIENTRY SBigFromStream(BigData *num, LPCVOID data, UINT maxBytes, UINT *bytes);
extern "C" void APIENTRY SBigFromUnsigned(BigData *num, UINT val);
extern "C" void APIENTRY SBigGcd(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigInc(BigData *a, const BigData &b);
extern "C" void APIENTRY SBigInvMod(BigData *a, const BigData &b, const BigData &c);
extern "C" int APIENTRY  SBigIsEven(const BigData &a);
extern "C" int APIENTRY  SBigIsOdd(const BigData &a);
extern "C" int APIENTRY  SBigIsOne(const BigData &a);
extern "C" int APIENTRY  SBigIsPrime(const BigData &a);
extern "C" int APIENTRY  SBigIsZero(const BigData &a);
extern "C" void APIENTRY SBigMod(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigMul(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigMulMod(BigData *a, const BigData &b, const BigData &c, const BigData &d);
extern "C" void APIENTRY SBigNew(BigData **num);
extern "C" void APIENTRY SBigNot(BigData *a, const BigData &b);
extern "C" void APIENTRY SBigOr(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigPow(BigData *a, const BigData &b, UINT c);
extern "C" void APIENTRY SBigPowMod(BigData *a, const BigData &b, const BigData &c, const BigData &d);
extern "C" void APIENTRY SBigRand(BigData *a, const BigData &b, BigData *seed);
extern "C" void APIENTRY SBigSet2Exp(BigData *a, UINT b);
extern "C" void APIENTRY SBigSetOne(BigData *a);
extern "C" void APIENTRY SBigSetZero(BigData *a);
extern "C" void APIENTRY SBigShl(BigData *a, const BigData &b, UINT bits);
extern "C" void APIENTRY SBigShr(BigData *a, const BigData &b, UINT bits);
extern "C" void APIENTRY SBigSquare(BigData *a, const BigData &b);
extern "C" void APIENTRY SBigSub(BigData *a, const BigData &b, const BigData &c);
extern "C" void APIENTRY SBigToBinaryArray(const BigData &num, TSGrowableArray<BYTE> *array, int append);
extern "C" void APIENTRY SBigToBinaryBuffer(const BigData &num, LPVOID data, UINT maxBytes, UINT *bytes);
extern "C" void APIENTRY SBigToBinaryPtr(const BigData &num, LPCVOID *data, UINT *bytes);
extern "C" void APIENTRY SBigToStrArray(const BigData &num, TSGrowableArray<char> *array, int append);
extern "C" void APIENTRY SBigToStrBuffer(const BigData &num, char *str, UINT chars);
extern "C" void APIENTRY SBigToStrPtr(const BigData &num, LPCSTR *str);
extern "C" void APIENTRY SBigToStreamArray(const BigData &num, TSGrowableArray<BYTE> *array, int append);
extern "C" void APIENTRY SBigToStreamBuffer(const BigData &num, LPVOID data, UINT maxBytes, UINT *bytes);
extern "C" void APIENTRY SBigToStreamPtr(const BigData &num, LPCVOID *data, UINT *bytes);
extern "C" void APIENTRY SBigToUnsigned(const BigData &num, UINT *val);
extern "C" void APIENTRY SBigXor(BigData *a, const BigData &b, const BigData &c);

// --------------------------------
// Machine-state logging
// --------------------------------

typedef void(__cdecl *LOGMACHINESTATEPROC)(LPVOID param, LPCSTR format, ...);

void LogComputerInfoHeader(UINT logOptions, LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam, LPCSTR title, SYSTEMTIME *localTime);
void LogMachineState(UINT logOptions, LOGMACHINESTATEPROC logLineProc, LPVOID logLineProcParam, UINT stackFramesToSkip, CONTEXT *context);

// --------------------------------
// Utility functions
// --------------------------------

int __cdecl vsnoprintf(char *out, int outSize, LPCSTR format, char *argumentList);
int __cdecl vsoprintf(char *out, LPCSTR format, char *argumentList);
int __cdecl snoprintf(char *out, int maxchars, LPCSTR format, ...);
int __cdecl soprintf(char *out, LPCSTR format, ...);

DWORD                     CrcBuffer(LPCVOID buffer, DWORD len, DWORD *pcrc, DWORD stage);
extern "C" DWORD APIENTRY SCrcBuffer(LPCVOID buffer, DWORD len, DWORD *pcrc, DWORD stage);

class Sha1 {
 public:
  enum {
    DIGEST = 0x14,
    DATA_SIZE = 0x40,
    DATA_MASK = 0x3F
  };

  void Initialize();
  void Append(LPCVOID _data, DWORD size);
  void Append(LPCSTR data);
  void Finalize(BYTE *hash);

  static void Hash(BYTE *hash, LPCVOID data, DWORD size);
  static void Hash(BYTE *hash, LPCSTR data);

 private:
  static void Pump(DWORD *hash, const BYTE *data);

  DWORDLONG m_size;
  DWORD     m_hash[5];
  BYTE      m_data[DATA_SIZE];
};

class BigNum {
 public:
  BigNum();
  BigNum(LPCSTR value);
  BigNum(UINT value);
  BigNum(const BigNum &copy);
  ~BigNum();

  BigNum &operator=(LPCSTR value);
  BigNum &operator=(UINT value);
  BigNum &operator=(const BigNum &copy);

  BigNum &Add(const BigNum &a, const BigNum &b);
  BigNum &Sub(const BigNum &a, const BigNum &b);
  BigNum &Mul(const BigNum &a, const BigNum &b);
  BigNum &Div(const BigNum &a, const BigNum &b);
  BigNum &Mod(const BigNum &a, const BigNum &b);
  BigNum &And(const BigNum &a, const BigNum &b);
  BigNum &Or(const BigNum &a, const BigNum &b);
  BigNum &Xor(const BigNum &a, const BigNum &b);
  BigNum &Not(const BigNum &a);
  BigNum &Dec(const BigNum &a);
  BigNum &Inc(const BigNum &a);
  BigNum &Shl(const BigNum &a, UINT bits);
  BigNum &Shr(const BigNum &a, UINT bits);

  BigNum  operator+(const BigNum &value);
  BigNum  operator-(const BigNum &value);
  BigNum  operator*(const BigNum &value);
  BigNum  operator/(const BigNum &value);
  BigNum  operator%(const BigNum &value);
  BigNum  operator&(const BigNum &value);
  BigNum  operator|(const BigNum &value);
  BigNum  operator^(const BigNum &value);
  BigNum  operator<<(UINT bits);
  BigNum  operator>>(UINT bits);
  BigNum  operator~();
  BigNum &operator--();
  BigNum  operator--(int);
  BigNum &operator++();
  BigNum  operator++(int);
  BigNum &operator+=(const BigNum &value);
  BigNum &operator-=(const BigNum &value);
  BigNum &operator*=(const BigNum &value);
  BigNum &operator/=(const BigNum &value);
  BigNum &operator%=(const BigNum &value);
  BigNum &operator&=(const BigNum &value);
  BigNum &operator|=(const BigNum &value);
  BigNum &operator^=(const BigNum &value);
  BigNum &operator<<=(UINT bits);
  BigNum &operator>>=(UINT bits);

  int Compare(const BigNum &value);
  int operator==(const BigNum &value);
  int operator!=(const BigNum &value);
  int operator<=(const BigNum &value);
  int operator>=(const BigNum &value);
  int operator<(const BigNum &value);
  int operator>(const BigNum &value);

  BigNum &FindPrime(UINT bits, const BigNum &seed);
  BigNum &FindPrime(UINT bits, const BigNum &minimum, const BigNum &maximum);
  BigNum &Gcd(const BigNum &value);
  BigNum &Gcd(const BigNum &a, const BigNum &b);
  BigNum &InvMod(const BigNum &value);
  BigNum &InvMod(const BigNum &a, const BigNum &b);
  BigNum &MulMod(const BigNum &a, const BigNum &b);
  BigNum &MulMod(const BigNum &a, const BigNum &b, const BigNum &modulus);
  BigNum &Pow(UINT exponent);
  BigNum &Pow(const BigNum &value, UINT exponent);
  BigNum &PowMod(const BigNum &value, const BigNum &modulus);
  BigNum &PowMod(const BigNum &b, const BigNum &c, const BigNum &d);
  BigNum &Rand(const BigNum &maximum, BigNum *seed);
  BigNum &Square();
  BigNum &Square(const BigNum &value);

  char   *ToStr(char *buffer, UINT bytes) const;
  LPVOID  ToBinaryBuffer(LPVOID data, UINT bytes) const;
          operator UINT();
  void    FromBinary(LPCVOID data, UINT bytes);
  BOOL    IsEven();
  BOOL    IsOdd();
  BOOL    IsOne();
  BOOL    IsPrime();
  BOOL    IsZero();
  BigNum &Set2Exp(UINT exponent);
  BigNum &SetOne();
  BigNum &SetZero();

 private:
  BigData *m_data;
};

inline BigNum::BigNum() {
  SBigNew(&m_data);
}

inline BigNum::BigNum(const BigNum &copy) {
  SBigNew(&m_data);
  SBigCopy(m_data, *copy.m_data);
}

inline BigNum::~BigNum() {
  SBigDel(m_data);
}

inline BigNum &BigNum::operator=(const BigNum &copy) {
  SBigCopy(m_data, *copy.m_data);
  return *this;
}

namespace Crypt {
  class RSA {
   public:
    void Prepare(LPCVOID modulus, DWORD mLength, LPCVOID exponent, DWORD eLength);
    void Process(BYTE *data, DWORD size);

   private:
    BigNum m_modulus;
    BigNum m_exponent;
  };
}

class SSignatureData;

extern "C" void  SSignatureVerifyStream_Begin(SSignatureData **token, DWORD modulusSize, DWORD pubExponentSize);
extern "C" DWORD SSignatureVerifyStream_GetSignatureLength(SSignatureData *token);
extern "C" void  SSignatureVerifyStream_ProvideData(SSignatureData *token, const BYTE *data, DWORD size);
extern "C" int   SSignatureVerifyStream_Finish(SSignatureData *token, const BYTE *modulus, const BYTE *pubExponent);
extern "C" int SSignatureVerify(const BYTE *data, DWORD size, const BYTE *modulus, DWORD modulusSize, const BYTE *pubExponent, DWORD pubExponentSize);
extern "C" int SSignatureGenerate(
    BYTE       *data,
    DWORD      &size,
    const BYTE *modulus,
    DWORD       modulusSize,
    const BYTE *privExponent,
    DWORD       privExponentSize,
    const BYTE *pubExponent,
    DWORD       pubExponentSize
);

// clang-format off
#define ASSERT(a)                         \
  if (!(a))                              \
    SErrDisplayError(STORM_ERROR_ASSERTION, \
                     __FILE__,           \
                     __LINE__,           \
                     #a,                 \
                     FALSE)
#define FATALASSERT(a)                    \
  if (!(a))                              \
    SErrDisplayError(STORM_ERROR_ASSERTION, \
                     __FILE__,           \
                     __LINE__,           \
                     #a,                 \
                     FALSE)
#define FATALERROR(args)                  \
  do {                                    \
    SErrPrepareAppFatal(__FILE__, __LINE__); \
    SErrDisplayAppFatal args;             \
  } while (0)
#define VALIDATEBEGIN \
  do {
#define VALIDATE(a) ASSERT(a)
#define VALIDATEANDBLANK(a) \
  do {                      \
    ASSERT(a);              \
    *(a) = 0;               \
  } while (0)
#define VALIDATEEND \
  } while (0)
#define VALIDATEENDVOID \
  } while (0)
// clang-format on

// --------------------------------
// Memory allocation functions
// --------------------------------

#define SMEM_FLAG_ZEROMEMORY        0x00000008
#define SMEM_FLAG_PRESERVEONDESTROY 0x08000000

DECLARE_STRICT_HANDLE(HSHEAP);
DECLARE_STRICT_HANDLE(HOUTPUTCONTEXT);

typedef enum SMEMREPORTTYPE {
  SMEM_REPORT_BY_CALLER = 0,
  SMEM_REPORT_HISTOGRAM = 1
} SMEMREPORTTYPE;

typedef struct _SMEMBLOCKDETAILS {
  DWORD  size;
  LPVOID ptr;
  BOOL   allocated;
  BOOL   valid;
  DWORD  bytes;
  DWORD  overhead;
  DWORD  flags;
} SMEMBLOCKDETAILS, *LPSMEMBLOCKDETAILS;

typedef struct _SMEMHEAPDETAILS {
  DWORD  size;
  HSHEAP handle;
  char   filename[MAX_PATH];
  int    linenumber;
  DWORD  regions;
  DWORD  committedbytes;
  DWORD  reservedbytes;
  DWORD  maximumsize;
  DWORD  allocatedblocks;
  DWORD  allocatedbytes;
} SMEMHEAPDETAILS, *LPSMEMHEAPDETAILS;

typedef struct _SMEMHEAPDETAILS2 {
  DWORD  size;
  HSHEAP handle;
  char   filename[MAX_PATH];
  int    linenumber;
  DWORD  regions;
  DWORD  committedbytes;
  DWORD  reservedbytes;
  DWORD  maximumsize;
  DWORD  cumulativeAllocs;
  DWORD  cumulativeFrees;
  DWORD  cumulativeReallocs;
  DWORD  allocatedblocks;
  DWORD  allocatedbytes;
  DWORD  mark_allocatedblocks;
  DWORD  mark_allocatedbytes;
  DWORD  mark_committedbytes;
  DWORD  mark_cumulativeAllocs;
  DWORD  mark_cumulativeFrees;
  DWORD  mark_cumulativeReallocs;
} SMEMHEAPDETAILS2, *LPSMEMHEAPDETAILS2;

typedef struct SMemReportByCallerInfo {
  DWORD numSubHeaps;
  DWORD cumulativeAllocs;
  DWORD cumulativeFrees;
  DWORD cumulativeReallocs;
  DWORD allocatedBlocks;
  DWORD allocatedBytes;
  DWORD committedBytes;
  DWORD reservedBytes;
  DWORD mark_allocatedBlocks;
  DWORD mark_allocatedBytes;
  DWORD mark_committedBytes;
  DWORD mark_cumulativeAllocs;
  DWORD mark_cumulativeFrees;
  DWORD mark_cumulativeReallocs;
  int   lineNumber;
  char  fileName[0x100];
} SMemReportByCallerInfo;

typedef void(APIENTRY *SMEMREPORTPROC)(HOUTPUTCONTEXT outputcontext, LPCSTR report);
typedef void(APIENTRY *SMEMDUMPPROC)(HOUTPUTCONTEXT outputcontext, LPCSTR text);

extern "C" LPVOID APIENTRY SMemAlloc(DWORD bytes, LPCSTR filename = NULL, int linenumber = 0, DWORD flags = 0);
extern "C" BOOL APIENTRY   SMemDestroy();
extern "C" BOOL APIENTRY   SMemDumpState(SMEMDUMPPROC outputproc, HOUTPUTCONTEXT outputcontext);
BOOL APIENTRY              SMemDumpStateEx(char *arglist);
void                       SMemGenerateReport(SMEMREPORTTYPE reporttype, SMEMREPORTPROC outputproc, HOUTPUTCONTEXT outputcontext);
BOOL APIENTRY              SMemMarkAllHeapsEx(char *arglist);
extern "C" BOOL APIENTRY   SMemFindNextBlock(HSHEAP heap, LPVOID prevblock, LPVOID *nextblock, LPSMEMBLOCKDETAILS details);
extern "C" void APIENTRY   SMemHeapGetDetails(HSHEAP heap, LPSMEMHEAPDETAILS details);
extern "C" BOOL APIENTRY   SMemFindNextHeap(HSHEAP prevheap, HSHEAP *nextheap, LPSMEMHEAPDETAILS details);
BOOL APIENTRY              SMemFindNextHeap2(HSHEAP prevheap, HSHEAP *nextheap, LPSMEMHEAPDETAILS2 details);
extern "C" BOOL APIENTRY   SMemFree(LPVOID ptr, LPCSTR filename = NULL, int linenumber = 0, DWORD flags = 0);
extern "C" DWORD APIENTRY  SMemGetAllocated(LPDWORD allocated, LPDWORD committed, LPDWORD reserved);
extern "C" HSHEAP APIENTRY SMemGetHeapByCaller(LPCSTR filename, int linenumber);
extern "C" HSHEAP APIENTRY SMemGetHeapByPtr(LPVOID ptr);
extern "C" DWORD APIENTRY  SMemGetSize(LPVOID ptr, LPCSTR filename, int linenumber);
extern "C" LPVOID APIENTRY SMemHeapAlloc(HSHEAP handle, DWORD flags, DWORD bytes);
extern "C" HSHEAP APIENTRY SMemHeapCreate(DWORD options, DWORD initialsize, DWORD maximumsize, LPCSTR filename, int linenumber);
extern "C" BOOL APIENTRY   SMemHeapDestroy(HSHEAP handle);
extern "C" BOOL APIENTRY   SMemHeapFree(HSHEAP handle, DWORD flags, LPVOID ptr);
extern "C" LPVOID APIENTRY SMemHeapReAlloc(HSHEAP handle, DWORD flags, LPVOID ptr, DWORD bytes);
extern "C" DWORD APIENTRY  SMemHeapSize(HSHEAP handle, DWORD flags, LPVOID ptr);
extern "C" void APIENTRY   SMemInitialize();
extern "C" BOOL APIENTRY   SMemIsValidPointer(LPCVOID address, DWORD size, BOOL forWriting);
extern "C" LPVOID APIENTRY SMemReAlloc(LPVOID ptr, DWORD bytes, LPCSTR filename = NULL, int linenumber = 0, DWORD flags = 0);
extern "C" void APIENTRY   SMemSetDebugFlags(DWORD flags, DWORD changeMask);
extern "C" void __cdecl    SMemTrace(LPCSTR format, ...);

// --------------------------------
// Interlocked functions
// --------------------------------

LPVOID   SInterlockedExchangePointer(LPVOID *destPtr, LPVOID exchange);
LPVOID   SInterlockedCompareExchangePointer(LPVOID *destPtr, LPVOID exchange, LPVOID comperand);
long     SInterlockedIncrement(long *valuePtr);
long     SInterlockedDecrement(long *valuePtr);
long     SInterlockedExchangeAdd(long *valuePtr, long delta);
long     SInterlockedExchangeSub(long *valuePtr, long delta);
long     SInterlockedExchange(long *destPtr, long exchange);
long     SInterlockedCompareExchange(long *destPtr, long exchange, long comperand);
LONGLONG SInterlockedIncrement(LONGLONG *valuePtr);
LONGLONG SInterlockedDecrement(LONGLONG *valuePtr);
LONGLONG SInterlockedExchangeAdd(LONGLONG *valuePtr, long delta);
LONGLONG SInterlockedExchangeSub(LONGLONG *valuePtr, long delta);
LONGLONG SInterlockedExchangeAdd(LONGLONG *valuePtr, const LONGLONG &delta);
LONGLONG SInterlockedExchangeSub(LONGLONG *valuePtr, const LONGLONG &delta);
LONGLONG SInterlockedRead(const LONGLONG *sourcePtr);
LONGLONG SInterlockedExchange(LONGLONG *destPtr, const LONGLONG &exchange);
LONGLONG SInterlockedCompareExchange(LONGLONG *destPtr, const LONGLONG &exchange, const LONGLONG &comperand);
void     SInterlockedIncrementNonAtomic(LONGLONG *valuePtr);
void     SInterlockedDecrementNonAtomic(LONGLONG *valuePtr);
void     SInterlockedAddNonAtomic(LONGLONG *valuePtr, long delta);
void     SInterlockedSubNonAtomic(LONGLONG *valuePtr, long delta);
void     SInterlockedAddNonAtomic(LONGLONG *valuePtr, const LONGLONG &delta);
void     SInterlockedSubNonAtomic(LONGLONG *valuePtr, const LONGLONG &delta);

// --------------------------------
// Synchronization functions
// --------------------------------

class SCritSect {
 private:
  BYTE m_opaqueData[0x18];
  SCritSect(const SCritSect &);

 public:
  SCritSect();
  ~SCritSect();
  void Enter();
  void Leave();
  BOOL TryEnter();

 private:
  SCritSect &operator=(const SCritSect &);
};

class CDebugSCritSect : private SCritSect {
 private:
  BYTE m_debugData[0x0C];

  CDebugSCritSect();
  CDebugSCritSect(const CDebugSCritSect &);
  CDebugSCritSect &operator=(const CDebugSCritSect &);

 public:
  ~CDebugSCritSect();
  void        Enter(LPCSTR fileName, DWORD line);
  void        Leave(LPCSTR fileName, DWORD line);
  BOOL        TryEnter(LPCSTR fileName, DWORD line);
  static void DumpAllEntries();
};

class CSRWLock {
 private:
  BYTE m_opaqueData[0x0C];
  CSRWLock(const CSRWLock &);

 public:
  CSRWLock();
  ~CSRWLock();
  void Enter(int forwriting);
  void Leave(int fromwriting);
  BOOL TryEnter(int forwriting);

 private:
  CSRWLock &operator=(const CSRWLock &);
};

class CDebugSRWLock : private CSRWLock {
 private:
  BYTE m_debugData[0x0C];

  CDebugSRWLock();
  CDebugSRWLock(const CDebugSRWLock &);
  CDebugSRWLock &operator=(const CDebugSRWLock &);

 public:
  ~CDebugSRWLock();
  void        Enter(int forwriting, LPCSTR fileName, DWORD line);
  void        Leave(int fromwriting, LPCSTR fileName, DWORD line);
  BOOL        TryEnter(int forwriting, LPCSTR fileName, DWORD line);
  static void DumpAllEntries();
};

class SSyncObject {
  friend DWORD WaitMultiplePtr(UINT, SSyncObject **, int, DWORD);

 public:
  SSyncObject();
  SSyncObject(const SSyncObject &rhs);
  ~SSyncObject();
  SSyncObject &operator=(const SSyncObject &rhs);
  BOOL         Valid();
  void         Close();
  DWORD        Wait(DWORD timeoutMs);

 protected:
  BYTE m_opaqueData[0x04];

  void Copy(const SSyncObject &rhs);
};

class SInitCritSect {
 private:
  LONG       m_spinLock;
  SCritSect *m_critsect;
  BYTE       m_critsectData[0x18];

 public:
  BOOL Enter();
  void Leave();
};

class SEvent : public SSyncObject {
 public:
  SEvent(int manualReset = 0, int initialValue = 0);
  ~SEvent() {
  }
  SEvent &operator=(const SEvent &rhs);
  BOOL    Set();
  BOOL    Reset();
};

class SSemaphore : public SSyncObject {
 public:
  SSemaphore(UINT initialCount, UINT maximumCount);
  ~SSemaphore() {
  }
  SSemaphore &operator=(const SSemaphore &rhs);
  BOOL        Signal(UINT count);
};

class SMutex : public SSyncObject {
 public:
  SMutex();
  SMutex(int initialOwner, LPCSTR name);
  SMutex(LPCSTR name);
  ~SMutex();
  SMutex &operator=(const SMutex &rhs);
  void    Create(int initialOwner, LPCSTR name);
  void    Open(LPCSTR name);
  int     Release();
};

DWORD WaitMultiple(UINT count, SSyncObject *objects, int waitAll, DWORD timeoutMs);
DWORD WaitMultiplePtr(UINT count, SSyncObject **objectPtrs, int waitAll, DWORD timeoutMs);
void  SServerInitialize();
void  SServerDestroy();
int   STryEnterCriticalSection(LPVOID opaqueData);

typedef UINT(APIENTRY *STHREADPROC)(LPVOID);

class SThread : public SSyncObject {
 public:
  SThread() {
  }
  ~SThread() {
  }

  SThread    &operator=(const SThread &rhs);
  static BOOL Create(STHREADPROC proc, LPVOID param, SThread &thread, char *name);
};

LPVOID SCreateThread(DWORD stackSize, STHREADPROC proc, LPVOID param, DWORD flags, UINT *threadId, char *name);
LPVOID SCreateThread(STHREADPROC lpStartAddress, LPVOID lpParameter, UINT *lpThreadId, LPVOID linuxData, char *threadName);
DWORD  SGetCurrentThreadId();
int    SGetCurrentThreadPriority();
void   SSetCurrentThreadPriority(int priority);

// --------------------------------
// Unicode functions
// --------------------------------

extern "C" int APIENTRY   SUniConvertUTF16to8Len(const WORD *src, DWORD srcMaxChars, DWORD *srcChars);
extern "C" int APIENTRY   SUniConvertUTF16to8(char *dst, DWORD dstMaxChars, const WORD *src, DWORD srcMaxChars, DWORD *dstChars, DWORD *srcChars);
extern "C" int APIENTRY   SUniConvertUTF8to16Len(LPCSTR src, DWORD srcMaxChars, DWORD *srcChars);
extern "C" int APIENTRY   SUniConvertUTF8to16(WORD *dst, DWORD dstMaxChars, LPCSTR src, DWORD srcMaxChars, DWORD *dstChars, DWORD *srcChars);
extern "C" UINT APIENTRY  SUniSGetUTF8(const BYTE *strptr, int *chars);
extern "C" char *APIENTRY SUniSPutUTF8(DWORD c, char *strptr);
extern "C" int APIENTRY   SUniFindUTF8ChrStart(LPCSTR utf8String, int index);
extern "C" int APIENTRY   SUniFindAfterUTF8Chr(LPCSTR utf8String, int index);
extern "C" DWORD APIENTRY SUniConvertUTF16ToWin(char *dest, const WORD *source, DWORD destsize);
extern "C" DWORD APIENTRY SUniConvertUTF16ToMac(char *dest, const WORD *source, DWORD destsize);
extern "C" DWORD APIENTRY SUniConvertUTF16ToDos(char *dest, const WORD *source, DWORD destsize);
extern "C" DWORD APIENTRY SUniConvertWinToUTF16(WORD *dest, LPCSTR source, DWORD destsize);
extern "C" DWORD APIENTRY SUniConvertMacToUTF16(WORD *dest, LPCSTR source, DWORD destsize);
extern "C" DWORD APIENTRY SUniConvertDosToUTF16(WORD *dest, LPCSTR source, DWORD destsize);

// --------------------------------
// System functions
// --------------------------------

typedef struct OSFILETIME {
  DWORDLONG m_value;
} OSFILETIME, *LPOSFILETIME;

typedef struct OSSYSTEMTIME {
  WORD year;
  WORD month;
  WORD dayOfWeek;
  WORD day;
  WORD hour;
  WORD minute;
  WORD second;
  WORD milliseconds;
} OSSYSTEMTIME, *LPOSSYSTEMTIME;

void  OsGetSystemTime(OSSYSTEMTIME *sysTime);
void  OsGetLocalTime(OSSYSTEMTIME *sysTime);
int   OsSystemTimeCompare(const OSSYSTEMTIME *sysTime1, const OSSYSTEMTIME *sysTime2);
void  OsTimeToFileTime(DWORD time, OSFILETIME *fileTime);
void  OsFileTimeToLocalFileTime(const OSFILETIME *fileTime, OSFILETIME *localFileTime);
void  OsFileTimeToSystemTime(const OSFILETIME *fileTime, OSSYSTEMTIME *sysTime);
void  OsSystemTimeToFileTime(const OSSYSTEMTIME *sysTime, OSFILETIME *fileTime);
void  OsTimeToLocalSystemTime(DWORD time, OSSYSTEMTIME *localSysTime);
BOOL  OsDirectoryExists(LPCSTR dirName);
DWORD OsGetFileAttributes(LPCSTR fileName);
void  OsSystemObjectCreate(LPCSTR name);
BOOL  OsSystemObjectExists(LPCSTR name);

// --------------------------------
// Directory functions
// --------------------------------

typedef struct SDIRENT {
  char d_name[MAX_PATH];
} SDIRENT;

typedef struct SDIR {
  char             name[MAX_PATH];
  HANDLE           handle;
  WIN32_FIND_DATAA findData;
  SDIRENT          dirent;
} SDIR;

DECLARE_STRICT_HANDLE(HSFILE);
DECLARE_STRICT_HANDLE(HSARCHIVE);

#define SFILE_AUTH_UNABLETOAUTHENTICATE 0
#define SFILE_AUTH_NOSIGNATURE          1
#define SFILE_AUTH_BADSIGNATURE         2
#define SFILE_AUTH_UNKNOWNSIGNATURE     3
#define SFILE_AUTH_FIRSTAUTHENTIC       5
#define SFILE_AUTH_AUTHENTICBLIZZARD    5

#define SFILE_DDA_LOOP 0x00040000

#define SFILE_ERRORMODE_RETURNCODE 0
#define SFILE_ERRORMODE_CUSTOM     1
#define SFILE_ERRORMODE_FATAL      2

#define SREG_FLAG_USERSPECIFIC 0x00000001
#define SREG_FLAG_BATTLENET    0x00000002
#define SREG_FLAG_FLUSHTODISK  0x00000008
#define SREG_FLAG_MULTISZ      0x00000080

struct _TASYNCPARAMBLOCK;
struct IDirectSound;
struct IDirectSoundBuffer;
struct ZipFileFCB;
struct z_stream_s;
class ASYNCREAD;

enum SFILE_TYPE {
  SFILE_PLAIN = 0,
  SFILE_COMPRESSED = 1,
  SFILE_PAQ = 2,
  SFILE_OLD_SFILE = 3,
  SFILE_ZIP_FILE = 4
};

enum SARCHIVE_TYPE {
  SARCHIVE_MPQ = 0,
  SARCHIVE_ZIP = 1
};

class SArchive {
  friend class SFile;

 public:
  SARCHIVE_TYPE m_type;
  LPVOID        m_archive;
};

typedef struct SOVERLAPPED {
  UINT    Offset;
  SEvent *hEvent;
} SOVERLAPPED;

#include <SFile2.h>

class SFile {
  SFile(SFILE_TYPE type);
  SFile(const SFile &);
  ~SFile();
  SFile &operator=(const SFile &);

  static void          DoAsyncRead(ASYNCREAD *ptr);
  static UINT APIENTRY ReadProc(LPVOID);
  static void          InitializeReadThread();
  static void          QueueReadRequest(SFile *fileptr, LPVOID buffer, DWORD bytestoread, SOVERLAPPED *overlapped);
  static int           DoZRead(SFile *fileptr, LPVOID buffer, DWORD bytestoread, DWORD *bytesread);

  SFILE_TYPE  m_type;
  LPVOID      m_fileptr;
  SArchive   *m_archive;
  char       *m_filename;
  char       *m_actualname;
  UINT        m_size;
  BYTE       *m_zbuffer;
  z_stream_s *m_zstream;
  UINT        m_curOffset;
  SCritSect   m_lock;
  LPVOID      m_hsfile;
  ZipFileFCB *m_zipFile;
  MD5         m_md5;
  int         m_haveMD5;
  int         m_closeAfterLoad;
  UINT        m_asyncCount;

 public:
  SFILE_TYPE GetDiskType();
  DWORD      GetFileSize();
  int        GetMD5(MD5 &sum);

  static DWORD APIENTRY Open(LPCSTR filename, SFile **file);
  static DWORD APIENTRY OpenEx(SArchive *archive, LPCSTR filename, DWORD flags, SFile **file);
  static DWORD APIENTRY Close(SFile *file);
  static DWORD APIENTRY
  Read(SFile *fileptr, LPVOID buffer, DWORD bytestoread, DWORD *bytesread, SOVERLAPPED *overlapped, _TASYNCPARAMBLOCK *asyncparam);
  static DWORD APIENTRY LoadFile(LPCSTR filename, LPVOID *buffer, DWORD *bytes, DWORD extraBytes, SOVERLAPPED *overlapped);
  static DWORD APIENTRY
                      Load(SArchive *archive, LPCSTR filename, LPVOID *buffer, DWORD *bytes, DWORD extraBytes, DWORD flags, SOVERLAPPED *overlapped);
  static int APIENTRY Unload(LPVOID buffer);
  static DWORD APIENTRY    GetFileSize(SFile *file, DWORD *filesizehigh);
  static DWORD APIENTRY    SetFilePointer(SFile *file, LONG distancetomove, LONG *distancetomovehigh, DWORD movemethod);
  static int APIENTRY      GetActualFileName(SFile *file, char *buffer, DWORD bufferchars);
  static int APIENTRY      GetBasePath(char *buffer, DWORD bufferchars);
  static int APIENTRY      SetBasePath(LPCSTR path);
  static int APIENTRY      SetDataPath(LPCSTR path);
  static int APIENTRY      SetDataPathAlternate(LPCSTR path);
  static int APIENTRY      FileExists(LPCSTR filename);
  static int APIENTRY      EnableDirectAccess(DWORD access);
  static void APIENTRY     DisableSFileCheckDisk();
  static void APIENTRY     DisableSFileCritSection();
  static void APIENTRY     EnableHash(bool enable);
  static void APIENTRY     RebuildHash();
  static void              Destroy();
  static int APIENTRY      OpenArchive(LPCSTR archivename, int priority, DWORD flags, SArchive **handle);
  static int APIENTRY      CloseArchive(SArchive *archive);
  static int APIENTRY      List(SArchive *archive, int (*cb)(LPCSTR filename, LPVOID param), LPVOID param);
  static int APIENTRY      GetMD5(SFile *file, MD5 &sum);
  static void APIENTRY     CreateOverlapped(SOVERLAPPED *overlapped);
  static void APIENTRY     DestroyOverlapped(SOVERLAPPED *overlapped);
  static void APIENTRY     ResetOverlapped(SOVERLAPPED *overlapped);
  static int APIENTRY      PollOverlapped(SOVERLAPPED *overlapped);
  static void APIENTRY     WaitOverlapped(SOVERLAPPED *overlapped);
  static SDIR *APIENTRY    OpenDir(LPCSTR path);
  static SDIRENT *APIENTRY ReadDir(SDIR *dir);
  static void APIENTRY     CloseDir(SDIR *dir);
};

extern "C" DWORD APIENTRY SFileOpenFile(LPCSTR filename, HSFILE *handle);
extern "C" BOOL APIENTRY  SFileOpenFileAsArchive(HSARCHIVE ownerarchive, LPCSTR filename, int priority, DWORD flags, HSARCHIVE *handle);
extern "C" DWORD APIENTRY SFileOpenFileEx(HSARCHIVE archivehandle, LPCSTR filename, DWORD flags, HSFILE *handle);
extern "C" DWORD APIENTRY SFileFileExists(LPCSTR filename);
extern "C" DWORD APIENTRY SFileFileExistsEx(HSARCHIVE archivehandle, LPCSTR filename, DWORD flags);
extern "C" BOOL APIENTRY  SFileCloseFile(HSFILE handle);
extern "C" BOOL APIENTRY  SFileReadFile(HSFILE handle, LPVOID buffer, DWORD bytestoread, DWORD *bytesread, OVERLAPPED *overlapped);
extern "C" BOOL APIENTRY
SFileReadFileEx(HSFILE handle, LPVOID buffer, DWORD bytestoread, DWORD *bytesread, OVERLAPPED *overlapped, _TASYNCPARAMBLOCK *asyncparam);
extern "C" BOOL APIENTRY SFileReadFileEx2(
    HSFILE             handle,
    LPVOID             buffer,
    DWORD              bytestoread,
    DWORD             *bytesread,
    OVERLAPPED        *overlapped,
    LONG               overlappedpriority,
    _TASYNCPARAMBLOCK *asyncparam
);
extern "C" DWORD APIENTRY SFileGetFileSize(HSFILE handle, DWORD *filesizehigh);
extern "C" DWORD APIENTRY SFileGetFileCompressedSize(HSFILE handle, DWORD *fileSizeHigh);
extern "C" DWORD APIENTRY SFileSetFilePointer(HSFILE handle, LONG distancetomove, LONG *distancetomovehigh, DWORD movemethod);
extern "C" BOOL APIENTRY  SFileLoadFile(LPCSTR filename, LPVOID *buffer, DWORD *bytes, DWORD extraBytes, OVERLAPPED *overlapped);
extern "C" BOOL APIENTRY
SFileLoadFileEx(HSARCHIVE archive, LPCSTR filename, LPVOID *buffer, DWORD *bytes, DWORD extraBytes, DWORD flags, OVERLAPPED *overlapped);
extern "C" BOOL APIENTRY SFileLoadFileEx2(
    HSARCHIVE   archive,
    LPCSTR      filename,
    LPVOID     *buffer,
    DWORD      *bytes,
    DWORD       extraBytes,
    DWORD       flags,
    OVERLAPPED *overlapped,
    LONG        overlappedpriority
);
extern "C" BOOL APIENTRY  SFileUnloadFile(LPVOID buffer);
extern "C" BOOL APIENTRY  SFileOpenArchive(LPCSTR archivename, int priority, DWORD flags, HSARCHIVE *handle);
extern "C" BOOL APIENTRY  SFileCloseArchive(HSARCHIVE handle);
extern "C" BOOL APIENTRY  SFileGetArchiveName(HSARCHIVE archive, char *buffer, DWORD bufferchars);
extern "C" BOOL APIENTRY  SFileGetArchiveInfo(HSARCHIVE archive, int *priority, int *cdrom);
extern "C" BOOL APIENTRY  SFileGetFileArchive(HSFILE file, HSARCHIVE *archive);
extern "C" DWORD APIENTRY SFileCalcFileCrc(LPCSTR filename);
extern "C" DWORD APIENTRY SFileGetFileCrc(HSFILE handle);
extern "C" BOOL APIENTRY  SFileGetFileMD5(HSFILE handle, BYTE *md5);
extern "C" BOOL APIENTRY  SFileGetFileTime(HSFILE handle, FILETIME *filetime);
extern "C" BOOL APIENTRY  SFileGetActualFileName(HSFILE file, char *buffer, DWORD bufferchars);
extern "C" BOOL APIENTRY  SFileSetBasePath(LPCSTR path);
extern "C" BOOL APIENTRY  SFileGetBasePath(char *buffer, DWORD bufferchars);
extern "C" BOOL APIENTRY  SFileGetFileName(HSFILE file, char *buffer, DWORD bufferchars);
extern "C" void APIENTRY  SFileSetAsyncBudget(DWORD bytesPerSec);
extern "C" void APIENTRY  SFileSetDataChunkSize(DWORD bytes);
extern "C" BOOL APIENTRY  SFileSetIoErrorMode(DWORD errormode, int(APIENTRY *errorproc)(LPCSTR, DWORD, DWORD));
extern "C" void APIENTRY  SFileSetLocale(DWORD lcid);
extern "C" WORD APIENTRY  SFileGetLocale();
extern "C" void APIENTRY  SFileSetPlatform(DWORD platformId);
extern "C" void APIENTRY  SFileCancelRequest(LPVOID buffer);
extern "C" BOOL APIENTRY  SFileCancelRequestEx(LPVOID buffer);
extern "C" void APIENTRY  SFilePrioritizeRequest(LPVOID buffer, LONG overlappedpriority);
extern "C" void APIENTRY  SFileRegisterLoadNotifyProc(void(APIENTRY *f)(LPCSTR, LPVOID), LPVOID opaqueData);
extern "C" BOOL APIENTRY  SFileEnableArchive(HSARCHIVE archive, int enable);
extern "C" BOOL APIENTRY  SFileEnableDirectAccess(DWORD access);
extern "C" void APIENTRY  SFileEnableSeekOptimization(int enable);
extern "C" BOOL APIENTRY  SFileAuthenticateArchive(HSARCHIVE handle, DWORD *extendedresult);
extern "C" BOOL APIENTRY
SFileAuthenticateArchiveEx(HSARCHIVE handle, DWORD *extendedresult, const BYTE *modulus, DWORD modulusSize, const BYTE *exponent, DWORD exponentSize);
extern "C" BOOL APIENTRY SFileDdaInitialize(IDirectSound *directsound);
extern "C" BOOL APIENTRY SFileDdaDestroy();
extern "C" BOOL APIENTRY SFileDestroy();
extern "C" BOOL APIENTRY SFileDdaBegin(HSFILE handle, DWORD buffersize, DWORD flags);
extern "C" BOOL APIENTRY
SFileDdaBeginEx(HSFILE handle, DWORD buffersize, DWORD flags, DWORD offset, LONG volume, LONG pan, IDirectSoundBuffer *soundbuffer);
extern "C" BOOL APIENTRY SFileDdaEnd(HSFILE handle);
extern "C" BOOL APIENTRY SFileDdaGetPos(HSFILE handle, DWORD *position, DWORD *maxposition);
extern "C" BOOL APIENTRY SFileDdaGetVolume(HSFILE handle, LONG *volume, LONG *pan);
extern "C" BOOL APIENTRY SFileDdaSetVolume(HSFILE handle, LONG volume, LONG pan);
extern "C" void APIENTRY SFileLoadDump();
extern "C" void APIENTRY SFileArchiveDump();

// --------------------------------
// Log functions
// --------------------------------

DECLARE_STRICT_HANDLE(HSLOG);

extern "C" void APIENTRY SLogClose(HSLOG log);
extern "C" BOOL APIENTRY SLogCreate(LPCSTR filename, DWORD flags, HSLOG *log);
extern "C" void APIENTRY SLogDestroy();
extern "C" void APIENTRY SLogDump(HSLOG log, LPCVOID data, DWORD bytes);
extern "C" void APIENTRY SLogFlush(HSLOG log);
extern "C" void APIENTRY SLogFlushAll();
extern "C" void APIENTRY SLogGetDefaultDirectory(char *dirname, DWORD dirnamesize);
extern "C" void APIENTRY SLogInitialize();
extern "C" BOOL APIENTRY SLogIsInitialized();
extern "C" void __cdecl  SLogPend(HSLOG log, LPCSTR format, ...);
extern "C" long APIENTRY SLogSetAbsIndent(HSLOG log, long indent);
extern "C" void APIENTRY SLogSetDefaultDirectory(LPCSTR dirname);
extern "C" long APIENTRY SLogSetIndent(HSLOG log, long deltaIndent);
extern "C" void APIENTRY SLogSetTimestamp(HSLOG log, BOOL timeStamp);
extern "C" void APIENTRY SLogVWrite(HSLOG log, LPCSTR format, char *arglist);
extern "C" void __cdecl  SLogWrite(HSLOG log, LPCSTR format, ...);

// --------------------------------
// Region functions
// --------------------------------

DECLARE_STRICT_HANDLE(HSRGN);

typedef struct RECTF {
  float left;
  float bottom;
  float right;
  float top;
} RECTF, *LPRECTF;

extern "C" void APIENTRY SRgnClear(HSRGN handle);
extern "C" void APIENTRY SRgnCombineRectf(HSRGN handle, const RECTF *rect, LPVOID param, int combinemode);
extern "C" void APIENTRY SRgnCombineRecti(HSRGN handle, const RECT *rect, LPVOID param, int combinemode);
extern "C" void APIENTRY SRgnCreate(HSRGN *handle, DWORD reserved);
extern "C" void APIENTRY SRgnDelete(HSRGN handle);
extern "C" void APIENTRY SRgnDestroy();
extern "C" void APIENTRY SRgnDuplicate(HSRGN orighandle, HSRGN *handle, DWORD reserved);
extern "C" void APIENTRY SRgnGetBoundingRectf(HSRGN handle, RECTF *rect);
extern "C" void APIENTRY SRgnGetBoundingRecti(HSRGN handle, RECT *rect);
extern "C" void APIENTRY SRgnGetRectParamsf(HSRGN handle, const RECTF *rect, DWORD *numparams, LPVOID *buffer);
extern "C" void APIENTRY SRgnGetRectParamsi(HSRGN handle, const RECT *rect, DWORD *numparams, LPVOID *buffer);
extern "C" void APIENTRY SRgnGetRectsf(HSRGN handle, DWORD *numrects, RECTF *buffer);
extern "C" void APIENTRY SRgnGetRectsi(HSRGN handle, DWORD *numrects, RECT *buffer);
extern "C" BOOL APIENTRY SRgnIsPointInRegionf(HSRGN handle, float x, float y);
extern "C" BOOL APIENTRY SRgnIsPointInRegioni(HSRGN handle, int x, int y);
extern "C" BOOL APIENTRY SRgnIsRectInRegionf(HSRGN handle, const RECTF *rect);
extern "C" BOOL APIENTRY SRgnIsRectInRegioni(HSRGN handle, const RECT *rect);
extern "C" void APIENTRY SRgnOffsetf(HSRGN handle, float xoffset, float yoffset);
extern "C" void APIENTRY SRgnOffseti(HSRGN handle, int xoffset, int yoffset);

class CSRgn {
 private:
  HSRGN m_handle;

  void CopyConstructor(const CSRgn &copy);

 public:
  CSRgn();
  CSRgn(const CSRgn &copy);
  ~CSRgn();
  CSRgn &operator=(const CSRgn &copy);
  void   AddParamf(const RECTF *rect, LPVOID param);
  void   AddParami(const RECT *rect, LPVOID param);
  void   AddRectf(const RECTF *rect, LPVOID param);
  void   AddRecti(const RECT *rect, LPVOID param);
  void   Clear();
  void   CombineRectf(const RECTF *rect, LPVOID param, int combinemode);
  void   CombineRecti(const RECT *rect, LPVOID param, int combinemode);
  void   GetBoundingRectf(RECTF *rect);
  void   GetBoundingRecti(RECT *rect);
  void   GetRectsf(DWORD *numrects, RECTF *buffer);
  void   GetRectsi(DWORD *numrects, RECT *buffer);
  void   GetRectParamsf(const RECTF *rect, DWORD *numparams, LPVOID *buffer);
  void   GetRectParamsi(const RECT *rect, DWORD *numparams, LPVOID *buffer);
  BOOL   IsPointInRegionf(float x, float y);
  BOOL   IsPointInRegioni(int x, int y);
  BOOL   IsRectInRegionf(const RECTF *rect);
  BOOL   IsRectInRegioni(const RECT *rect);
  void   Offsetf(float xoffset, float yoffset);
  void   Offseti(int xoffset, int yoffset);
};

// --------------------------------
// Storm core functions
// --------------------------------

typedef struct _STORMOPTIONS {
  int   smemleaksilentwarning;
  int   serrleaksilentwarning;
  DWORD wavechunksize;
  int   alignstreamingwavedata;
  int   echotooutputdebugstring;
  int   serrsuppresslogs;
  int   crcenabled;
  int   orderedprintfenabled;
} STORMOPTIONS, *LPSTORMOPTIONS;

extern STORMOPTIONS g_opt;

extern "C" void APIENTRY      StormInitialize();
extern "C" void APIENTRY      StormDestroy();
extern "C" HINSTANCE APIENTRY StormGetInstance();
extern "C" BOOL APIENTRY      StormGetOption(int optname, LPVOID optval, LPDWORD optlen);
extern "C" BOOL APIENTRY      StormSetOption(int optname, LPVOID optval, DWORD optlen);
extern "C" int __cdecl        StormCallService(int service, ...);
void                          StormRtlInitialize();
void                          StormRtlDestroy();
void                          IncrementAllocCount();
void                          IncrementFreeCount();

namespace STypeCache {
  void   Shutdown();
  void   Grow();
  int    GetProbe(LPCSTR rawname);
  LPCSTR Get(LPCSTR rawname);
  LPCSTR Set(LPCSTR rawname, LPCSTR decname);
}

// --------------------------------
// Registry functions
// --------------------------------

extern "C" BOOL APIENTRY SRegDeleteValue(LPCSTR keyname, LPCSTR valuename, UINT flags);
extern "C" BOOL APIENTRY SRegDeleteKey(LPCSTR keyname, UINT flags);
extern "C" BOOL APIENTRY SRegGetBaseKey(UINT flags, char *buffer, UINT bufferchars);
extern "C" BOOL APIENTRY SRegLoadData(LPCSTR keyname, LPCSTR valuename, UINT flags, LPVOID buffer, DWORD buffersize, LPDWORD bytesread);
extern "C" BOOL APIENTRY SRegLoadString(LPCSTR keyname, LPCSTR valuename, UINT flags, char *buffer, DWORD buffersize);
extern "C" BOOL APIENTRY SRegLoadValue(LPCSTR keyname, LPCSTR valuename, UINT flags, LPDWORD value);
extern "C" BOOL APIENTRY SRegSaveData(LPCSTR keyname, LPCSTR valuename, UINT flags, LPCVOID data, DWORD databytes);
extern "C" BOOL APIENTRY SRegSaveString(LPCSTR keyname, LPCSTR valuename, UINT flags, LPCSTR string);
extern "C" BOOL APIENTRY SRegSaveValue(LPCSTR keyname, LPCSTR valuename, UINT flags, DWORD value);
extern "C" BOOL APIENTRY SRegEnumKey(LPCSTR baseKeyName, UINT flags, UINT subKeyIndex, char *keyNameBuffer, UINT bufferChars);
extern "C" BOOL APIENTRY SRegGetNumSubKeys(LPCSTR keyName, UINT flags, UINT *numSubKeys);

// --------------------------------
// String functions
// --------------------------------

extern "C" void APIENTRY SStrInitialize();
extern "C" void APIENTRY SStrDestroy();

LPCSTR            SStrChr(LPCSTR string, char ch);
char             *SStrChr(char *string, char ch);
LPCSTR            SStrChrR(LPCSTR string, char ch);
char             *SStrChrR(char *string, char ch);
int APIENTRY      SStrCmp(LPCSTR string1, LPCSTR string2, DWORD maxchars);
int APIENTRY      SStrCmpI(LPCSTR string1, LPCSTR string2, DWORD maxchars);
DWORD APIENTRY    SStrCopy(char *dest, LPCSTR source, DWORD destsize);
char *APIENTRY    SStrDupA(LPCSTR string, LPCSTR fileName, UINT lineNumber);
DWORD APIENTRY    SStrLen(LPCSTR string);
DWORD APIENTRY    SStrLen(const WORD *string);
DWORD APIENTRY    SStrPack(char *dest, LPCSTR source, DWORD destsize);
DWORD __cdecl     SStrPrintf(char *dest, DWORD maxchars, LPCSTR format, ...);
DWORD __cdecl     SStrVPrintf(char *dest, DWORD maxchars, LPCSTR format, char *arglist);
double APIENTRY   SStrToDouble(LPCSTR string);
float APIENTRY    SStrToFloat(LPCSTR string);
int APIENTRY      SStrToInt(LPCSTR string);
LONGLONG APIENTRY SStrToInt64(LPCSTR string);
UINT APIENTRY     SStrToUnsigned(LPCSTR string);
void APIENTRY     SStrTokenize(LPCSTR *string, char *buffer, DWORD bufferchars, LPCSTR whitespace, int *quoted);
DWORD APIENTRY    SStrHash(LPCSTR string, DWORD flags, DWORD seed);
LONGLONG APIENTRY SStrHash64(LPCSTR string, DWORD flags, LONGLONG seed);
DWORD APIENTRY    SStrHashHT(LPCSTR string);
void APIENTRY     SStrUpper(char *string);
void APIENTRY     SStrLower(char *string);
LPCSTR            SStrStr(LPCSTR string, LPCSTR search);
char             *SStrStr(char *string, LPCSTR search);
LPCSTR            SStrStrI(LPCSTR string, LPCSTR search);
char             *SStrStrI(char *string, LPCSTR search);

#define ALLOC(bytes)     SMemAlloc(bytes, __FILE__, __LINE__, 0)
#define ALLOCZERO(bytes) SMemAlloc(bytes, __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY)
#define DEL(ptr)         delete (ptr)
#define DELIFUSED(ptr) \
  do                   \
    if (ptr)           \
      delete ptr;      \
  while (0)
#define FREE(ptr) SMemFree(ptr, __FILE__, __LINE__, 0)
#define FREEIFUSED(ptr)                     \
  do                                        \
    if (ptr)                                \
      SMemFree(ptr, __FILE__, __LINE__, 0); \
  while (0)
#define NEW(struct)     new (SMemAlloc(sizeof(struct), __FILE__, __LINE__, 0)) struct
#define NEWZERO(struct) (new (SMemAlloc(sizeof(struct), __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY)) struct)
