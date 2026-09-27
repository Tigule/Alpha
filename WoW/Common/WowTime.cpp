#include "WowTime.h"

#include <storm.h>

#include <string.h>
#include <time.h>

static LPCSTR s_WeekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

WowTime::WowTime() : m_minute(-1), m_hour(-1), m_weekday(-1), m_monthDay(-1), m_month(-1), m_year(-1), m_flags(0) {
}

int WowTime::GetDaysSinceEpoch() const {
  if (m_year < 0 || m_month < 0 || m_monthDay < 0) {
    return 0;
  }

  struct tm date;
  memset(&date, 0, sizeof(date));
  date.tm_year = m_year + 100;
  date.tm_mon = m_month;
  date.tm_mday = m_monthDay + 1;
  date.tm_isdst = -1;

  return mktime(&date) / 86400;
}

void WowTime::SetDaysSinceEpoch(int days) {
  time_t     seconds = 86400 * days + 43200;
  struct tm *date = localtime(&seconds);

  if (date) {
    m_year = date->tm_year - 100;
    m_month = date->tm_mon;
    m_monthDay = date->tm_mday - 1;
    m_weekday = date->tm_wday;
  }
}

int WowTime::GetHourAndMinutes() const {
  if (m_hour < 0 || m_minute < 0) {
    return 0;
  }

  return m_hour * 60 + m_minute;
}

void WowTime::SetHourAndMinutes(int minutes) {
  m_hour = minutes / 60;
  m_minute = minutes % 60;
}

int WowTime::CompareYear(const WowTime &compareTime) const {
  return m_year > compareTime.m_year ? 1 : m_year < compareTime.m_year ? -1 : 0;
}

int WowTime::CompareMonth(const WowTime &compareTime) const {
  return m_month > compareTime.m_month ? 1 : m_month < compareTime.m_month ? -1 : 0;
}

int WowTime::CompareDay(const WowTime &compareTime) const {
  return m_monthDay > compareTime.m_monthDay ? 1 : m_monthDay < compareTime.m_monthDay ? -1 : 0;
}

int WowTime::CompareWeekday(const WowTime &compareTime) const {
  return m_weekday > compareTime.m_weekday ? 1 : m_weekday < compareTime.m_weekday ? -1 : 0;
}

int WowTime::CompareHour(const WowTime &compareTime) const {
  return m_hour > compareTime.m_hour ? 1 : m_hour < compareTime.m_hour ? -1 : 0;
}

int WowTime::CompareMinute(const WowTime &compareTime) const {
  return m_minute > compareTime.m_minute ? 1 : m_minute < compareTime.m_minute ? -1 : 0;
}

bool WowTime::InRange(const WowTime &valMin, const WowTime &valMax) const {
  if (valMin <= valMax) {
    if (*this >= valMin && *this < valMax) {
      return true;
    }
  } else if (*this >= valMin || *this < valMax) {
    return true;
  }
  return false;
}

bool WowTime::operator<(const WowTime &cmpTime) const {
  if (&cmpTime == this) {
    return false;
  }

  int result;
  if (cmpTime.m_year >= 0 && m_year >= 0) {
    result = CompareYear(cmpTime);
    if (result) {
      return result < 0;
    }
  }
  if (cmpTime.m_month >= 0 && m_month >= 0) {
    result = CompareMonth(cmpTime);
    if (result) {
      return result < 0;
    }
  }
  if (cmpTime.m_monthDay >= 0 && m_monthDay >= 0) {
    result = CompareDay(cmpTime);
    if (result) {
      return result < 0;
    }
  }
  if (cmpTime.m_weekday >= 0 && m_weekday >= 0) {
    result = CompareWeekday(cmpTime);
    if (result) {
      return result < 0;
    }
  }
  if (cmpTime.m_hour >= 0 && m_hour >= 0) {
    result = CompareHour(cmpTime);
    if (result) {
      return result < 0;
    }
  }
  if (cmpTime.m_minute >= 0 && m_minute >= 0) {
    result = CompareMinute(cmpTime);
    if (result) {
      return result < 0;
    }
  }
  return false;
}

