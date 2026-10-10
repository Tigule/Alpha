#include <storm.h>
#include <stpl.h>

#include <ctype.h>

NODEDECL(CMDDEF) {
  DWORD        flags;
  DWORD        id;
  char         name[0x10];
  int          namelength;
  DWORD        setvalue;
  DWORD        setmask;
  LPVOID       variableptr;
  DWORD        variablebytes;
  SCMDCALLBACK callback;
  int          found;
  union {
    DWORD currvalue;
    char *currvaluestr;
  };
};

typedef struct _PROCESSING {
  CMDDEF *ptr;
  char    name[0x10];
  int     namelength;
} PROCESSING;

typedef LIST(CMDDEF) CMDDEF_LIST;

static LPCSTR const s_errorstr[] = {"Invalid argument: %s", "The syntax of the command is incorrect.", "Unable to open response file: %s"};
static BOOL         s_addedoptional;
static CMDDEF_LIST  s_arglist;
static CMDDEF_LIST  s_flaglist;
#define SCMD_ARG_LIST  (&s_arglist)
#define SCMD_FLAG_LIST (&s_flaglist)

static void ConvertBool(CMDDEF *ptr, LPCSTR string, int *datachars) {
  BOOL enabled;

  if (string[0] == '-') {
    enabled = FALSE;
    *datachars = 1;
  } else if (string[0] == '+') {
    enabled = TRUE;
    *datachars = 1;
  } else {
    enabled = ((BYTE)ptr->flags & SCMD_BOOL_MASK) != SCMD_BOOL_CLEAR;
  }

  ptr->currvalue &= ~ptr->setmask;
  if (enabled) {
    ptr->currvalue |= ptr->setvalue;
  }

  if (ptr->variableptr) {
    *(DWORD *)ptr->variableptr &= ~ptr->setmask;
    if (enabled) {
      *(DWORD *)ptr->variableptr |= ptr->setvalue;
    }
  }
}

static void ConvertNumber(CMDDEF *ptr, LPCSTR string, int *datachars) {
  char *endptr = NULL;

  if ((ptr->flags & SCMD_NUM_MASK) == SCMD_NUM_SIGNED) {
    ptr->currvalue = strtol(string, &endptr, 0);
  } else {
    ptr->currvalue = strtoul(string, &endptr, 0);
  }

  if (endptr) {
    *datachars = endptr - string;
  } else {
    *datachars = SStrLen(string);
  }
  if (ptr->variableptr) {
    memcpy(ptr->variableptr, &ptr->currvalue, min(sizeof(DWORD), ptr->variablebytes));
  }
}

static void ConvertString(CMDDEF *ptr, LPCSTR string, int *datachars) {
  *datachars = SStrLen(string);
  if (ptr->currvaluestr) {
    SMemFree(ptr->currvaluestr, __FILE__, __LINE__, 0);
  }
  ptr->currvaluestr = (char *)SMemAlloc(SStrLen(string) + 1, __FILE__, __LINE__, 0);
  SStrCopy(ptr->currvaluestr, string, 0x7FFFFFFF);
  if (ptr->variableptr) {
    SStrCopy((char *)ptr->variableptr, string, ptr->variablebytes);
  }
}

static CMDDEF *FindFlagDef(LPCSTR string, CMDDEF *firstdef, int minlength) {
  int     strlength = SStrLen(string);
  int     bestchars = minlength - 1;
  CMDDEF *bestptr = NULL;

  for (CMDDEF *def = firstdef; def; def = def->Next()) {
    if (def->namelength > bestchars && def->namelength <= strlength) {
      if ((def->flags & SCMD_CASESENSITIVE) ? !strncmp(def->name, string, def->namelength) : !_strnicmp(def->name, string, def->namelength)) {
        bestchars = def->namelength;
        bestptr = def;
      }
    }
  }

  return bestptr;
}

