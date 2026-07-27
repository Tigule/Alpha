#ifndef WOW_COMMON_WOWTIME_H
#define WOW_COMMON_WOWTIME_H

#include <string.h>

class WowTime {
 public:
  WowTime();
  WowTime(const WowTime &time) {
    memcpy(this, &time, sizeof(*this));
  }
  WowTime(int, int);
  WowTime(unsigned int value);

  int  GetDaysSinceEpoch() const;
  void SetDaysSinceEpoch(int days);
  int  GetHourAndMinutes() const;
  void SetHourAndMinutes(int minutes);
  int  CompareYear(const WowTime &compareTime) const;
  int  CompareMonth(const WowTime &compareTime) const;
  int  CompareDay(const WowTime &compareTime) const;
  int  CompareWeekday(const WowTime &compareTime) const;
  int  CompareHour(const WowTime &compareTime) const;
  int  CompareMinute(const WowTime &compareTime) const;
  bool InRange(const WowTime &valMin, const WowTime &valMax) const;
  bool operator<(const WowTime &cmpTime) const;
  bool operator<=(const WowTime &cmpTime) const;
  bool operator>(const WowTime &cmpTime) const;
  bool operator>=(const WowTime &cmpTime) const;
  bool operator==(const WowTime &cmpTime) const;
  bool operator!=(const WowTime &cmpTime) const;
  operator unsigned int() const;

  static void WowEncodeTime(unsigned int &value, const WowTime *time);
  static void WowEncodeTime(unsigned int &value, int minute, int hour, int weekday, int monthday, int month, int year, int flags);
  static void WowDecodeTime(unsigned int value, WowTime *time);
  static void WowDecodeTime(unsigned int value, int *minute, int *hour, int *weekday, int *monthday, int *month, int *year, int *flags);
  static const char *WowGetTimeString(WowTime *time, char *string, int maxlen);
  static const char *WowGetTimeString(unsigned int value, char *string, int maxlen);

  int m_minute;
  int m_hour;
  int m_weekday;
  int m_monthDay;
  int m_month;
  int m_year;
  int m_flags;
};

inline WowTime::WowTime(unsigned int value) {
  WowDecodeTime(value, this);
}

#endif
