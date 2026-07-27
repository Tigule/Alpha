#pragma once

int OsLaunchURL(const char *url);
unsigned int OsGetProcessorCount();
unsigned long OsGetProcessorFeatures();
unsigned long OsGetProcessorFeaturesEx(int &vendorID);
void OsGetVersionString(char *string, int length);
int OsGetComputerName(char *computerName, unsigned long *computerNameLen);
int OsGetUserName(char *userName, unsigned long *userNameLen);
unsigned long OsGetPhysicalMemory();
