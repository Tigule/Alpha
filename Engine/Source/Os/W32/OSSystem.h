#pragma once

BOOL  OsLaunchURL(LPCSTR url);
UINT  OsGetProcessorCount();
DWORD OsGetProcessorFeatures();
DWORD OsGetProcessorFeaturesEx(int &vendorID);
void  OsGetVersionString(char *string, int length);
BOOL  OsGetComputerName(char *computerName, DWORD *computerNameLen);
BOOL  OsGetUserName(char *userName, DWORD *userNameLen);
DWORD OsGetPhysicalMemory();
