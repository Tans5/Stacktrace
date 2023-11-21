#ifndef STACKTRACE_DUMP_STACK_H
#define STACKTRACE_DUMP_STACK_H

#include <unwind.h>
#include <dlfcn.h>

#define DEFAULT_MAX_SINGLE_STACK_SIZE 300
#define DEFAULT_MAX_STACK_SIZE 50

typedef struct {
    void **pcStart;
    void **pcEnd;
} DumpStackPcState;

typedef struct DumpStackResult {
    int size = 0;
    int maxSingleStackSize = DEFAULT_MAX_SINGLE_STACK_SIZE;
    int maxStackSize = DEFAULT_MAX_STACK_SIZE;
    char *stacks;
} DumpStackResult;

/**
 * dump thread stack.
 * @param maxSize max stack size.
 * @param result output result.
 * @return the stack size
 */
void dumpStack(DumpStackResult* result);

#endif //STACKTRACE_DUMP_STACK_H
