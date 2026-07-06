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
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <vector>

namespace fuzzer {
int ExecuteCommand(const std::string &Command) {  
  // Parse command into arguments
  std::vector<std::string> args;
  std::istringstream iss(Command);
  std::string token;
  while (iss >> token) {
    args.push_back(token);
  }
  
  if (args.empty()) {
    return -1;
  }
  
  // Convert to char* array for execvp
  std::vector<char*> argv;
  for (auto &arg : args) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);
  
  pid_t pid = fork();
  if (pid == -1) {
    return -1;
  } else if (pid == 0) {
    // Child process - execute command directly (no shell)
    execvp(argv[0], argv.data());
    // If execvp returns, it failed
    _exit(127);
  } else {
    // Parent process - wait for child
    int status;
    pid_t w;
    do {
      w = waitpid(pid, &status, 0);
    } while (w == -1 && errno == EINTR);
    if (w == -1) {
      return -1;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
  }
}

} // namespace fuzzer

#endif // LIBFUZZER_LINUX
