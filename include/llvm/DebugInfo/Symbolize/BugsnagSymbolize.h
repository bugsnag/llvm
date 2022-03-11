#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DebugInformationData {
  int64_t address;
  bool inlined;
  char* fileName;
  char* shortFunctionName;
  char* linkageFunctionName;
  char* symbolTableFunctionName;
  int line;
  int column;
  int startLine;
} DebugInformationData;

typedef struct WrappedDebugInformationData {
  int resultCount;
  DebugInformationData* results;
  const char* pstrErr;
} WrappedDebugInformationData;

WrappedDebugInformationData BugsnagSymbolize(const char* filePath, bool includeInline, int64_t addresses[], int addressCount);

#ifdef __cplusplus
}
#endif
