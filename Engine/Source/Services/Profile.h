#pragma once

typedef void *HPROFILE;

HPROFILE __fastcall    ProfileCreate();
int __fastcall         ProfileReadFile(HPROFILE handle, const char *path);
int __fastcall         ProfileWriteFile(HPROFILE handle, const char *path);
int __fastcall         ProfileReadBuffer(HPROFILE handle, const void *buffer, unsigned long bufferBytes);
int __fastcall         ProfileAddValue(HPROFILE profile, const char *section, const char *key, bool value);
int __fastcall         ProfileAddValue(HPROFILE handle, const char *section, const char *key, int value);
int __fastcall         ProfileAddValue(HPROFILE handle, const char *section, const char *key, __int64 value);
int __fastcall         ProfileAddValue(HPROFILE handle, const char *section, const char *key, float value);
int __fastcall         ProfileAddValue(HPROFILE handle, const char *section, const char *key, const char *value);
int __fastcall         ProfileSetValue(HPROFILE profile, const char *section, const char *key, bool value);
int __fastcall         ProfileSetValue(HPROFILE handle, const char *section, const char *key, int value);
int __fastcall         ProfileSetValue(HPROFILE handle, const char *section, const char *key, __int64 value);
int __fastcall         ProfileSetValue(HPROFILE handle, const char *section, const char *key, float value);
int __fastcall         ProfileSetValue(HPROFILE handle, const char *section, const char *key, const char *value);
int __fastcall         ProfileGetValue(HPROFILE profile, const char *section, const char *key, bool *value, unsigned int index);
int __fastcall         ProfileGetValue(HPROFILE handle, const char *section, const char *key, int *value, unsigned int index);
int __fastcall         ProfileGetValue(HPROFILE handle, const char *section, const char *key, __int64 *value, unsigned int index);
int __fastcall         ProfileGetValue(HPROFILE handle, const char *section, const char *key, float *value, unsigned int index);
int __fastcall         ProfileGetValue(HPROFILE handle, const char *section, const char *key, char *value, unsigned int maxChars, unsigned int index);
const char *__fastcall ProfileGetValueNoCopy(HPROFILE handle, const char *section, const char *key, unsigned int index);
unsigned int __fastcall ProfileGetNumValues(HPROFILE handle, const char *section, const char *key);
int __fastcall          ProfileGetValueIndex(HPROFILE handle, const char *section, const char *key, const char *value);
typedef int(__fastcall *PROFILEENUMKEYCALLBACK)(const char *key, const char *value, void *opaqueData);
typedef int(__fastcall *PROFILEENUMSECTIONCALLBACK)(const char *section, void *opaqueData);
void __fastcall ProfileEnumKeys(HPROFILE profile, const char *section, PROFILEENUMKEYCALLBACK callback, void *opaqueData);
void __fastcall ProfileEnumSections(HPROFILE profile, PROFILEENUMSECTIONCALLBACK callback, void *opaqueData);
int __fastcall  ProfileSectionExists(HPROFILE profile, const char *section);
void __fastcall ProfileClose(HPROFILE handle);
