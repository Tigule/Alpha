#include "Frame/SimpleFrameRegistry.h"

#include "Services/SysMessage.h"

#include <stpl.h>

struct SIMPLEFRAMEREGHASH : public TSHashObject<SIMPLEFRAMEREGHASH, HASHKEY_STR> {
  CSimpleFrame *object;
};

struct SIMPLETEXTUREREGHASH : public TSHashObject<SIMPLETEXTUREREGHASH, HASHKEY_STR> {
  CSimpleTexture *object;
};

struct SIMPLEFONTSTRINGREGHASH : public TSHashObject<SIMPLEFONTSTRINGREGHASH, HASHKEY_STR> {
  CSimpleFontString *object;
};

struct SIMPLECONTEXTHASHOBJ : public TSHashObject<SIMPLECONTEXTHASHOBJ, HASHKEY_NONE> {
  TSHashTable<SIMPLEFRAMEREGHASH, HASHKEY_STR>      s_frameRegistry;
  TSHashTable<SIMPLETEXTUREREGHASH, HASHKEY_STR>    s_textureRegistry;
  TSHashTable<SIMPLEFONTSTRINGREGHASH, HASHKEY_STR> s_stringRegistry;
};

static TSHashTable<SIMPLECONTEXTHASHOBJ, HASHKEY_NONE> s_contextLookup;
static HASHKEY_NONE                                    s_nullHashKey;

static SIMPLEFRAMEREGHASH *GetSimpleFrameHash(const char *name, unsigned int context, int create, int *alreadyExisted) {
  SIMPLECONTEXTHASHOBJ *contextHash;
  SIMPLEFRAMEREGHASH   *hash;

  ASSERT(alreadyExisted);
  *alreadyExisted = 1;

  contextHash = s_contextLookup.Ptr(context, s_nullHashKey);
  if (!contextHash) {
    *alreadyExisted = 0;
    if (!create) {
      return 0;
    }

    contextHash = s_contextLookup.New(context, s_nullHashKey, 0, 0);
  }

  hash = contextHash->s_frameRegistry.Ptr(name);
  if (!hash) {
    *alreadyExisted = 0;
    if (create) {
      hash = contextHash->s_frameRegistry.New(name, 0, 0);
    }
  }

  return hash;
}

static SIMPLETEXTUREREGHASH *GetSimpleTextureHash(const char *name, unsigned int context, int create, int *alreadyExisted) {
  SIMPLECONTEXTHASHOBJ *contextHash;
  SIMPLETEXTUREREGHASH *hash;

  ASSERT(alreadyExisted);
  *alreadyExisted = 1;

  contextHash = s_contextLookup.Ptr(context, s_nullHashKey);
  if (!contextHash) {
    *alreadyExisted = 0;
    if (!create) {
      return 0;
    }

    contextHash = s_contextLookup.New(context, s_nullHashKey, 0, 0);
  }

  hash = contextHash->s_textureRegistry.Ptr(name);
  if (!hash) {
    *alreadyExisted = 0;
    if (create) {
      hash = contextHash->s_textureRegistry.New(name, 0, 0);
    }
  }

  return hash;
}

static SIMPLEFONTSTRINGREGHASH *GetSimpleFontStringHash(const char *name, unsigned int context, int create, int *alreadyExisted) {
  SIMPLECONTEXTHASHOBJ    *contextHash;
  SIMPLEFONTSTRINGREGHASH *hash;

  ASSERT(alreadyExisted);
  *alreadyExisted = 1;

  contextHash = s_contextLookup.Ptr(context, s_nullHashKey);
  if (!contextHash) {
    *alreadyExisted = 0;
    if (!create) {
      return 0;
    }

    contextHash = s_contextLookup.New(context, s_nullHashKey, 0, 0);
  }

  hash = contextHash->s_stringRegistry.Ptr(name);
  if (!hash) {
    *alreadyExisted = 0;
    if (create) {
      hash = contextHash->s_stringRegistry.New(name, 0, 0);
    }
  }

  return hash;
}

