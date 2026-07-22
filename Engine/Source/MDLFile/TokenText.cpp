namespace MDL {

  static char s_singletoktbl[256][2];

  void __fastcall InitializeTokenText() {
    unsigned int index = 256;

    do {
      --index;
      s_singletoktbl[index][0] = static_cast<char>(index);
    } while (index);
  }

  void __fastcall DestroyTokenText() {
  }

}  // namespace MDL
