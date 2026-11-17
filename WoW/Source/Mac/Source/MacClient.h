#ifndef WOW_SOURCE_MAC_SOURCE_MACCLIENT_H
#define WOW_SOURCE_MAC_SOURCE_MACCLIENT_H

class MacClient {
 public:
  static void SetupCommandLine(int argc, char **argv);
  static void Initialize();
  static void Destroy();
  static void InitializeMenus();
  static long ShowAboutBox();
};

#endif
