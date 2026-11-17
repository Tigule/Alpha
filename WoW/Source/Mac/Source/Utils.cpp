static unsigned int s_vcRandSeed = 1;

extern "C" int vc_rand() {
  s_vcRandSeed = s_vcRandSeed * 214013 + 2531011;
  return (s_vcRandSeed >> 16) & 0x7FFF;
}