bool WowTime::operator<=(const WowTime &cmpTime) const {
  return *this == cmpTime || *this < cmpTime;
}

bool WowTime::operator>(const WowTime &cmpTime) const {
  if (&cmpTime == this) {
    return false;
  }

  int result;
  if (cmpTime.m_year >= 0 && m_year >= 0) {
    result = CompareYear(cmpTime);
    if (result) {
      return result > 0;
    }
  }
  if (cmpTime.m_month >= 0 && m_month >= 0) {
    result = CompareMonth(cmpTime);
    if (result) {
      return result > 0;
    }
  }
  if (cmpTime.m_monthDay >= 0 && m_monthDay >= 0) {
    result = CompareDay(cmpTime);
    if (result) {
      return result > 0;
    }
  }
  if (cmpTime.m_weekday >= 0 && m_weekday >= 0) {
    result = CompareWeekday(cmpTime);
    if (result) {
      return result > 0;
    }
  }
  if (cmpTime.m_hour >= 0 && m_hour >= 0) {
    result = CompareHour(cmpTime);
    if (result) {
      return result > 0;
    }
  }
  if (cmpTime.m_minute >= 0 && m_minute >= 0) {
    result = CompareMinute(cmpTime);
    if (result) {
      return result > 0;
    }
  }
  return false;
}

bool WowTime::operator>=(const WowTime &cmpTime) const {
  return *this == cmpTime || *this > cmpTime;
}

bool WowTime::operator==(const WowTime &cmpTime) const {
  if (cmpTime.m_year > 0 && m_year > 0 && cmpTime.m_year != m_year) {
    return false;
  }
  if (cmpTime.m_month > 0 && m_month > 0 && cmpTime.m_month != m_month) {
    return false;
  }
  if (cmpTime.m_monthDay > 0 && m_monthDay > 0 && cmpTime.m_monthDay != m_monthDay) {
    return false;
  }
  if (cmpTime.m_weekday > 0 && m_weekday > 0 && cmpTime.m_weekday != m_weekday) {
    return false;
  }
  if (cmpTime.m_hour > 0 && m_hour > 0 && cmpTime.m_hour != m_hour) {
    return false;
  }
  if (cmpTime.m_minute > 0 && m_minute > 0 && cmpTime.m_minute != m_minute) {
    return false;
  }
  return true;
}

bool WowTime::operator!=(const WowTime &cmpTime) const {
  return !(*this == cmpTime);
}

void WowTime::WowEncodeTime(UINT &value, int minute, int hour, int weekday, int monthday, int month, int year, int flags) {
  ASSERT(minute == -1 || (minute >= 0 && minute < 60));
  ASSERT(hour == -1 || (hour >= 0 && hour < 24));
  ASSERT(weekday == -1 || (weekday >= 0 && weekday < 7));
  ASSERT(monthday == -1 || (monthday >= 0 && monthday < 32));
  ASSERT(month == -1 || (month >= 0 && month < 12));
  ASSERT(year == -1 || year >= 0 && year <= ((1<<5)-1));
  ASSERT(flags >= 0 && flags <= ((1<<2)-1));

  value = (minute & ((1 << 6) - 1)) | ((hour & ((1 << 5) - 1)) << 6) | ((weekday & ((1 << 3) - 1)) << 11) | ((monthday & ((1 << 6) - 1)) << 14) |
          ((month & ((1 << 4) - 1)) << 20) | ((year & ((1 << 5) - 1)) << 24) | ((flags & ((1 << 2) - 1)) << 29);
}

