#include <storm.h>

#include <math.h>
#include <CoreServices/CoreServices.h>

enum {
  FASTTIMING_NONE = 0,
  FASTTIMING_MICROSECONDS = 1,
  FASTTIMING_UPTIME = 2,
};

static UInt32 s_upTimeToMilliseconds[2];
static UInt32 s_upTimeToMicroseconds[2];
static int    s_fastTimingMode;

bool InitFastTiming();

inline void RawMicroseconds(DWORD *hi, DWORD *lo) {
  switch (s_fastTimingMode) {
    case FASTTIMING_NONE:
      InitFastTiming();
      RawMicroseconds(hi, lo);
      break;

    case FASTTIMING_MICROSECONDS: {
      UnsignedWide micro;
      Microseconds(&micro);
      *hi = micro.hi;
      *lo = micro.lo;
      break;
    }

    case FASTTIMING_UPTIME: {
      AbsoluteTime upTime = UpTime();
      UInt64 low = (UInt64)upTime.lo * s_upTimeToMicroseconds[0];
      UInt64 high = (UInt64)upTime.hi * s_upTimeToMicroseconds[0] + (low >> 32);
      UInt64 micro = high * s_upTimeToMicroseconds[1] + (((low & 0xFFFFFFFF) * s_upTimeToMicroseconds[1]) >> 32);
      *hi = (UInt32)(micro >> 32);
      *lo = (UInt32)micro;
      break;
    }

    default:
      ASSERT(!"unknown FastTiming mode");
      *hi = 0;
      *lo = 0;
      break;
  }
}

inline void RawMilliseconds(DWORD *hi, DWORD *lo) {
  switch (s_fastTimingMode) {
    case FASTTIMING_NONE:
      InitFastTiming();
      RawMicroseconds(hi, lo);
      break;

    case FASTTIMING_MICROSECONDS: {
      UnsignedWide micro;
      Microseconds(&micro);
      *hi = micro.hi;
      *lo = micro.lo;
      break;
    }

    case FASTTIMING_UPTIME: {
      AbsoluteTime upTime = UpTime();
      UInt64 low = (UInt64)upTime.lo * s_upTimeToMilliseconds[0];
      UInt64 high = (UInt64)upTime.hi * s_upTimeToMilliseconds[0] + (low >> 32);
      UInt64 milli = high * s_upTimeToMilliseconds[1] + (((low & 0xFFFFFFFF) * s_upTimeToMilliseconds[1]) >> 32);
      *hi = (UInt32)(milli >> 32);
      *lo = (UInt32)milli;
      return;
    }

    default:
      ASSERT(!"unknown FastTiming mode");
      *hi = 0;
      *lo = 0;
      break;
  }

  *lo = (UInt32)(*hi * 4294967) + (UInt32)(((UInt64)*lo * 4294967) >> 32);
  *hi = (UInt32)*hi / 1000;
}

extern "C" void FastMilliseconds(DWORD *hi, DWORD *lo) {
  RawMilliseconds(hi, lo);
}

extern "C" void FastMicroseconds(DWORD *hi, DWORD *lo) {
  RawMicroseconds(hi, lo);
}

bool InitFastTiming_UpTime() {
  AbsoluteTime absolute;
  absolute.hi = 1;
  absolute.lo = 0;
  Nanoseconds nano = AbsoluteToNanoseconds(absolute);

  double nanoPerAbsolute = (double)(((UInt64)nano.hi << 32) | nano.lo) / (double)(((UInt64)absolute.hi << 32) | absolute.lo);
  double fixedOne = pow(2.0, 32.0);
  double micro = nanoPerAbsolute / 1000.0 * fixedOne;
  double milli = nanoPerAbsolute / 1000000.0 * fixedOne;

  if (micro >= fixedOne) {
    s_upTimeToMicroseconds[0] = (UInt32)sqrt(micro);
    s_upTimeToMicroseconds[1] = s_upTimeToMicroseconds[0];
  } else {
    s_upTimeToMicroseconds[0] = (UInt32)micro;
    s_upTimeToMicroseconds[1] = 1;
  }

  if (milli >= fixedOne) {
    s_upTimeToMilliseconds[0] = (UInt32)sqrt(milli);
    s_upTimeToMilliseconds[1] = s_upTimeToMilliseconds[0];
  } else {
    s_upTimeToMilliseconds[0] = (UInt32)milli;
    s_upTimeToMilliseconds[1] = 1;
  }

  return true;
}

bool InitFastTiming() {
  if (s_fastTimingMode) {
    return true;
  }

  bool result = false;
  if (InitFastTiming_UpTime()) {
    s_fastTimingMode = FASTTIMING_UPTIME;
    result = true;
  }
  if (!result) {
    s_fastTimingMode = FASTTIMING_MICROSECONDS;
    result = true;
  }

  DWORD hi;
  DWORD lo;
  FastMicroseconds(&hi, &lo);
  FastMilliseconds(&hi, &lo);

  return result;
}

void FPMicroseconds(UnsignedWide *wide) {
  DWORD hi;
  DWORD lo;

  FastMicroseconds(&hi, &lo);
  wide->hi = hi;
  wide->lo = lo;
}
