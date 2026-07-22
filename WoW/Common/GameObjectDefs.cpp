#include <storm.h>

class CGameObjectDef {
 public:
  static int __fastcall GetPropNum(int typeId, int propId);
};

struct ObjectInfo {
  int         numProps;
  const int  *props;
  const void *values;
  const char *name;
  int         unused;
};

static const int s_doorPropertiesList[] = {1, 4, 3};
static const int s_buttonPropertiesList[] = {1, 4, 3};
static const int s_questGiverPropertiesList[] = {4, 5, 17};
static const int s_chestPropertiesList[] = {4, 6, 7, 8, 25, 26, 13, 30};
static const int s_genericPropertiesList[] = {19, 18};
static const int s_trapPropertiesList[] = {4, 31, 24, 10, 9, 23, 3, 32};
static const int s_chairPropertiesList[] = {11, 12};
static const int s_spellFocusPropertiesList[] = {14, 24, 30};
static const int s_textPropertiesList[] = {15, 16, 17};
static const int s_gooberPropertiesList[] = {4, 20, 21, 3, 22, 8, 23, 15, 16, 17};
static const int s_areaDamagePropertiesList[] = {4, 24, 27, 28, 29, 3};
static const int s_cameraPropertiesList[] = {4, 33, 21};
static const int s_mapObjTransPropertiesList[] = {35, 36, 37};
static const int s_ritualPropertiesList[] = {34, 10};

static ObjectInfo s_objectInfo[19] = {
    { 3,        s_doorPropertiesList, 0, 0, 0},
    { 3,      s_buttonPropertiesList, 0, 0, 0},
    { 3,  s_questGiverPropertiesList, 0, 0, 0},
    { 8,       s_chestPropertiesList, 0, 0, 0},
    { 0,                           0, 0, 0, 0},
    { 2,     s_genericPropertiesList, 0, 0, 0},
    { 8,        s_trapPropertiesList, 0, 0, 0},
    { 2,       s_chairPropertiesList, 0, 0, 0},
    { 3,  s_spellFocusPropertiesList, 0, 0, 0},
    { 3,        s_textPropertiesList, 0, 0, 0},
    {10,      s_gooberPropertiesList, 0, 0, 0},
    { 0,                           0, 0, 0, 0},
    { 6,  s_areaDamagePropertiesList, 0, 0, 0},
    { 3,      s_cameraPropertiesList, 0, 0, 0},
    { 0,                           0, 0, 0, 0},
    { 3, s_mapObjTransPropertiesList, 0, 0, 0},
    { 0,                           0, 0, 0, 0},
    { 0,                           0, 0, 0, 0},
    { 2,      s_ritualPropertiesList, 0, 0, 0}
};

int __fastcall CGameObjectDef::GetPropNum(int typeId, int propId) {
  if (typeId >= 19 || propId >= 38) {
    return -1;
  }
  FATALASSERT(typeId >= 0);
  FATALASSERT(propId >= 0);

  for (int propNum = 0; propNum < s_objectInfo[typeId].numProps; ++propNum) {
    if (s_objectInfo[typeId].props[propNum] == propId) {
      return propNum;
    }
  }
  return -1;
}
