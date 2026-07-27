#pragma once

class unreal;

typedef void *HPROFILE;

HPROFILE ProfileCreate();
int ProfileReadFile(HPROFILE handle, const char *path);
int ProfileWriteFile(HPROFILE handle, const char *path);
int ProfileReadBuffer(HPROFILE handle, const void *buffer, unsigned long bufferBytes);
int ProfileAddValue(HPROFILE profile, const char *section, const char *key, bool value);
int ProfileAddValue(HPROFILE handle, const char *section, const char *key, int value);
int ProfileAddValue(HPROFILE handle, const char *section, const char *key, __int64 value);
int ProfileAddValue(HPROFILE handle, const char *section, const char *key, float value);
int ProfileAddValue(HPROFILE handle, const char *section, const char *key, const unreal &value);
int ProfileAddValue(HPROFILE handle, const char *section, const char *key, const char *value);
int ProfileSetValue(HPROFILE profile, const char *section, const char *key, bool value);
int ProfileSetValue(HPROFILE handle, const char *section, const char *key, int value);
int ProfileSetValue(HPROFILE handle, const char *section, const char *key, __int64 value);
int ProfileSetValue(HPROFILE handle, const char *section, const char *key, float value);
int ProfileSetValue(HPROFILE handle, const char *section, const char *key, const unreal &value);
int ProfileSetValue(HPROFILE handle, const char *section, const char *key, const char *value);
int ProfileGetValue(HPROFILE profile, const char *section, const char *key, bool *value, unsigned int index);
int ProfileGetValue(HPROFILE handle, const char *section, const char *key, int *value, unsigned int index);
int ProfileGetValue(HPROFILE handle, const char *section, const char *key, __int64 *value, unsigned int index);
int ProfileGetValue(HPROFILE handle, const char *section, const char *key, float *value, unsigned int index);
int ProfileGetValue(HPROFILE handle, const char *section, const char *key, unreal *value, unsigned int index);
int ProfileGetValue(HPROFILE handle, const char *section, const char *key, char *value, unsigned int maxChars, unsigned int index);
const char *ProfileGetValueNoCopy(HPROFILE handle, const char *section, const char *key, unsigned int index);
unsigned int ProfileGetNumValues(HPROFILE handle, const char *section, const char *key);
int ProfileGetValueIndex(HPROFILE handle, const char *section, const char *key, const char *value);
typedef int(*PROFILEENUMKEYCALLBACK)(const char *key, const char *value, void *opaqueData);
typedef int(*PROFILEENUMSECTIONCALLBACK)(const char *section, void *opaqueData);
void ProfileEnumKeys(HPROFILE profile, const char *section, PROFILEENUMKEYCALLBACK callback, void *opaqueData);
void ProfileEnumSections(HPROFILE profile, PROFILEENUMSECTIONCALLBACK callback, void *opaqueData);
int ProfileSectionExists(HPROFILE profile, const char *section);
void ProfileClose(HPROFILE handle);
