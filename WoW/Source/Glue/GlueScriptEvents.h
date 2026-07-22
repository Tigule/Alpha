#pragma once

extern const char *g_glueScriptEvents[11];

void __fastcall GlueScriptEventsInitialize();
void __fastcall GlueScriptEventsRegisterFunctions();
void __fastcall GlueScriptEventsUnregisterFunctions();
