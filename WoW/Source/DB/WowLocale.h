#pragma once

enum WOW_LOCALE {
  LOCALE_en_US = 0,
  LOCALE_ko_KR = 1,
  LOCALE_ja_JP = 2,
  LOCALE_zh_CN = 3,
  LOCALE_zh_TW = 4,
  LOCALE_es_ES = 5,
  LOCALE_fr_FR = 6,
  LOCALE_de_DE = 7,
  NUM_LOCALES = 8,
  DEFAULT_LANGUAGE = 0
};

extern WOW_LOCALE CURRENT_LANGUAGE;
extern LPCSTR     g_localeID[NUM_LOCALES];
