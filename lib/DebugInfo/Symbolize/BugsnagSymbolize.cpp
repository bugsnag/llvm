#include "llvm/DebugInfo/Symbolize/BugsnagSymbolize.h"
#include "llvm/DebugInfo/Symbolize/DIPrinter.h"
#include "llvm/DebugInfo/Symbolize/Symbolize.h"
#include <cstdlib>
#include <cstring>

using namespace llvm;

template<typename T>
static bool error(Expected<T> &ResOrErr) {
  if (ResOrErr)
    return false;
  logAllUnhandledErrors(ResOrErr.takeError(), errs(),
                        "LLVMSymbolizer: error reading file: ");
  return true;
}

DebugInformationData getDebugInformationData(const DILineInfo &info, int64_t address, bool inlined) {
  DebugInformationData result = {{0}};

  result.address = address;
  result.inlined = inlined;

  unsigned int len = std::strlen(info.FileName.c_str());
  result.fileName = new char[len];
  std::strcpy(result.fileName, info.FileName.c_str());

  len = std::strlen(info.ShortFunctionName.c_str());
  result.shortFunctionName = new char[len];
  std::strcpy(result.shortFunctionName, info.ShortFunctionName.c_str());

  len = std::strlen(info.LinkageFunctionName.c_str());
  result.linkageFunctionName = new char[len];
  std::strcpy(result.linkageFunctionName, info.LinkageFunctionName.c_str());

  len = std::strlen(info.SymbolTableFunctionName.c_str());
  result.symbolTableFunctionName = new char[len];
  std::strcpy(result.symbolTableFunctionName, info.SymbolTableFunctionName.c_str());

  result.line = info.Line;
  result.column = info.Column;
  result.startLine = info.StartLine;

  return result;
}

WrappedDebugInformationData BugsnagSymbolize(const char* filePath, bool includeInline, int64_t addresses[], int addressCount) {
  symbolize::DIPrinter Printer(outs(), true, true, true, true);

  symbolize::LLVMSymbolizer::Options Opts(symbolize::FunctionNameKind::LinkageName, true, true, false, "");
  symbolize::LLVMSymbolizer Symbolizer(Opts);

  WrappedDebugInformationData retVal = {{0}};

  std::string moduleName(filePath);
  std::vector<DebugInformationData> results;

  for (int i = 0; i < addressCount; i++) {
    auto ResOrErr = Symbolizer.symbolizeCode(moduleName, addresses[i]);
    if (includeInline) {
      auto ResOrErr = Symbolizer.symbolizeInlinedCode(moduleName, addresses[i]);
      if (ResOrErr) {
        for (int j = 0; j < ResOrErr.get().getNumberOfFrames(); j++) {
          DebugInformationData result = getDebugInformationData(ResOrErr.get().getFrame(j), addresses[i], (j == 0) ? false: true);
          results.push_back(result);
        }
      }

      Printer << (error(ResOrErr) ? DIInliningInfo() : ResOrErr.get());
      outs() << "\n";
      outs().flush();
    } else {
      auto ResOrErr = Symbolizer.symbolizeCode(moduleName, addresses[i]);
      if (ResOrErr) {
        DebugInformationData result = getDebugInformationData(ResOrErr.get(), addresses[i], false);
        results.push_back(result);

        Printer << (error(ResOrErr) ? DILineInfo() : ResOrErr.get());
        outs() << "\n";
        outs().flush();
      }
    }
  }

  retVal.resultCount = results.size();
  retVal.results = new DebugInformationData[results.size()];
  std::copy(results.begin(), results.end(), retVal.results);

  return retVal;
}
