#include <storm.h>

#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

#include <stdio.h>
#include <sys/stat.h>

extern "C" OSErr GetApplicationInformation(StringPtr name, FSRef *appRef, OSType *signature, ProcessSerialNumber *psn);
extern "C" OSErr DeleteDirectory(const char *path);

class AutoTemporarySubfolderClass {
 public:
  AutoTemporarySubfolderClass() {
    Str255 name;
    char   subfolder[31];
    char   folderPath[PATH_MAX];
    FSRef  folder;

    m_valid = false;
    GetApplicationInformation(name, 0, 0, 0);
    snprintf(subfolder, sizeof(subfolder), "com.blizzard.%.*s.Temp", name[0], &name[1]);

    if (FSFindFolder(kOnSystemDisk, kChewableItemsFolderType, kCreateFolder, &folder) == noErr &&
        FSRefMakePath(&folder, (UInt8 *)folderPath, sizeof(folderPath)) == noErr) {
      snprintf(m_path, sizeof(m_path), "%s/%s", folderPath, subfolder);
      DeleteDirectory(m_path);
      if (mkdir(m_path, 0777) == 0) {
        m_valid = true;
      }
    }
  }

  ~AutoTemporarySubfolderClass() {
    if (m_valid) {
      DeleteDirectory(m_path);
    }
    m_valid = false;
  }

  char m_path[PATH_MAX];
  bool m_valid;
};

bool GetTemporaryFSSpec(const char *name, char *path, UINT pathSize) {
  static AutoTemporarySubfolderClass s_folder;

  if (!s_folder.m_valid) {
    return false;
  }

  snprintf(path, pathSize, "%s/%s", s_folder.m_path, name);
  return true;
}
