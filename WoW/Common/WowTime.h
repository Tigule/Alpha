#ifndef WOW_COMMON_WOWTIME_H
#define WOW_COMMON_WOWTIME_H

#include <Base/Base.h>
#include <string.h>

class WowTime {
 public:
  WowTime();
  WowTime(const WowTime &time) {
    memcpy(this, &time, sizeof(*this));
  }
  WowTime(int, int);
  WowTime(UINT value);

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
       operator UINT() const;

  static void   WowEncodeTime(UINT &value, const WowTime *time);
  static void   WowEncodeTime(UINT &value, int minute, int hour, int weekday, int monthday, int month, int year, int flags);
  static void   WowDecodeTime(UINT value, WowTime *time);
  static void   WowDecodeTime(UINT value, int *minute, int *hour, int *weekday, int *monthday, int *month, int *year, int *flags);
  static LPCSTR WowGetTimeString(WowTime *time, char *string, int maxlen);
  static LPCSTR WowGetTimeString(UINT value, char *string, int maxlen);

  int m_minute;
  int m_hour;
  int m_weekday;
  int m_monthDay;
  int m_month;
  int m_year;
  int m_flags;
};

inline WowTime::WowTime(UINT value) {
  WowDecodeTime(value, this);
}

#endif
