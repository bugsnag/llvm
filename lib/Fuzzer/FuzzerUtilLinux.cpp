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

#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fuzzer {

int ExecuteCommand(const std::string &Command) {
  if (Command.empty())
    return -1;

  std::vector<std::string> Args;
  std::string Current;
  bool Escaping = false;
  // Parse command as an argv list with allowlist validation.
  // Backslash escapes allow literal special characters (e.g. space) inside
  // arguments while rejecting malformed escape sequences.
  for (char C : Command) {
    unsigned char UC = static_cast<unsigned char>(C);
    if (Escaping) {
      if (isalnum(UC) || C == ' ' || C == '/' || C == '.' || C == '_' ||
          C == '-' || C == ':' || C == '=' || C == '\\') {
        Current.push_back(C);
        Escaping = false;
        continue;
      }
      return -1;
    }

    if (C == '\\') {
      Escaping = true;
      continue;
    }

    if (isspace(UC)) {
      if (!Current.empty()) {
        Args.push_back(Current);
        Current.clear();
      }
      continue;
    }

    if (!(isalnum(UC) || C == '/' || C == '.' || C == '_' || C == '-' ||
          C == ':' || C == '='))
      return -1;

    Current.push_back(C);
  }
  if (Escaping)
    return -1;
  if (!Current.empty())
    Args.push_back(Current);
  if (Args.empty())
    return -1;

  std::vector<char *> Argv;
  Argv.reserve(Args.size() + 1);
  for (auto &Arg : Args)
    Argv.push_back(const_cast<char *>(Arg.c_str()));
  Argv.push_back(nullptr);

  pid_t Pid = fork();
  if (Pid < 0)
    return -1;

  if (Pid == 0) {
    execvp(Argv[0], Argv.data());
    _exit(127);
  }

  int Status = 0;
  while (waitpid(Pid, &Status, 0) == -1) {
    if (errno != EINTR)
      return -1;
  }
  return Status;
}

} // namespace fuzzer

#endif // LIBFUZZER_LINUX
