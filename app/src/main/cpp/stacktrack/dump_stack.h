#ifndef STACKTRACE_DUMP_STACK_H
#define STACKTRACE_DUMP_STACK_H

#include <unwind.h>
#include <dlfcn.h>

#define DEFAULT_MAX_STACK_STR_SIZE 1024 * 10
#define DEFAULT_MAX_STACK_SIZE 50

typedef struct {
    void **pcStart;
    void **pcEnd;
} DumpStackPcState;

typedef struct DumpStackResult {
    int stackSize = 0;
    int stackStrSize = 0;
    int maxStackSize = DEFAULT_MAX_STACK_SIZE;
    int maxStackStrSize = DEFAULT_MAX_STACK_STR_SIZE;
    char *stacks;
} DumpStackResult;

typedef struct StringsOffsetsResult {
    int *offsets;
    int offsetsSize;
    int maxOffsetsSize;
} StringsOffsetsResult;

/**
 * dump thread stack.
 * @param maxSize max stack stackSize.
 * @param result output result.
 * @return the stack stackSize
 */
void dumpStack(DumpStackResult* result, int skip);

void computeStringsOffsets(const char *strings, StringsOffsetsResult *result);

void printStackResult(DumpStackResult* result);

#endif //STACKTRACE_DUMP_STACK_H
