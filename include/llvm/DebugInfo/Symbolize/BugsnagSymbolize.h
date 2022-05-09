#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SymbolizeResult {
  char* address;
  bool inlined;
  char* fileName;
  char* shortFunctionName;
  char* linkageFunctionName;
  char* symbolTableFunctionName;
  int line;
  int column;
  int startLine;
} SymbolizeResult;

typedef struct SymbolizeResults {
  int resultCount;
  SymbolizeResult* results;
} SymbolizeResults;

SymbolizeResults BugsnagSymbolize(const char* filePath, bool includeInline, int64_t addresses[], int addressCount);
void DestroySymbolizeResults(SymbolizeResults* symbolizeResults);

#ifdef __cplusplus
}
#endif
