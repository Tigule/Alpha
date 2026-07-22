#include <storm.h>

int main(void) {
  char *test = (char *)SMemAlloc(256);
  test = (char *)SMemReAlloc(test, 512);
  SMemFree(test);

  return 0;
}
