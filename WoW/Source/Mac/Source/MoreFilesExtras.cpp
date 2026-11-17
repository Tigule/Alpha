#include <CoreServices/CoreServices.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static OSErr DeleteLevel(const char *path) {
  DIR           *dir;
  struct dirent *entry;
  OSErr          err = noErr;

  dir = opendir(path);
  if (!dir) {
    return dirNFErr;
  }

  while ((entry = readdir(dir)) != NULL) {
    char        child[PATH_MAX];
    struct stat info;

    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
      continue;
    }

    snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);

    if (lstat(child, &info)) {
      continue;
    }

    if (S_ISDIR(info.st_mode)) {
      err = DeleteLevel(child);
      if (err == noErr && rmdir(child) && (errno != EPERM || chflags(child, info.st_flags & ~UF_IMMUTABLE) || rmdir(child))) {
        err = fBsyErr;
      }
    } else if (unlink(child) && (errno != EPERM || chflags(child, info.st_flags & ~UF_IMMUTABLE) || unlink(child))) {
      err = fBsyErr;
    }

    if (err != noErr) {
      break;
    }
  }

  closedir(dir);
  return err;
}

extern "C" OSErr DeleteDirectory(const char *path) {
  OSErr err = DeleteLevel(path);

  if (err == noErr && rmdir(path)) {
    err = errno == ENOENT ? dirNFErr : fBsyErr;
  }

  return err;
}
