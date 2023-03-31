
#include <cstdio>
#include <cstdlib>


namespace simprpc{
  void __bad_assertion(const char* msg) {
    fputs(msg, stderr);
    abort();
  }
}