void WowTime::WowDecodeTime(UINT value, int *minute, int *hour, int *weekday, int *monthday, int *month, int *year, int *flags) {
  int decoded;

  if (minute) {
    decoded = value & ((1 << 6) - 1);
    if (decoded == ((1 << 6) - 1)) {
      *minute = -1;
    } else {
      *minute = decoded;
    }
  }
  if (hour) {
    decoded = (value >> 6) & ((1 << 5) - 1);
    if (decoded == ((1 << 5) - 1)) {
      *hour = -1;
    } else {
      *hour = decoded;
    }
  }
  if (weekday) {
    decoded = (value >> 11) & ((1 << 3) - 1);
    if (decoded == ((1 << 3) - 1)) {
      *weekday = -1;
    } else {
      *weekday = decoded;
    }
  }
  if (monthday) {
    decoded = (value >> 14) & ((1 << 6) - 1);
    if (decoded == ((1 << 6) - 1)) {
      *monthday = -1;
    } else {
      *monthday = decoded;
    }
  }
  if (month) {
    decoded = (value >> 20) & ((1 << 4) - 1);
    if (decoded == ((1 << 4) - 1)) {
      *month = -1;
    } else {
      *month = decoded;
    }
  }
  if (year) {
    decoded = (value >> 24) & ((1 << 5) - 1);
    if (decoded == ((1 << 5) - 1)) {
      *year = -1;
    } else {
      *year = decoded;
    }
  }
  if (flags) {
    decoded = (value >> 29) & ((1 << 2) - 1);
    if (decoded == ((1 << 2) - 1)) {
      *flags = -1;
    } else {
      *flags = decoded;
    }
  }
}

void WowTime::WowEncodeTime(UINT &value, const WowTime *time) {
  WowEncodeTime(value, time->m_minute, time->m_hour, time->m_weekday, time->m_monthDay, time->m_month, time->m_year, time->m_flags);
}

void WowTime::WowDecodeTime(UINT value, WowTime *time) {
  WowDecodeTime(value, &time->m_minute, &time->m_hour, &time->m_weekday, &time->m_monthDay, &time->m_month, &time->m_year, &time->m_flags);
}

LPCSTR WowTime::WowGetTimeString(UINT value, char *string, int maxlen) {
  WowTime time(value);
  return WowGetTimeString(&time, string, maxlen);
}

LPCSTR WowTime::WowGetTimeString(WowTime *time, char *string, int maxlen) {
  {
    UINT value;
    WowEncodeTime(value, time);
    if (!value) {
      SStrPrintf(string, maxlen, "Not Set");
      return string;
    }
  }

  char buffMonth[8];
  char buffmonthDay[8];
  char buffYear[8];
  char buffWeekDay[8];
  char buffHour[8];
  char buffMinute[8];

  if (time->m_year < 0) {
    SStrPrintf(buffYear, sizeof(buffYear), "A");
  } else {
    SStrPrintf(buffYear, sizeof(buffYear), "%i", time->m_year + 2000);
  }

  if (time->m_month < 0) {
    SStrPrintf(buffMonth, sizeof(buffMonth), "A");
  } else {
    SStrPrintf(buffMonth, sizeof(buffMonth), "%i", time->m_month + 1);
  }

  if (time->m_monthDay < 0) {
    SStrPrintf(buffmonthDay, sizeof(buffmonthDay), "A");
  } else {
    SStrPrintf(buffmonthDay, sizeof(buffmonthDay), "%i", time->m_monthDay + 1);
  }

  if (time->m_weekday < 0) {
    SStrPrintf(buffWeekDay, sizeof(buffWeekDay), "Any");
  } else {
    SStrPrintf(buffWeekDay, sizeof(buffWeekDay), s_WeekDays[time->m_weekday]);
  }

  if (time->m_hour < 0) {
    SStrPrintf(buffHour, sizeof(buffHour), "A");
  } else {
    SStrPrintf(buffHour, sizeof(buffHour), "%i", time->m_hour);
  }

  if (time->m_minute < 0) {
    SStrPrintf(buffMinute, sizeof(buffMinute), "A");
  } else {
    SStrPrintf(buffMinute, sizeof(buffMinute), "%2.2i", time->m_minute);
  }

  SStrPrintf(string, maxlen, "%s/%s/%s (%s) %s:%s", buffMonth, buffmonthDay, buffYear, buffWeekDay, buffHour, buffMinute);
  return string;
}
