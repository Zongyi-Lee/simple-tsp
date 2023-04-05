
#pragma once

namespace simprpc{
  void __bad_assertion(const char *msg);
}

#define __SIMPRPC_STR(x)  # x
#define __SIMPRPC_XSTR(x)  __SIMPRPC_STR(X)
#define SIMPRPC_ASSERT(expr)                                            \
    ((expr) ? (void)0 :                                                 \
     simprpc::__bad_assertion("Assertion \"" #expr                       \
                             "\" failed, file " __SIMPRPC_XSTR(__FILE__) \
                             ", line " __SIMPRPC_XSTR(__LINE__) "\n"))
