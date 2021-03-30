//===- lib/DebugInfo/Symbolize/DIPrinter.cpp ------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the DIPrinter class, which is responsible for printing
// structures defined in DebugInfo/DIContext.h
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/Symbolize/DIPrinter.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/DebugInfo/DIContext.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <sstream>
#include <iomanip>

namespace llvm {
namespace symbolize {

// By default, DILineInfo contains "<invalid>" for function/filename it
// cannot fetch. We replace it to "??" to make our output closer to addr2line.
static const char kDILineInfoBadString[] = "<invalid>";
static const char kBadString[] = "??";

// Prints source code around in the FileName the Line.
void DIPrinter::printContext(const std::string &FileName, int64_t Line) {
  if (PrintSourceContext <= 0)
    return;

  ErrorOr<std::unique_ptr<MemoryBuffer>> BufOrErr =
      MemoryBuffer::getFile(FileName);
  if (!BufOrErr)
    return;

  std::unique_ptr<MemoryBuffer> Buf = std::move(BufOrErr.get());
  int64_t FirstLine =
      std::max(static_cast<int64_t>(1), Line - PrintSourceContext / 2);
  int64_t LastLine = FirstLine + PrintSourceContext;
  size_t MaxLineNumberWidth = std::ceil(std::log10(LastLine));

  for (line_iterator I = line_iterator(*Buf, false);
       !I.is_at_eof() && I.line_number() <= LastLine; ++I) {
    int64_t L = I.line_number();
    if (L >= FirstLine && L <= LastLine) {
      OS << format_decimal(L, MaxLineNumberWidth);
      if (L == Line)
        OS << " >: ";
      else
        OS << "  : ";
      OS << *I << "\n";
    }
  }
}

void DIPrinter::print(const DILineInfo &Info, bool Inlined) {
  OS << "\"inlined\":" << (Inlined ? "true" : "false") << ",";
  if (PrintFunctionNames) {
    OS << "\"shortFunctionName\":" << (Info.ShortFunctionName == kDILineInfoBadString ? "null" : "\"" + escapeJson(Info.ShortFunctionName) + "\"") << ",";
    OS << "\"linkageFunctionName\":" << (Info.LinkageFunctionName == kDILineInfoBadString ? "null" : "\"" + escapeJson(Info.LinkageFunctionName) + "\"") << ",";
    OS << "\"symbolTableFunctionName\":" << (Info.SymbolTableFunctionName == kDILineInfoBadString ? "null" : "\"" + escapeJson(Info.SymbolTableFunctionName) + "\"") << ",";
  }
  OS << "\"fileName\":" << (Info.FileName == kDILineInfoBadString ? "null" : "\"" + escapeJson(Info.FileName) + "\"") << ",";
  OS << "\"startLine\":" << (Info.StartLine ? std::to_string(Info.StartLine) : "null") << ",";
  OS << "\"line\":" << std::to_string(Info.Line) << ",";
  OS << "\"column\":" << std::to_string(Info.Column) << ",";
  OS << "\"discriminator\":" << (Info.Discriminator ? std::to_string(Info.Discriminator) : "null");
}

std::string DIPrinter::escapeJson(const std::string &s) {
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        case '"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if ('\x00' <= *c && *c <= '\x1f') {
                o << "\\u"
                  << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
            } else {
                o << *c;
            }
        }
    }
    return o.str();
}

DIPrinter &DIPrinter::operator<<(const DILineInfo &Info) {
  print(Info, false);
  return *this;
}

DIPrinter &DIPrinter::operator<<(const DIInliningInfo &Info) {
  uint32_t FramesNum = Info.getNumberOfFrames();
  if (FramesNum == 0) {
    print(DILineInfo(), false);
    return *this;
  }
  for (uint32_t i = 0; i < FramesNum; i++)
    print(Info.getFrame(i), i > 0);
  return *this;
}

DIPrinter &DIPrinter::operator<<(const DIGlobal &Global) {
  OS << "\"name\":" << (Global.Name == kDILineInfoBadString ? "null" : "\"" + escapeJson(Global.Name) + "\"") << ",";
  OS << "\"start\":" << std::to_string(Global.Start) << ",";
  OS << "\"size\":" << std::to_string(Global.Size);
  return *this;
}

} // end namespace symbolize
} // end namespace llvm
