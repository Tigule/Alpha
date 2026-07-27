#include <storm.h>
#include <stpl.h>

#include <ctype.h>

struct CMDDEF : public TSLinkedNode<CMDDEF> {
  CMDDEF();
  ~CMDDEF();

  DWORD        flags;
  DWORD        id;
  char         name[0x10];
  int          namelength;
  DWORD        setvalue;
  DWORD        setmask;
  void        *variableptr;
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

typedef TSList<CMDDEF, TSGetLink<CMDDEF> > CMDDEF_LIST;

static const char *s_errorstr[] = {"Invalid argument: %s", "The syntax of the command is incorrect.", "Unable to open response file: %s"};
static BOOL        s_addedoptional;
static CMDDEF_LIST s_arglist;
static CMDDEF_LIST s_flaglist;
#define SCMD_ARG_LIST  (&s_arglist)
#define SCMD_FLAG_LIST (&s_flaglist)

CMDDEF::CMDDEF() {
}

CMDDEF::~CMDDEF() {
}

static void ConvertBool(CMDDEF *ptr, const char *string, int *datachars) {
  BOOL enabled;

  if (string[0] == '-') {
    enabled = FALSE;
    *datachars = 1;
  } else if (string[0] == '+') {
    enabled = TRUE;
    *datachars = 1;
  } else {
    enabled = (ptr->flags & SCMD_BOOL_MASK) != SCMD_BOOL_CLEAR;
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

static void ConvertNumber(CMDDEF *ptr, const char *string, int *datachars) {
  char *endptr = NULL;
  DWORD bytes;

  if ((ptr->flags & SCMD_NUM_MASK) == SCMD_NUM_SIGNED) {
    ptr->currvalue = (DWORD)strtol(string, &endptr, 0);
  } else {
    ptr->currvalue = strtoul(string, &endptr, 0);
  }

  *datachars = endptr ? (int)(endptr - string) : (int)SStrLen(string);
  if (ptr->variableptr) {
    bytes = ptr->variablebytes < sizeof(DWORD) ? ptr->variablebytes : sizeof(DWORD);
    memcpy(ptr->variableptr, &ptr->currvalue, bytes);
  }
}

static void ConvertString(CMDDEF *ptr, const char *string, int *datachars) {
  *datachars = (int)SStrLen(string);
  if (ptr->currvaluestr) {
    SMemFree(ptr->currvaluestr, __FILE__, __LINE__, 0);
  }
  ptr->currvaluestr = (char *)SMemAlloc(SStrLen(string) + 1, __FILE__, __LINE__, 0);
  SStrCopy(ptr->currvaluestr, string, 0x7FFFFFFF);
  if (ptr->variableptr) {
    SStrCopy((char *)ptr->variableptr, string, ptr->variablebytes);
  }
}

static CMDDEF *FindFlagDef(const char *string, CMDDEF *firstdef, int minlength) {
  int     strlength;
  CMDDEF *bestptr = NULL;
  int     bestchars;

  strlength = (int)SStrLen(string);
  bestchars = minlength - 1;
  while (firstdef) {
    if (firstdef->namelength > bestchars && firstdef->namelength <= strlength) {
      if ((firstdef->flags & SCMD_CASESENSITIVE) ? !strncmp(firstdef->name, string, firstdef->namelength)
                                                 : !_strnicmp(firstdef->name, string, firstdef->namelength))
      {
        bestptr = firstdef;
        bestchars = firstdef->namelength;
      }
    }
    firstdef = firstdef->Next();
  }

  return bestptr;
}

static void GenerateError(SCMDERRORCALLBACK errorcallback, DWORD errorcode, const char *itemstring) {
  char     errorstr[0x100];
  char     buffer[0x100];
  CMDERROR data;
  int      errorIndex;
  int      resourceId;

  switch (errorcode) {
    case 0x85100065:
      errorIndex = 0;
      resourceId = 0x5201;
      break;

    case 0x8510006D:
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

static BOOL PerformConversion(CMDDEF *ptr, const char *string, int *datachars) {
  CMDPARAMS    params;
  CMDDEF_LIST *list;
  CMDDEF      *other;
  DWORD        type;
  int          pass;

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

  for (pass = 0; pass < 2; pass++) {
    list = pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST;
    other = list->Head();
    while ((LONG)other > 0) {
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
      other = list->RawNext(other);
    }
  }

  return TRUE;
}

static BOOL ProcessCurrentFlag(const char *string, PROCESSING *processing, int *datachars) {
  CMDDEF *cmd;
  int     currdatachars;

  *datachars = 0;
  cmd = processing->ptr;
  processing->ptr = NULL;
  while (cmd) {
    currdatachars = 0;
    if (!PerformConversion(cmd, string, &currdatachars)) {
      return FALSE;
    }
    if (currdatachars > *datachars) {
      *datachars = currdatachars;
    }
    cmd = FindFlagDef(processing->name, cmd->Next(), processing->namelength);
  }

  return TRUE;
}

static BOOL
ProcessString(const char **stringptr, PROCESSING *processing, CMDDEF **nextarg, SCMDPROCESSCALLBACK extracallback, SCMDERRORCALLBACK errorcallback);

static BOOL
ProcessFile(const char *filename, PROCESSING *processing, CMDDEF **nextarg, SCMDPROCESSCALLBACK extracallback, SCMDERRORCALLBACK errorcallback) {
  const char *curr;
  DWORD       bytesread;
  HANDLE      handle;
  DWORD       size;
  char       *buffer;
  BOOL        result;

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

static BOOL ProcessFlags(const char *string, PROCESSING *processing, SCMDERRORCALLBACK errorcallback) {
  char lastflag[0x100];
  int  datachars;
  int  strlength;

  lastflag[0] = 0;
  while (*string) {
    CMDDEF *cmd;

    strlength = (int)SStrLen(string);
    datachars = SStrLen(lastflag) < 1 ? 1 : (int)SStrLen(lastflag);

    cmd = NULL;
    while (datachars--) {
      if (strlength + datachars < 0x100) {
        SStrCopy(lastflag + datachars, string, sizeof(lastflag));
        cmd = FindFlagDef(lastflag, SCMD_FLAG_LIST->Head(), 0);
        if (cmd) {
          datachars = cmd->namelength;
          lastflag[datachars] = 0;
          break;
        }
      }
    }

    if (!cmd) {
      if (errorcallback) {
        GenerateError(errorcallback, 0x85100065, string);
      }
      return FALSE;
    }

    string += datachars;
    while (SStrChr("=:", *string)) {
      string++;
    }

    processing->ptr = cmd;
    processing->namelength = datachars;
    SStrCopy(processing->name, lastflag, 0x7FFFFFFF);

    if (!*string && (cmd->flags & SCMD_TYPE_MASK) != SCMD_TYPE_BOOL) {
      cmd->found = TRUE;
      return TRUE;
    }

    datachars = 0;
    if (!ProcessCurrentFlag(string, processing, &datachars)) {
      return FALSE;
    }
    string += datachars;
  }

  return TRUE;
}

static BOOL ProcessToken(
    const char         *string,
    int                 quoted,
    PROCESSING         *processing,
    CMDDEF            **nextarg,
    SCMDPROCESSCALLBACK extracallback,
    SCMDERRORCALLBACK   errorcallback
);

static BOOL
ProcessString(const char **stringptr, PROCESSING *processing, CMDDEF **nextarg, SCMDPROCESSCALLBACK extracallback, SCMDERRORCALLBACK errorcallback) {
  char        buffer[0x100];
  const char *nextptr;
  int         quoted;

  while (**stringptr) {
    nextptr = *stringptr;
    SStrTokenize(&nextptr, buffer, sizeof(buffer), " ,;\"\t\n\r\x1A", &quoted);
    if (buffer[0] && !ProcessToken(buffer, quoted, processing, nextarg, extracallback, errorcallback)) {
      break;
    }
    *stringptr = nextptr;
  }

  return !**stringptr;
}

static BOOL ProcessToken(
    const char         *string,
    int                 quoted,
    PROCESSING         *processing,
    CMDDEF            **nextarg,
    SCMDPROCESSCALLBACK extracallback,
    SCMDERRORCALLBACK   errorcallback
) {
  if (!quoted && string[0] == '@') {
    return ProcessFile(string + 1, processing, nextarg, extracallback, errorcallback);
  }

  if (!quoted && SStrChr("-/", string[0])) {
    processing->ptr = NULL;
    return ProcessFlags(string + 1, processing, errorcallback);
  }

  if (processing->ptr) {
    int datachars;
    return ProcessCurrentFlag(string, processing, &datachars);
  }

  if (*nextarg) {
    int datachars;
    if (PerformConversion(*nextarg, string, &datachars)) {
      *nextarg = (*nextarg)->Next();
      return TRUE;
    }
  } else if (extracallback) {
    return extracallback(string);
  } else if (errorcallback) {
    GenerateError(errorcallback, 0x85100065, string);
  }

  return FALSE;
}

extern "C" BOOL APIENTRY SCmdCheckId(DWORD id) {
  CMDDEF *cmd;
  int     pass;

  for (pass = 0; pass <= 1; pass++) {
    cmd = (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST)->Head();
    while ((LONG)cmd > 0) {
      if (cmd->id == id) {
        return cmd->found;
      }
      cmd = (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST)->RawNext(cmd);
    }
  }
  return FALSE;
}

extern "C" BOOL APIENTRY SCmdDestroy() {
  CMDDEF *cmd;
  int     pass;

  for (pass = 0; pass <= 1; pass++) {
    cmd = (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST)->Head();
    while ((LONG)cmd > 0) {
      if ((cmd->flags & SCMD_TYPE_MASK) == SCMD_TYPE_STRING && cmd->currvaluestr) {
        SMemFree(cmd->currvaluestr, __FILE__, __LINE__, 0);
        cmd->currvaluestr = NULL;
      }
      cmd = (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST)->RawNext(cmd);
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
  CMDDEF *cmd;
  int     pass;

  for (pass = 0; pass <= 1; pass++) {
    cmd = (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST)->Head();
    while ((LONG)cmd > 0) {
      if (cmd->id == id) {
        return cmd->currvalue;
      }
      cmd = (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST)->RawNext(cmd);
    }
  }
  return 0;
}

extern "C" BOOL APIENTRY SCmdGetString(DWORD id, char *buffer, DWORD bufferchars) {
  CMDDEF *cmd;
  int     pass;

  FATALASSERT(buffer);
  buffer[0] = 0;
  FATALASSERT(bufferchars);

  for (pass = 0; pass <= 1; pass++) {
    cmd = (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST)->Head();
    while ((LONG)cmd > 0) {
      if (cmd->id == id) {
        if (cmd->currvaluestr) {
          SStrCopy(buffer, cmd->currvaluestr, bufferchars);
        }
        return TRUE;
      }
      cmd = (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST)->RawNext(cmd);
    }
  }
  return FALSE;
}

extern "C" BOOL APIENTRY SCmdGetStringAlloc(DWORD id, char **buffer) {
  CMDDEF *cmd;
  int     pass;

  FATALASSERT(buffer);

  *buffer = NULL;
  for (pass = 0; pass <= 1; pass++) {
    cmd = (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST)->Head();
    while ((LONG)cmd > 0) {
      if (cmd->id == id) {
        if (cmd->currvaluestr) {
          *buffer = SStrDupA(cmd->currvaluestr, __FILE__, __LINE__);
        } else {
          *buffer = (char *)SMemAlloc(1, __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY);
        }
        return TRUE;
      }
      cmd = (pass ? SCMD_FLAG_LIST : SCMD_ARG_LIST)->RawNext(cmd);
    }
  }
  return FALSE;
}

extern "C" BOOL APIENTRY SCmdProcess(const char *cmdline, int skipprogname, SCMDPROCESSCALLBACK extracallback, SCMDERRORCALLBACK errorcallback) {
  PROCESSING processing;
  CMDDEF    *nextarg;
  BOOL       result;

  FATALASSERT(cmdline);

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
    GenerateError(errorcallback, 0x8510006D, "");
  }

  return result;
}

extern "C" BOOL APIENTRY SCmdProcessCommandLine(SCMDPROCESSCALLBACK extracallback, SCMDERRORCALLBACK errorcallback) {
  return SCmdProcess(GetCommandLineA(), TRUE, extracallback, errorcallback);
}

extern "C" BOOL APIENTRY SCmdRegisterArgList(const ARGLIST *listptr, DWORD numargs) {
  DWORD i;

  FATALASSERT(listptr);

  for (i = 0; i < numargs; i++) {
    if (!SCmdRegisterArgument(listptr[i].flags, listptr[i].id, listptr[i].name, NULL, 0, 1, 0xFFFFFFFF, listptr[i].callback)) {
      return FALSE;
    }
  }

  return TRUE;
}

extern "C" BOOL APIENTRY SCmdRegisterArgument(
    DWORD        flags,
    DWORD        id,
    const char  *name,
    void        *variableptr,
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

  namelength = (int)SStrLen(name);
  FATALASSERT(namelength < 16);
  FATALASSERT((!variablebytes) || variableptr);
  FATALASSERT((((flags) & ((0 << 24) | (1 << 24) | (2 << 24))) != (2 << 24)) || (!s_addedoptional));
  FATALASSERT((((flags) & ((0 << 24) | (1 << 24) | (2 << 24))) != (0 << 24)) || (namelength > 0));
  FATALASSERT((((flags) & ((0 << 16) | (1 << 16) | (2 << 16))) != (0 << 16)) || (!variableptr) || (variablebytes == sizeof(DWORD)));

  cmd = ((flags & SCMD_ARG_MASK) ? SCMD_ARG_LIST : SCMD_FLAG_LIST)->NewNode(LIST_TAIL, 0, 0);

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
