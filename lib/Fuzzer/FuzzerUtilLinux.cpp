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
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <vector>

namespace fuzzer {
int ExecuteCommand(const std::string &Command) {
  // Parse command into program arguments and optional output redirection.
  // Supports "> file" and "2>&1" at the end of the command string; these
  // are implemented via open()/dup2() in the child, without invoking a shell.
  std::vector<std::string> args;
  std::string outFile;
  bool redirectErrToOut = false;

  std::istringstream iss(Command);
  std::string token;
  while (iss >> token) {
    if (token == ">") {
      if (!(iss >> outFile))
        return -1;
    } else if (token == "2>&1") {
      redirectErrToOut = true;
    } else {
      args.push_back(token);
    }
  }

  if (args.empty())
    return -1;

  // Convert to char* array for execvp
  std::vector<char *> argv;
  for (auto &arg : args)
    argv.push_back(const_cast<char *>(arg.c_str()));
  argv.push_back(nullptr);

  pid_t pid = fork();
  if (pid == -1) {
    return -1;
  } else if (pid == 0) {
    // Child process: set up redirections, then exec without a shell.
    if (!outFile.empty()) {
      int fd = open(outFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd == -1)
        _exit(127);
      if (dup2(fd, STDOUT_FILENO) == -1)
        _exit(127);
      close(fd);
    }
    if (redirectErrToOut) {
      if (dup2(STDOUT_FILENO, STDERR_FILENO) == -1)
        _exit(127);
    }
    execvp(argv[0], argv.data());
    _exit(127);
  } else {
    // Parent process: wait for child.
    int status;
    pid_t w;
    do {
      w = waitpid(pid, &status, 0);
    } while (w == -1 && errno == EINTR);
    if (w == -1)
      return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
  }
}

} // namespace fuzzer

#endif // LIBFUZZER_LINUX
