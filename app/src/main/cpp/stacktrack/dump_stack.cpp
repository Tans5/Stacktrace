#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "dump_stack.h"
#include "../android_log.h"

static _Unwind_Reason_Code singleStackPcUnwind(_Unwind_Context *ctx, void *pcState) {
    auto* state = static_cast<DumpStackPcState *> (pcState);
    uintptr_t pc = _Unwind_GetIP(ctx);
    int pcOffset = 0;
#if defined(__arm__)
    pcOffset = -4;
#elif defined(__aarch64__)
    pcOffset = -4;
#endif
    if (pc) {
        if (state->pcStart == state->pcEnd) {
            return _URC_END_OF_STACK;
        } else {
            *state->pcStart++ = reinterpret_cast<void*>(pc + pcOffset);
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
    int size = s.pcStart - pcBuffer;
    if (size > 0) {
        Dl_info dl_info;
        int realWriteSize = 0;
        int wroteOffsetInBytes = 0;
        for (int i = 0; i < size; i ++) {
            int indexInStack = i - skip;
            if (indexInStack < 0) {
                continue;
            }
            void* inst_addr_in_mem = pcBuffer[i];
            char *target_output = result->stacks + wroteOffsetInBytes;
            int thisTimeWroteSizeInBytes;
            if (dladdr(inst_addr_in_mem, &dl_info)) {
                const char *so_file_path = dl_info.dli_fname;
                const char *method_name = dl_info.dli_sname;
                long text_addr_in_mem = (long)dl_info.dli_fbase;
                long method_addr_in_mem = (long)dl_info.dli_saddr;
                long inst_addr_in_text = (long)inst_addr_in_mem - text_addr_in_mem;
                long inst_offset = (long)inst_addr_in_mem - method_addr_in_mem;
                if (!method_name) {
                    thisTimeWroteSizeInBytes = sprintf(target_output, "#%02d pc %016lx  %s", indexInStack, inst_addr_in_text, so_file_path);
                } else {
                    thisTimeWroteSizeInBytes = sprintf(target_output, "#%02d pc %016lx  %s (%s+%ld)", indexInStack, inst_addr_in_text, so_file_path, method_name, inst_offset);
                }
            } else {
                thisTimeWroteSizeInBytes = sprintf(target_output, "#%02d", indexInStack);
            }
            target_output[thisTimeWroteSizeInBytes] = 0x00;
            thisTimeWroteSizeInBytes += 1;
            wroteOffsetInBytes += thisTimeWroteSizeInBytes;
            realWriteSize ++;
            if (wroteOffsetInBytes >= result->maxStackStrSize) {
                result->stacks[result->maxStackStrSize - 1] = 0x00;
                break;
            }
        }
        if (realWriteSize < size) {
            size = realWriteSize;
        }
        result->stackSize = size;
        result->stackStrSize = wroteOffsetInBytes;
    } else {
        result->stackSize = 0;
        result->stackStrSize = 0;
    }
}

void computeStringsOffsets(const char *strings, int maxCharsSize, StringsOffsetsResult *result) {
    result->offsets[0] = 0;
    int strOffsetsIndex = 0;
    for (int i = 0; i < maxCharsSize; i ++) {
        if (strings[i] == 0x00) {
            result->offsets[++strOffsetsIndex] = (i + 1);
            if (strOffsetsIndex >= (result->maxOffsetsSize - 1)) {
                break;
            }
        }
    }
    result->offsetsSize = strOffsetsIndex + 1;
}

void printStackResult(DumpStackResult *result) {
    StringsOffsetsResult offsetsResult;
    offsetsResult.maxOffsetsSize = result->stackSize;
    offsetsResult.offsets = static_cast<int *>(malloc(sizeof(int) * result->stackSize));
    computeStringsOffsets(result->stacks, result->stackStrSize, &offsetsResult);
    for (int i = 0; i < offsetsResult.offsetsSize + 1; i ++) {
        char *str = result->stacks + offsetsResult.offsets[i];
        LOGE("%s", str);
    }
    free(offsetsResult.offsets);
}