#pragma once

class unreal;

typedef LPVOID HPROFILE;

HPROFILE ProfileCreate();
int      ProfileReadFile(HPROFILE handle, LPCSTR path);
int      ProfileWriteFile(HPROFILE handle, LPCSTR path);
int      ProfileReadBuffer(HPROFILE handle, LPCVOID buffer, DWORD bufferBytes);
BOOL     ProfileAddValue(HPROFILE profile, LPCSTR section, LPCSTR key, bool value);
BOOL     ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, int value);
BOOL     ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, LONGLONG value);
BOOL     ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, float value);
BOOL     ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, const unreal &value);
BOOL     ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, LPCSTR value);
BOOL     ProfileSetValue(HPROFILE profile, LPCSTR section, LPCSTR key, bool value);
BOOL     ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, int value);
BOOL     ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, LONGLONG value);
BOOL     ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, float value);
BOOL     ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, const unreal &value);
BOOL     ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, LPCSTR value);
BOOL     ProfileGetValue(HPROFILE profile, LPCSTR section, LPCSTR key, bool *value, UINT index);
BOOL     ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, int *value, UINT index);
BOOL     ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, LONGLONG *value, UINT index);
BOOL     ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, float *value, UINT index);
BOOL     ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, unreal *value, UINT index);
BOOL     ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, char *value, UINT maxChars, UINT index);
LPCSTR   ProfileGetValueNoCopy(HPROFILE handle, LPCSTR section, LPCSTR key, UINT index);
UINT     ProfileGetNumValues(HPROFILE handle, LPCSTR section, LPCSTR key);
int      ProfileGetValueIndex(HPROFILE handle, LPCSTR section, LPCSTR key, LPCSTR value);
typedef int (*PROFILEENUMKEYCALLBACK)(LPCSTR key, LPCSTR value, LPVOID opaqueData);
typedef int (*PROFILEENUMSECTIONCALLBACK)(LPCSTR section, LPVOID opaqueData);
void ProfileEnumKeys(HPROFILE profile, LPCSTR section, PROFILEENUMKEYCALLBACK callback, LPVOID opaqueData);
void ProfileEnumSections(HPROFILE profile, PROFILEENUMSECTIONCALLBACK callback, LPVOID opaqueData);
BOOL ProfileSectionExists(HPROFILE profile, LPCSTR section);
void ProfileClose(HPROFILE handle);
