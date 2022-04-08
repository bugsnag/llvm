#include "llvm/DebugInfo/Symbolize/BugsnagSymbolize.h"
#include "llvm/DebugInfo/Symbolize/Symbolize.h"
#include <cstdlib>
#include <cstring>

using namespace llvm;

static void freeAndInvalidate(void** p) {
  free((void*)*p);
  *p = nullptr;
}

static void destroySymbolizeResult(SymbolizeResult* symbolizeResult) {
  if (!symbolizeResult) {
    return;
  }
  freeAndInvalidate((void**)&symbolizeResult->fileName);
  freeAndInvalidate((void**)&symbolizeResult->shortFunctionName);
  freeAndInvalidate((void**)&symbolizeResult->linkageFunctionName);
  freeAndInvalidate((void**)&symbolizeResult->symbolTableFunctionName);
}

static void destroySymbolizeResults(SymbolizeResults* symbolizeResults) {
  if (!symbolizeResults) {
    return;
  }
  for (int i = 0; i < symbolizeResults->resultCount; i++) {
    destroySymbolizeResult(&symbolizeResults->results[i]);
  }
}

SymbolizeResult getSymbolizeResult(const DILineInfo &info, int64_t address, bool inlined) {
  SymbolizeResult result = {{0}};

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

SymbolizeResults BugsnagSymbolize(const char* filePath, bool includeInline, int64_t addresses[], int addressCount) {
  symbolize::LLVMSymbolizer::Options Opts(symbolize::FunctionNameKind::LinkageName, true, true, false, "");
  symbolize::LLVMSymbolizer Symbolizer(Opts);

  SymbolizeResults retVal = {{0}};

  std::string moduleName(filePath);
  std::vector<SymbolizeResult> results;

  for (int i = 0; i < addressCount; i++) {
    auto ResOrErr = Symbolizer.symbolizeCode(moduleName, addresses[i]);
    if (includeInline) {
      auto ResOrErr = Symbolizer.symbolizeInlinedCode(moduleName, addresses[i]);
      if (ResOrErr) {
        for (int j = 0; j < ResOrErr.get().getNumberOfFrames(); j++) {
          SymbolizeResult result = getSymbolizeResult(ResOrErr.get().getFrame(j), addresses[i], (j == 0) ? false: true);
          results.push_back(result);
        }
      }
    } else {
      auto ResOrErr = Symbolizer.symbolizeCode(moduleName, addresses[i]);
      if (ResOrErr) {
        SymbolizeResult result = getSymbolizeResult(ResOrErr.get(), addresses[i], false);
        results.push_back(result);
      }
    }
  }

  retVal.resultCount = results.size();
  retVal.results = new SymbolizeResult[results.size()];
  std::copy(results.begin(), results.end(), retVal.results);

  return retVal;
}

void DestroySymbolizeResults(SymbolizeResults* symbolizeResults) {
  if (symbolizeResults) {
    destroySymbolizeResults(symbolizeResults);
  }
}