static void GenerateError(SCMDERRORCALLBACK errorcallback, DWORD errorcode, LPCSTR itemstring) {
  char     errorstr[0x100];
  char     buffer[0x100];
  CMDERROR data;
  int      errorIndex;
  int      resourceId;

  switch (errorcode) {
    case STORM_ERROR_BAD_ARGUMENT:
      errorIndex = 0;
      resourceId = 0x5201;
      break;

    case STORM_ERROR_NOT_ENOUGH_ARGUMENTS:
      errorIndex = 1;
      resourceId = 0x5202;
      break;

    case ERROR_OPEN_FAILED:
      errorIndex = 2;
      resourceId = 0x5203;
      break;

    default:
      return;
  }

  buffer[0] = 0;
  LoadStringA(StormGetInstance(), resourceId, buffer, sizeof(buffer));
  if (!buffer[0]) {
    SStrCopy(buffer, s_errorstr[errorIndex], sizeof(buffer));
  }

  if (strstr(buffer, "%s")) {
    wsprintfA(errorstr, buffer, itemstring);
  } else {
    SStrCopy(errorstr, buffer, sizeof(errorstr));
  }
  if (errorstr[0]) {
    SStrPack(errorstr, "\n", sizeof(errorstr));
  }

  SErrSetLastError(errorcode);
  data.errorcode = errorcode;
  data.itemstr = itemstring;
  data.errorstr = errorstr;
  errorcallback(&data);
}

static BOOL PerformConversion(CMDDEF *ptr, LPCSTR string, int *datachars) {
  CMDPARAMS params;
  DWORD     type;
  int       pass;

  *datachars = 0;

  switch (ptr->flags & SCMD_TYPE_MASK) {
    case SCMD_TYPE_BOOL:
      ConvertBool(ptr, string, datachars);
      break;

    case SCMD_TYPE_NUMERIC:
      ConvertNumber(ptr, string, datachars);
      break;

    case SCMD_TYPE_STRING:
      ConvertString(ptr, string, datachars);
      break;

    default:
      return FALSE;
  }

  ptr->found = TRUE;
  if (ptr->callback) {
    params.flags = ptr->flags;
    params.id = ptr->id;
    params.name = ptr->name;
    params.variable = ptr->variableptr;
    params.setvalue = ptr->setvalue;
    params.setmask = ptr->setmask;
    params.unsignedvalue = ptr->currvalue;
    if (!ptr->callback(&params, string)) {
      return FALSE;
    }
  }

  for (pass = 0; pass <= 1; pass++) {
    ITERATELISTPTR(CMDDEF, (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST), other) {
      if (other->id == ptr->id) {
        type = other->flags & SCMD_TYPE_MASK;
        if (type == (ptr->flags & SCMD_TYPE_MASK) && other != ptr) {
          other->found = TRUE;
          if (type == SCMD_TYPE_STRING) {
            if (other->currvaluestr) {
              SMemFree(other->currvaluestr, __FILE__, __LINE__, 0);
            }
            other->currvaluestr = (char *)SMemAlloc(SStrLen(ptr->currvaluestr) + 1, __FILE__, __LINE__, 0);
            SStrCopy(other->currvaluestr, ptr->currvaluestr, 0x7FFFFFFF);
          } else {
            other->currvalue = ptr->currvalue;
          }
        }
      }
    }
  }

  return TRUE;
}

static BOOL ProcessCurrentFlag(LPCSTR string, PROCESSING *processing, int *datachars) {
  *datachars = 0;
  CMDDEF *ptr = processing->ptr;
  processing->ptr = NULL;
  while (ptr) {
    int currdatachars;
    if (!PerformConversion(ptr, string, &currdatachars)) {
      return FALSE;
    }
    *datachars = max(*datachars, currdatachars);
    ptr = FindFlagDef(processing->name, ptr->Next(), processing->namelength);
  }

  return TRUE;
}

