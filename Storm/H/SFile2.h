#ifndef STORM_H_SFILE2_H
#define STORM_H_SFILE2_H

class MD5 {
 public:
  MD5();

  MD5(DWORD a, DWORD b, DWORD c, DWORD d) {
    val[0] = a;
    val[1] = b;
    val[2] = c;
    val[3] = d;
  }

  const MD5 &operator=(const MD5 &copy) {
    val[0] = copy.val[0];
    val[1] = copy.val[1];
    val[2] = copy.val[2];
    val[3] = copy.val[3];
    return *this;
  }

  bool operator==(const MD5 &cmp) {
    return val[0] == cmp.val[0] && val[1] == cmp.val[1] && val[2] == cmp.val[2] && val[3] == cmp.val[3];
  }

  DWORD val[4];
};

#endif
