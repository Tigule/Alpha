#include "WowTime.h"

#include <storm.h>

#include <string.h>
#include <time.h>

static const char *s_WeekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

WowTime::WowTime() : m_minute(-1), m_hour(-1), m_weekday(-1), m_monthDay(-1), m_month(-1), m_year(-1), m_flags(0) {
}

int WowTime::GetDaysSinceEpoch() const {
  if (m_year < 0) {
    return 0;
  }
  if (m_month < 0) {
    return 0;
  }
  if (m_monthDay < 0) {
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
  if (m_hour < 0) {
    return 0;
  }
  if (m_minute < 0) {
    return 0;
  }

  return m_hour * 60 + m_minute;
}

void WowTime::SetHourAndMinutes(int minutes) {
  m_hour = minutes / 60;
  m_minute = minutes % 60;
}

int WowTime::CompareYear(const WowTime &compareTime) const {
  return m_year < compareTime.m_year ? -1 : m_year > compareTime.m_year;
}

int WowTime::CompareMonth(const WowTime &compareTime) const {
  return m_month < compareTime.m_month ? -1 : m_month > compareTime.m_month;
}

int WowTime::CompareDay(const WowTime &compareTime) const {
  return m_monthDay < compareTime.m_monthDay ? -1 : m_monthDay > compareTime.m_monthDay;
}

int WowTime::CompareWeekday(const WowTime &compareTime) const {
  return m_weekday < compareTime.m_weekday ? -1 : m_weekday > compareTime.m_weekday;
}

int WowTime::CompareHour(const WowTime &compareTime) const {
  return m_hour < compareTime.m_hour ? -1 : m_hour > compareTime.m_hour;
}

int WowTime::CompareMinute(const WowTime &compareTime) const {
  return m_minute < compareTime.m_minute ? -1 : m_minute > compareTime.m_minute;
}

bool WowTime::InRange(const WowTime &valMin, const WowTime &valMax) const {
  if (valMin <= valMax) {
    return *this >= valMin && *this < valMax;
  }
  return *this >= valMin || *this < valMax;
}

bool WowTime::operator<(const WowTime &cmpTime) const {
  if (&cmpTime == this) {
    return false;
  }

  if (cmpTime.m_year >= 0 && m_year >= 0 && CompareYear(cmpTime)) {
    return CompareYear(cmpTime) < 0;
  }
  if (cmpTime.m_month >= 0 && m_month >= 0 && CompareMonth(cmpTime)) {
    return CompareMonth(cmpTime) < 0;
  }
  if (cmpTime.m_monthDay >= 0 && m_monthDay >= 0 && CompareDay(cmpTime)) {
    return CompareDay(cmpTime) < 0;
  }
  if (cmpTime.m_weekday >= 0 && m_weekday >= 0 && CompareWeekday(cmpTime)) {
    return CompareWeekday(cmpTime) < 0;
  }
  if (cmpTime.m_hour >= 0 && m_hour >= 0 && CompareHour(cmpTime)) {
    return CompareHour(cmpTime) < 0;
  }
  return cmpTime.m_minute >= 0 && m_minute >= 0 && CompareMinute(cmpTime) < 0;
}

bool WowTime::operator<=(const WowTime &cmpTime) const {
  return *this == cmpTime || *this < cmpTime;
}

bool WowTime::operator>(const WowTime &cmpTime) const {
  return cmpTime < *this;
}

bool WowTime::operator>=(const WowTime &cmpTime) const {
  return *this == cmpTime || *this > cmpTime;
}

bool WowTime::operator==(const WowTime &cmpTime) const {
  if (&cmpTime == this) {
    return true;
  }
  if (cmpTime.m_year >= 0 && m_year >= 0 && CompareYear(cmpTime)) {
    return false;
  }
  if (cmpTime.m_month >= 0 && m_month >= 0 && CompareMonth(cmpTime)) {
    return false;
  }
  if (cmpTime.m_monthDay >= 0 && m_monthDay >= 0 && CompareDay(cmpTime)) {
    return false;
  }
  if (cmpTime.m_weekday >= 0 && m_weekday >= 0 && CompareWeekday(cmpTime)) {
    return false;
  }
  if (cmpTime.m_hour >= 0 && m_hour >= 0 && CompareHour(cmpTime)) {
    return false;
  }
  return cmpTime.m_minute < 0 || m_minute < 0 || !CompareMinute(cmpTime);
}

bool WowTime::operator!=(const WowTime &cmpTime) const {
  return !(*this == cmpTime);
}

void WowTime::WowEncodeTime(unsigned int &value, int minute, int hour, int weekday, int monthday, int month, int year, int flags) {
  ASSERT(minute == -1 || (minute >= 0 && minute < 60));
  ASSERT(hour == -1 || (hour >= 0 && hour < 24));
  ASSERT(weekday == -1 || (weekday >= 0 && weekday < 7));
  ASSERT(monthday == -1 || (monthday >= 0 && monthday < 32));
  ASSERT(month == -1 || (month >= 0 && month < 12));
  ASSERT(year == -1 || year >= 0 && year <= ((1 << 5) - 1));
  ASSERT(flags >= 0 && flags <= ((1 << 2) - 1));

  value = (minute & ((1 << 6) - 1)) | ((hour & ((1 << 5) - 1)) << 6) | ((weekday & ((1 << 3) - 1)) << 11) | ((monthday & ((1 << 6) - 1)) << 14) |
          ((month & ((1 << 4) - 1)) << 20) | ((year & ((1 << 5) - 1)) << 24) | ((flags & ((1 << 2) - 1)) << 29);
}

void WowTime::WowDecodeTime(unsigned int value, int *minute, int *hour, int *weekday, int *monthday, int *month, int *year, int *flags) {
  int decoded;

  if (minute) {
    decoded = value & ((1 << 6) - 1);
    *minute = decoded == ((1 << 6) - 1) ? -1 : decoded;
  }
  if (hour) {
    decoded = (value >> 6) & ((1 << 5) - 1);
    *hour = decoded == ((1 << 5) - 1) ? -1 : decoded;
  }
  if (weekday) {
    decoded = (value >> 11) & ((1 << 3) - 1);
    *weekday = decoded == ((1 << 3) - 1) ? -1 : decoded;
  }
  if (monthday) {
    decoded = (value >> 14) & ((1 << 6) - 1);
    *monthday = decoded == ((1 << 6) - 1) ? -1 : decoded;
  }
  if (month) {
    decoded = (value >> 20) & ((1 << 4) - 1);
    *month = decoded == ((1 << 4) - 1) ? -1 : decoded;
  }
  if (year) {
    decoded = (value >> 24) & ((1 << 5) - 1);
    *year = decoded == ((1 << 5) - 1) ? -1 : decoded;
  }
  if (flags) {
    decoded = (value >> 29) & ((1 << 2) - 1);
    *flags = decoded == ((1 << 2) - 1) ? -1 : decoded;
  }
}

void WowTime::WowEncodeTime(unsigned int &value, const WowTime *time) {
  WowEncodeTime(value, time->m_minute, time->m_hour, time->m_weekday, time->m_monthDay, time->m_month, time->m_year, time->m_flags);
}

void WowTime::WowDecodeTime(unsigned int value, WowTime *time) {
  WowDecodeTime(value, &time->m_minute, &time->m_hour, &time->m_weekday, &time->m_monthDay, &time->m_month, &time->m_year, &time->m_flags);
}

const char *WowTime::WowGetTimeString(unsigned int value, char *string, int maxlen) {
  WowTime time(value);
  return WowGetTimeString(&time, string, maxlen);
}

const char *WowTime::WowGetTimeString(WowTime *time, char *string, int maxlen) {
  unsigned int value;
  char         buffMonth[8];
  char         buffmonthDay[8];
  char         buffYear[8];
  char         buffWeekDay[8];
  char         buffHour[8];
  char         buffMinute[8];

  WowEncodeTime(value, time);
  if (!value) {
    SStrPrintf(string, maxlen, "Not Set");
    return string;
  }

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
