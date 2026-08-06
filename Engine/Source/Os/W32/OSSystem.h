#pragma once

int   OsLaunchURL(LPCSTR url);
UINT  OsGetProcessorCount();
DWORD OsGetProcessorFeatures();
DWORD OsGetProcessorFeaturesEx(int &vendorID);
void  OsGetVersionString(char *string, int length);
int   OsGetComputerName(char *computerName, DWORD *computerNameLen);
int   OsGetUserName(char *userName, DWORD *userNameLen);
DWORD OsGetPhysicalMemory();
