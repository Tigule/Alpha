#ifndef STORM_H_SZIP_H
#define STORM_H_SZIP_H

#include <stdio.h>

class WowFile;

class WowFileSystemProvider {
 public:
  WowFileSystemProvider() {
  }

  ~WowFileSystemProvider() {
  }
  virtual WowFile *Open(const char *filename) = 0;
  virtual bool     Close(WowFile *file) = 0;
};

class WowFile {
 public:
  WowFile(WowFileSystemProvider *provider) : m_provider(provider) {
  }

  ~WowFile() {
  }

  bool Close() {
    return m_provider->Close(this);
  }

  WowFileSystemProvider *m_provider;
};

class TestFile : public WowFile {
 public:
  TestFile(WowFileSystemProvider *provider, FILE *file);

  FILE *m_f;
};

class TestFileSystemProvider : public WowFileSystemProvider {
 public:
  TestFileSystemProvider();
  ~TestFileSystemProvider();
  virtual WowFile *Open(const char *filename);
  virtual bool     Close(WowFile *f);
};

class WowFileSystem {
 public:
  WowFileSystem() : m_providerList(0) {
  }

  ~WowFileSystem() {
  }
  void     RegisterProvider(WowFileSystemProvider &provider);
  void     UnregisterProvider(WowFileSystemProvider &provider);
  WowFile *Open(const char *filename);

  WowFileSystemProvider *m_providerList;
};

#endif
