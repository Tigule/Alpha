#include <Component/CharacterCustomization.h>
#include <DB/WowClientDB.h>

WOW_LOCALE CURRENT_LANGUAGE;

const char *g_localeID[NUM_LOCALES] = {"enUS", "koKR", "jaJP", "zhCN", "zhTW", "esES", "frFR", "deDE"};

const CHARTEXTUREVARIATIONS g_charTextureSectionMapping[CHARTEXTURESECTION_NUM] = {
    CHARTEXTUREVAR_SKIN, CHARTEXTUREVAR_SKIN, CHARTEXTUREVAR_SKIN, CHARTEXTUREVAR_SKIN,       CHARTEXTUREVAR_FACE,      CHARTEXTUREVAR_FACE,
    CHARTEXTUREVAR_HAIR, CHARTEXTUREVAR_HAIR, CHARTEXTUREVAR_HAIR, CHARTEXTUREVAR_FACIALHAIR, CHARTEXTUREVAR_FACIALHAIR
};

const char *const g_sexString[UNITSEX_LAST] = {"Male", "Female", "NOSEX"};

static const int s_ITEMTYPEARRAY[27] = {0,     1,      2,     4,     8,       16,     32, 64,    128,   256,   512, 3072,   12288, 98304,
                                        65536, 131072, 16384, 32768, 7864320, 262144, 16, 32768, 65536, 65536, 0,   131072, 131072};

extern const int *const g_ITEMTYPEARRAY = s_ITEMTYPEARRAY;
