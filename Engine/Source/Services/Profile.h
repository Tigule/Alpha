#pragma once

class unreal;

typedef LPVOID HPROFILE;

HPROFILE ProfileCreate();
int      ProfileReadFile(HPROFILE handle, LPCSTR path);
int      ProfileWriteFile(HPROFILE handle, LPCSTR path);
int      ProfileReadBuffer(HPROFILE handle, LPCVOID buffer, DWORD bufferBytes);
int      ProfileAddValue(HPROFILE profile, LPCSTR section, LPCSTR key, bool value);
int      ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, int value);
int      ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, LONGLONG value);
int      ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, float value);
int      ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, const unreal &value);
int      ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, LPCSTR value);
int      ProfileSetValue(HPROFILE profile, LPCSTR section, LPCSTR key, bool value);
int      ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, int value);
int      ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, LONGLONG value);
int      ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, float value);
int      ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, const unreal &value);
int      ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, LPCSTR value);
int      ProfileGetValue(HPROFILE profile, LPCSTR section, LPCSTR key, bool *value, UINT index);
int      ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, int *value, UINT index);
int      ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, LONGLONG *value, UINT index);
int      ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, float *value, UINT index);
int      ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, unreal *value, UINT index);
int      ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, char *value, UINT maxChars, UINT index);
LPCSTR   ProfileGetValueNoCopy(HPROFILE handle, LPCSTR section, LPCSTR key, UINT index);
UINT     ProfileGetNumValues(HPROFILE handle, LPCSTR section, LPCSTR key);
int      ProfileGetValueIndex(HPROFILE handle, LPCSTR section, LPCSTR key, LPCSTR value);
typedef int (*PROFILEENUMKEYCALLBACK)(LPCSTR key, LPCSTR value, LPVOID opaqueData);
typedef int (*PROFILEENUMSECTIONCALLBACK)(LPCSTR section, LPVOID opaqueData);
void ProfileEnumKeys(HPROFILE profile, LPCSTR section, PROFILEENUMKEYCALLBACK callback, LPVOID opaqueData);
void ProfileEnumSections(HPROFILE profile, PROFILEENUMSECTIONCALLBACK callback, LPVOID opaqueData);
int  ProfileSectionExists(HPROFILE profile, LPCSTR section);
void ProfileClose(HPROFILE handle);
