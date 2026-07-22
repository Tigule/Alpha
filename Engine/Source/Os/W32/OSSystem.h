#pragma once

int __fastcall           OsLaunchURL(const char *url);
unsigned int __fastcall  OsGetProcessorCount();
unsigned long __fastcall OsGetProcessorFeatures();
unsigned long __fastcall OsGetProcessorFeaturesEx(int &vendorID);
void __fastcall          OsGetVersionString(char *string, int length);
int __fastcall           OsGetComputerName(char *computerName, unsigned long *computerNameLen);
int __fastcall           OsGetUserName(char *userName, unsigned long *userNameLen);
unsigned long __fastcall OsGetPhysicalMemory();
