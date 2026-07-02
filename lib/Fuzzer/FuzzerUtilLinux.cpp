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
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fuzzer {

int ExecuteCommand(const std::string &Command) {
  if (Command.empty())
    return -1;

  std::vector<std::string> Args;
  std::string Current;
  std::string OutputFile;
  bool Escaping = false;
  bool RedirectStderr = false;
  
  // Parse command as an argv list with allowlist validation.
  // Supports output redirection (> file) and stderr merging (2>&1).
  // Backslash escapes allow literal special characters (e.g. space) inside
  // arguments while rejecting malformed escape sequences.
  size_t I = 0;
  while (I < Command.size()) {
    char C = Command[I];
    unsigned char UC = static_cast<unsigned char>(C);
    
    // Check for output redirection pattern: " > filename"
    if (C == '>' && !Escaping && Current.empty()) {
      // Skip whitespace before '>'
      size_t Start = I;
      while (Start > 0 && isspace(Command[Start - 1]))
        Start--;
      
      // Skip whitespace after '>'
      I++;
      while (I < Command.size() && isspace(Command[I]))
        I++;
      
      // Parse the output filename
      while (I < Command.size()) {
        C = Command[I];
        UC = static_cast<unsigned char>(C);
        
        if (isspace(UC)) {
          // Check if this is followed by "2>&1"
          size_t J = I;
          while (J < Command.size() && isspace(Command[J]))
            J++;
          
          if (J + 4 <= Command.size() && 
              Command.substr(J, 4) == "2>&1") {
            RedirectStderr = true;
            I = J + 4;
          }
          break;
        }
        
        // Validate filename characters
        if (!(isalnum(UC) || C == '/' || C == '.' || C == '_' || C == '-'))
          return -1;
        
        OutputFile.push_back(C);
        I++;
      }
      
      if (OutputFile.empty())
        return -1;
      
      continue;
    }
    
    if (Escaping) {
      if (isalnum(UC) || C == ' ' || C == '/' || C == '.' || C == '_' ||
          C == '-' || C == ':' || C == '=' || C == '\\') {
        Current.push_back(C);
        Escaping = false;
        I++;
        continue;
      }
      return -1;
    }

    if (C == '\\') {
      Escaping = true;
      I++;
      continue;
    }

    if (isspace(UC)) {
      if (!Current.empty()) {
        Args.push_back(Current);
        Current.clear();
      }
      I++;
      continue;
    }

    if (!(isalnum(UC) || C == '/' || C == '.' || C == '_' || C == '-' ||
          C == ':' || C == '='))
      return -1;

    Current.push_back(C);
    I++;
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
    // Handle output redirection in the child process
    if (!OutputFile.empty()) {
      int Fd = open(OutputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (Fd < 0)
        _exit(127);
      
      // Redirect stdout to the file
      if (dup2(Fd, STDOUT_FILENO) < 0) {
        close(Fd);
        _exit(127);
      }
      
      // Redirect stderr to stdout if requested
      if (RedirectStderr) {
        if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0) {
          close(Fd);
          _exit(127);
        }
      }
      
      close(Fd);
    }
    
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
