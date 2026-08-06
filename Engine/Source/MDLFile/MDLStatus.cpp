#include "MDLStatus.h"

#include <storm.h>

namespace MDL {
  LPCSTR TokenText(UINT token);
}

void CMDLStatus::FatalDuplicate(LPCSTR found, int lineno) {
  if (lineno == -1) {
    Add(STATUS_FATAL, "Error: Found duplicate \"%s\"\n", found);
  } else {
    Add(STATUS_FATAL, "Error (line %d): Found duplicate \"%s\"\n", lineno, found);
  }
}

void CMDLStatus::FatalUnmatched(LPCSTR item1, UINT count1, LPCSTR item2, UINT count2, int lineno) {
  if (lineno == -1) {
    Add(STATUS_FATAL, "Error: found %d \"%s\", but %d \"%s\", counts must match\n", count1, item1, count2, item2);
  } else {
    Add(STATUS_FATAL, "Error (line %d): found %d \"%s\", but %d \"%s\", counts must match\n", lineno, count1, item1, count2, item2);
  }
}

void CMDLStatus::FatalNotFound(LPCSTR expected, int lineno) {
  if (lineno == -1) {
    Add(STATUS_FATAL, "Error: Expected \"%s\"\n", expected);
  } else {
    Add(STATUS_FATAL, "Error (line %d): Expected \"%s\"\n", lineno, expected);
  }
}

void CMDLStatus::FatalNotFound(UINT what, int lineno) {
  FatalNotFound(MDL::TokenText(what), lineno);
}

void CMDLStatus::FatalUnexpected(LPCSTR found, int lineno) {
  if (lineno == -1) {
    Add(STATUS_FATAL, "Error: Unexpected token \"%s\"\n", found);
  } else {
    Add(STATUS_FATAL, "Error (line %d): Unexpected token \"%s\"\n", lineno, found);
  }
}

void CMDLStatus::FatalExpected(LPCSTR expected, LPCSTR found, int lineno) {
  if (lineno == -1) {
    Add(STATUS_FATAL, "Error: Expected \"%s\", but found \"%s\"\n", expected, found);
  } else {
    Add(STATUS_FATAL, "Error (line %d): Expected \"%s\", but found \"%s\"\n", lineno, expected, found);
  }
}

void CMDLStatus::FatalExpected(UINT what, LPCSTR found, int lineno) {
  FatalExpected(MDL::TokenText(what), found, lineno);
}

void CMDLStatus::FatalEOF(int lineno) {
  if (lineno == -1) {
    Add(STATUS_FATAL, "Error: Unexpected end of file\n");
  } else {
    Add(STATUS_FATAL, "Error (line %d): Unexpected end of file\n", lineno);
  }
}

void CMDLStatus::WarningCount(LPCSTR item, long expected, long actual, int lineno) {
  if (lineno == -1) {
    Add(STATUS_WARNING, "Warning: Expected %d \"%s\", but found %d\n", expected, item, actual);
  } else {
    Add(STATUS_WARNING, "Warning (line %d): Expected %d \"%s\", but found %d\n", lineno, expected, item, actual);
  }
}

void CMDLStatus::FatalOverran(LPCSTR section, int lineno) {
  if (lineno == -1) {
    Add(STATUS_FATAL, "Error: Section %s overran remaining buffer.\n", section);
  } else {
    Add(STATUS_FATAL, "Error (line %d): Section %s overran remaining buffer.\n", lineno, section);
  }
}

void CMDLStatus::FatalFlunked(LPCSTR section, int lineno) {
  if (lineno == -1) {
    Add(STATUS_FATAL, "Error: Could not create new %s section (out of memory?)\n", section);
  } else {
    Add(STATUS_FATAL, "Error (line %d): Could not create new %s section (out of memory?)\n", lineno, section);
  }
}

void CMDLStatus::FatalBadFileName(LPCSTR path) {
  LPCSTR extension = SStrChrR(path, '.');
  if (!extension) {
    extension = "";
  }
  Add(STATUS_FATAL, "Unrecognized file extension: \"%s\"\n", extension);
}