static BOOL
ProcessString(LPCSTR *stringptr, PROCESSING *processing, CMDDEF **nextarg, SCMDPROCESSCALLBACK extracallback, SCMDERRORCALLBACK errorcallback);

static BOOL
ProcessFile(LPCSTR filename, PROCESSING *processing, CMDDEF **nextarg, SCMDPROCESSCALLBACK extracallback, SCMDERRORCALLBACK errorcallback) {
  LPCSTR curr;
  DWORD  bytesread;
  HANDLE handle;
  DWORD  size;
  char  *buffer;
  BOOL   result;

  handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (handle == INVALID_HANDLE_VALUE) {
    if (errorcallback) {
      GenerateError(errorcallback, ERROR_OPEN_FAILED, filename);
    }
    return FALSE;
  }

  size = GetFileSize(handle, NULL);
  buffer = (char *)SMemAlloc(size + 1, __FILE__, __LINE__, 0);
  ReadFile(handle, buffer, size, &bytesread, NULL);
  CloseHandle(handle);

  buffer[bytesread] = 0;
  curr = buffer;
  result = ProcessString(&curr, processing, nextarg, extracallback, errorcallback);
  SMemFree(buffer, __FILE__, __LINE__, 0);
  return result;
}

static BOOL ProcessFlags(LPCSTR string, PROCESSING *processing, SCMDERRORCALLBACK errorcallback) {
  char lastflag[0x100];
  int  datachars;
  int  strlength;
  int  namelength;

  lastflag[0] = 0;
  while (*string) {
    CMDDEF *cmd;

    strlength = SStrLen(string);
    namelength = SStrLen(lastflag) < 1 ? 1 : (int)SStrLen(lastflag);

    cmd = NULL;
    while (namelength--) {
      if (strlength + namelength < 0x100) {
        SStrCopy(lastflag + namelength, string, sizeof(lastflag));
        cmd = FindFlagDef(lastflag, SCMD_FLAG_LIST->Head(), 0);
        if (cmd) {
          namelength = cmd->namelength;
          lastflag[namelength] = 0;
          break;
        }
      }
    }

    if (!cmd) {
      if (errorcallback) {
        GenerateError(errorcallback, STORM_ERROR_BAD_ARGUMENT, string);
      }
      return FALSE;
    }

    string += namelength;
    while (SStrChr("=:", *string)) {
      string++;
    }

    processing->ptr = cmd;
    processing->namelength = namelength;
    SStrCopy(processing->name, lastflag, 0x7FFFFFFF);

    if (!*string && (cmd->flags & SCMD_TYPE_MASK) != SCMD_TYPE_BOOL) {
      cmd->found = TRUE;
      return TRUE;
    }

    if (!ProcessCurrentFlag(string, processing, &datachars)) {
      return FALSE;
    }
    string += datachars;
  }

  return TRUE;
}

static BOOL
ProcessToken(LPCSTR string, int quoted, PROCESSING *processing, CMDDEF **nextarg, SCMDPROCESSCALLBACK extracallback, SCMDERRORCALLBACK errorcallback);

static BOOL
ProcessString(LPCSTR *stringptr, PROCESSING *processing, CMDDEF **nextarg, SCMDPROCESSCALLBACK extracallback, SCMDERRORCALLBACK errorcallback) {
  while (**stringptr) {
    char   buffer[0x100];
    int    quoted = 0;
    LPCSTR nextptr = *stringptr;
    SStrTokenize(&nextptr, buffer, sizeof(buffer), " ,;\"\t\n\r\x1A", &quoted);
    if (buffer[0] && !ProcessToken(buffer, quoted, processing, nextarg, extracallback, errorcallback)) {
      break;
    }
    *stringptr = nextptr;
  }

  return !**stringptr;
}