int SimpleFrameRegistryAddEntry(const char *name, CSimpleFrame *object, unsigned int context) {
  int                 alreadyExisted;
  SIMPLEFRAMEREGHASH *hash;

  FATALASSERT(name);
  FATALASSERT(*name);
  FATALASSERT(object);

  alreadyExisted = 0;
  hash = GetSimpleFrameHash(name, context, 1, &alreadyExisted);
  ASSERT(hash);

  if (alreadyExisted) {
    SysMsgPrintf(SYSMSG_WARNING, 4, "Warning, trying to add duplicate frame \"%s\" in context %d!", name, context);
    return 0;
  }

  hash->object = object;
  return 1;
}

int SimpleTextureRegistryAddEntry(const char *name, CSimpleTexture *object, unsigned int context) {
  int                   alreadyExisted;
  SIMPLETEXTUREREGHASH *hash;

  FATALASSERT(name);
  FATALASSERT(*name);
  FATALASSERT(object);

  alreadyExisted = 0;
  hash = GetSimpleTextureHash(name, context, 1, &alreadyExisted);
  ASSERT(hash);

  if (alreadyExisted) {
    SysMsgPrintf(SYSMSG_WARNING, 4, "Warning, trying to add duplicate texture \"%s\" in context %d!", name, context);
    return 0;
  }

  hash->object = object;
  return 1;
}

int SimpleFontStringRegistryAddEntry(const char *name, CSimpleFontString *object, unsigned int context) {
  int                      alreadyExisted;
  SIMPLEFONTSTRINGREGHASH *hash;

  FATALASSERT(name);
  FATALASSERT(*name);
  FATALASSERT(object);

  alreadyExisted = 0;
  hash = GetSimpleFontStringHash(name, context, 1, &alreadyExisted);
  ASSERT(hash);

  if (alreadyExisted) {
    SysMsgPrintf(SYSMSG_WARNING, 4, "Warning, trying to add duplicate string \"%s\" in context %d!", name, context);
    return 0;
  }

  hash->object = object;
  return 1;
}

void SimpleFrameRegistryRemoveEntry(const char *name, unsigned int context) {
  SIMPLECONTEXTHASHOBJ *contextHash;

  if (!name || !*name) {
    return;
  }

  contextHash = s_contextLookup.Ptr(context, s_nullHashKey);
  if (contextHash) {
    contextHash->s_frameRegistry.Delete(name);
  }
}

void SimpleTextureRegistryRemoveEntry(const char *name, unsigned int context) {
  SIMPLECONTEXTHASHOBJ *contextHash;

  if (!name || !*name) {
    return;
  }

  contextHash = s_contextLookup.Ptr(context, s_nullHashKey);
  if (contextHash) {
    contextHash->s_textureRegistry.Delete(name);
  }
}

void SimpleFontStringRegistryRemoveEntry(const char *name, unsigned int context) {
  SIMPLECONTEXTHASHOBJ *contextHash;

  if (!name || !*name) {
    return;
  }

  contextHash = s_contextLookup.Ptr(context, s_nullHashKey);
  if (contextHash) {
    contextHash->s_stringRegistry.Delete(name);
  }
}

CSimpleFrame *SimpleFrameRegistryGetEntry(const char *name, unsigned int context) {
  int                 unused;
  SIMPLEFRAMEREGHASH *hash;

  FATALASSERT(name);
  FATALASSERT(*name);

  hash = GetSimpleFrameHash(name, context, 0, &unused);
  return hash ? hash->object : 0;
}

CSimpleTexture *SimpleTextureRegistryGetEntry(const char *name, unsigned int context) {
  int                   unused;
  SIMPLETEXTUREREGHASH *hash;

  FATALASSERT(name);
  FATALASSERT(*name);

  hash = GetSimpleTextureHash(name, context, 0, &unused);
  return hash ? hash->object : 0;
}

CSimpleFontString *SimpleFontStringRegistryGetEntry(const char *name, unsigned int context) {
  int                      unused;
  SIMPLEFONTSTRINGREGHASH *hash;

  FATALASSERT(name);
  FATALASSERT(*name);

  hash = GetSimpleFontStringHash(name, context, 0, &unused);
  return hash ? hash->object : 0;
}

void SimpleFrameRegistryClear() {
  s_contextLookup.Clear();
}
