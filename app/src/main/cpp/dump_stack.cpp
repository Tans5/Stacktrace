#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "dump_stack.h"
#include "android_log.h"

static _Unwind_Reason_Code singleStackPcUnwind(_Unwind_Context *ctx, void *pcState) {
    auto* state = static_cast<DumpStackPcState *> (pcState);
    uintptr_t pc = _Unwind_GetIP(ctx);
    if (pc) {
        if (state->pcStart == state->pcEnd) {
            return _URC_END_OF_STACK;
        } else {
            *state->pcStart++ = reinterpret_cast<void*>(pc);
        }
    }
    return _URC_NO_REASON;
}

static void dumpStackPc(DumpStackPcState* state) {
    _Unwind_Backtrace(singleStackPcUnwind, state);
}

void dumpStack(DumpStackResult* result, int skip) {
    void *pcBuffer[result->maxStackSize];
    DumpStackPcState s = { pcBuffer, pcBuffer + result->maxStackSize };
    dumpStackPc(&s);
    int size = s.pcStart - pcBuffer - skip;
    if (size > 0) {
        Dl_info dl_info;
        for (int i = 0; i < size; i ++) {
            int indexInStack = i - skip;
            if (indexInStack < 0) {
                continue;
            }
            void* inst_addr_in_mem = pcBuffer[i];
            int offset = result->maxSingleStackSize * i;
            char *target_output = result->stacks + offset;
            if (dladdr(inst_addr_in_mem, &dl_info)) {
                const char *so_file_path = dl_info.dli_fname;
                const char *method_name = dl_info.dli_sname;
                long text_addr_in_mem = (long)dl_info.dli_fbase;
                long method_addr_in_mem = (long)dl_info.dli_saddr;
                long inst_addr_in_text = (long)inst_addr_in_mem - text_addr_in_mem;
                long inst_offset = (long)inst_addr_in_mem - method_addr_in_mem;
                if (!method_name) {
                    sprintf(target_output, "#%02d pc %016lx  %s", indexInStack, inst_addr_in_text, so_file_path);
                } else {
                    sprintf(target_output, "#%02d pc %016lx  %s (%s+%ld)", indexInStack, inst_addr_in_text, so_file_path, method_name, inst_offset);
                }
            } else {
                sprintf(target_output, "#%02d", i);
            }
        }
        result->size = size;
    } else {
        result->size = 0;
        result->stacks = nullptr;
    }
}

void printStackResult(DumpStackResult *result) {
    char *tempStr = static_cast<char *>(malloc(result->maxSingleStackSize));
    for (int i = 0; i < result->size; i ++) {
        char *str = result->stacks + i * result->maxSingleStackSize;
        memcpy(tempStr, str, result->maxSingleStackSize);
        LOGE("%s", tempStr);
    }
    free(tempStr);
}