static BOOL ProcessToken(
    LPCSTR              string,
    int                 quoted,
    PROCESSING         *processing,
    CMDDEF            **nextarg,
    SCMDPROCESSCALLBACK extracallback,
    SCMDERRORCALLBACK   errorcallback
) {
  if (string[0] == '@' && !quoted) {
    return ProcessFile(string + 1, processing, nextarg, extracallback, errorcallback);
  }

  if (SStrChr("-/", string[0]) && !quoted) {
    processing->ptr = NULL;
    return ProcessFlags(string + 1, processing, errorcallback);
  }

  if (processing->ptr) {
    int datachars;
    return ProcessCurrentFlag(string, processing, &datachars);
  }

  if (*nextarg) {
    int datachars;
    if (!PerformConversion(*nextarg, string, &datachars)) {
      return FALSE;
    }
    *nextarg = (*nextarg)->Next();
    return TRUE;
  }

  if (extracallback) {
    return extracallback(string);
  }

  if (errorcallback) {
    GenerateError(errorcallback, STORM_ERROR_BAD_ARGUMENT, string);
  }
  return FALSE;
}

extern "C" BOOL APIENTRY SCmdCheckId(DWORD id) {
  int pass;

  for (pass = 0; pass <= 1; pass++) {
    ITERATELISTPTR(CMDDEF, (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST), cmd) {
      if (cmd->id == id) {
        return cmd->found;
      }
    }
  }
  return FALSE;
}

extern "C" BOOL APIENTRY SCmdDestroy() {
  int pass;

  for (pass = 0; pass <= 1; pass++) {
    ITERATELISTPTR(CMDDEF, (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST), cmd) {
      if ((cmd->flags & SCMD_TYPE_MASK) == SCMD_TYPE_STRING && cmd->currvaluestr) {
        SMemFree(cmd->currvaluestr, __FILE__, __LINE__, 0);
        cmd->currvaluestr = NULL;
      }
    }
  }

  SCMD_ARG_LIST->Clear();
  SCMD_FLAG_LIST->Clear();
  s_addedoptional = FALSE;
  return TRUE;
}

extern "C" BOOL APIENTRY SCmdGetBool(DWORD id) {
  return SCmdGetNum(id) != 0;
}

extern "C" DWORD APIENTRY SCmdGetNum(DWORD id) {
  int pass;

  for (pass = 0; pass <= 1; pass++) {
    ITERATELISTPTR(CMDDEF, (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST), cmd) {
      if (cmd->id == id) {
        return cmd->currvalue;
      }
    }
  }
  return 0;
}

extern "C" BOOL APIENTRY SCmdGetString(DWORD id, char *buffer, DWORD bufferchars) {
  int pass;

  VALIDATEBEGIN;
  VALIDATEANDBLANK(buffer);
  VALIDATE(bufferchars);
  VALIDATEEND;

  for (pass = 0; pass <= 1; pass++) {
    ITERATELISTPTR(CMDDEF, (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST), cmd) {
      if (cmd->id == id) {
        if (cmd->currvaluestr) {
          SStrCopy(buffer, cmd->currvaluestr, bufferchars);
        }
        return TRUE;
      }
    }
  }
  return FALSE;
}

extern "C" BOOL APIENTRY SCmdGetStringAlloc(DWORD id, char **buffer) {
  int pass;

  VALIDATEBEGIN;
  VALIDATEANDBLANK(buffer);
  VALIDATEEND;

  for (pass = 0; pass <= 1; pass++) {
    ITERATELISTPTR(CMDDEF, (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST), cmd) {
      if (cmd->id == id) {
        if (cmd->currvaluestr) {
          *buffer = SStrDupA(cmd->currvaluestr, __FILE__, __LINE__);
        } else {
          *buffer = (char *)SMemAlloc(1, __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY);
        }
        return TRUE;
      }
    }
  }
  return FALSE;
}

