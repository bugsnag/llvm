//===- FuzzerUtilLinux.cpp - Misc utils for Linux. ------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Misc utils for Linux.
//===----------------------------------------------------------------------===//
#include "FuzzerDefs.h"
#if LIBFUZZER_LINUX

#include <stdlib.h>

namespace fuzzer {

int ExecuteCommand(const std::string &Command) {
  // Reject commands with shell metacharacters to prevent command injection (CWE-78, CWE-88)
  if (Command.find_first_of(";|$`<()\\\"'*?[]{}!~\r") != std::string::npos)
    return -1;
  return system(Command.c_str());
}

} // namespace fuzzer

#endif // LIBFUZZER_LINUX