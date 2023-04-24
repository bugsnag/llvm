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
  freeAndInvalidate((void**)&symbolizeResult->address);
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

SymbolizeResult getSymbolizeResult(const DILineInfo &info, std::string address, bool inlined, bool badAddress) {
  SymbolizeResult result = {0};
  result.inlined = inlined;
  result.badAddress = badAddress;

  result.address = new char[std::strlen(address.c_str()) + 1];
  std::strcpy(result.address, address.c_str());
  
  result.fileName = new char[std::strlen(info.FileName.c_str()) + 1];
  std::strcpy(result.fileName, info.FileName.c_str());

  result.shortFunctionName = new char[std::strlen(info.ShortFunctionName.c_str()) + 1];
  std::strcpy(result.shortFunctionName, info.ShortFunctionName.c_str());

  result.linkageFunctionName = new char[std::strlen(info.LinkageFunctionName.c_str()) + 1];
  std::strcpy(result.linkageFunctionName, info.LinkageFunctionName.c_str());

  result.symbolTableFunctionName = new char[std::strlen(info.SymbolTableFunctionName.c_str()) + 1];
  std::strcpy(result.symbolTableFunctionName, info.SymbolTableFunctionName.c_str());

  result.line = info.Line;
  result.column = info.Column;
  result.startLine = info.StartLine;

  return result;
}

SymbolizeResults BugsnagSymbolize(const char* filePath, bool includeInline, char* addresses[], int addressCount) {
  symbolize::LLVMSymbolizer::Options Opts(symbolize::FunctionNameKind::LinkageName, true, true, false, "");
  symbolize::LLVMSymbolizer Symbolizer(Opts);

  SymbolizeResults retVal = {0};

  std::string moduleName(filePath);
  std::vector<SymbolizeResult> results;

  for (int i = 0; i < addressCount; i++) {
    std::string hexAddress = addresses[i];
    char* addressErr;
    int64_t numericAddress = (int64_t)std::strtoul(hexAddress.c_str(), &addressErr, 16);

    if (includeInline) {
      auto ResOrErr = Symbolizer.symbolizeInlinedCode(moduleName, numericAddress);
      if (ResOrErr) {
        for (int j = 0; j < ResOrErr.get().getNumberOfFrames(); j++) {
          SymbolizeResult result = getSymbolizeResult(ResOrErr.get().getFrame(j), hexAddress, (j == 0) ? false: true, *addressErr != 0);
          results.push_back(result);
        }
      }
    } else {
      auto ResOrErr = Symbolizer.symbolizeCode(moduleName, numericAddress);
      if (ResOrErr) {
        SymbolizeResult result = getSymbolizeResult(ResOrErr.get(), hexAddress, false, *addressErr != 0);
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