extern "C" BOOL APIENTRY SCmdProcess(LPCSTR cmdline, int skipprogname, SCMDPROCESSCALLBACK extracallback, SCMDERRORCALLBACK errorcallback) {
  PROCESSING processing;
  CMDDEF    *nextarg;
  BOOL       result;

  VALIDATEBEGIN;
  VALIDATE(cmdline);
  VALIDATEEND;

  if (skipprogname) {
    SStrTokenize(&cmdline, NULL, 0, " ,;\"\t\n\r\x1A", NULL);
  }
  memset(&processing, 0, sizeof(processing));
  nextarg = SCMD_ARG_LIST->Head();
  if (!ProcessString(&cmdline, &processing, &nextarg, extracallback, errorcallback)) {
    return FALSE;
  }

  result = TRUE;
  while (nextarg && result) {
    if ((nextarg->flags & SCMD_ARG_MASK) == SCMD_ARG_REQUIRED) {
      result = FALSE;
    } else {
      nextarg = nextarg->Next();
    }
  }

  if (errorcallback && !result) {
    GenerateError(errorcallback, STORM_ERROR_NOT_ENOUGH_ARGUMENTS, "");
  }

  return result;
}

extern "C" BOOL APIENTRY SCmdProcessCommandLine(SCMDPROCESSCALLBACK extracallback, SCMDERRORCALLBACK errorcallback) {
  return SCmdProcess(GetCommandLineA(), TRUE, extracallback, errorcallback);
}

extern "C" BOOL APIENTRY SCmdRegisterArgList(const ARGLIST *listptr, DWORD numargs) {
  DWORD i;

  VALIDATEBEGIN;
  VALIDATE(listptr);
  VALIDATEEND;

  for (i = 0; i < numargs; i++) {
    if (!SCmdRegisterArgument(listptr->flags, listptr->id, listptr->name, NULL, 0, 1, 0xFFFFFFFF, listptr->callback)) {
      return FALSE;
    }
    listptr++;
  }

  return TRUE;
}

extern "C" BOOL APIENTRY SCmdRegisterArgument(
    DWORD        flags,
    DWORD        id,
    LPCSTR       name,
    LPVOID       variableptr,
    DWORD        variablebytes,
    DWORD        setvalue,
    DWORD        setmask,
    SCMDCALLBACK callback
) {
  int     namelength;
  CMDDEF *cmd;

  if (!name) {
    name = "";
  }

  namelength = SStrLen(name);
  VALIDATEBEGIN;
  VALIDATE(namelength < 16);
  VALIDATE((!variablebytes) || variableptr);
  VALIDATE((((flags) & ((0 << 24) | (1 << 24) | (2 << 24))) != (2 << 24)) || (!s_addedoptional));
  VALIDATE((((flags) & ((0 << 24) | (1 << 24) | (2 << 24))) != (0 << 24)) || (namelength > 0));
  VALIDATE((((flags) & ((0 << 16) | (1 << 16) | (2 << 16))) != (0 << 16)) || (!variableptr) || (variablebytes == sizeof(DWORD)));
  VALIDATEEND;

  if (flags & SCMD_ARG_MASK) {
    cmd = SCMD_ARG_LIST->NewNode(LIST_TAIL, 0, 0);
  } else {
    cmd = SCMD_FLAG_LIST->NewNode(LIST_TAIL, 0, 0);
  }

  SStrCopy(cmd->name, name, sizeof(cmd->name));
  cmd->id = id;
  cmd->namelength = namelength;
  cmd->variableptr = variableptr;
  cmd->variablebytes = variablebytes;
  cmd->flags = flags;
  cmd->setvalue = setvalue;
  cmd->setmask = setmask;
  cmd->callback = callback;
  if ((flags & SCMD_TYPE_MASK) == SCMD_TYPE_BOOL && (flags & SCMD_BOOL_MASK) == SCMD_BOOL_CLEAR) {
    cmd->currvalue = setvalue;
  } else {
    cmd->currvalue = 0;
  }

  if ((flags & SCMD_ARG_MASK) == SCMD_ARG_OPTIONAL) {
    s_addedoptional = TRUE;
  }

  return TRUE;
}
