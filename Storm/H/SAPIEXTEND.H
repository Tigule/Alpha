#pragma once

inline void SMemGenerateReport(SMEMREPORTTYPE reporttype, SMEMREPORTPROC outputproc, HOUTPUTCONTEXT outputcontext) {
  StormCallService(2, reporttype, outputproc, outputcontext);
}
