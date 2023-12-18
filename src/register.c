#include "register.h"

#include <stdint.h>

const char* get_register_mnemonic(const Width size, const uint8_t index) {
  if (index < 16) {
    switch (size) {
    case Quad:
      return mnemonic64[index];
    case Long:
      return mnemonic32[index];
    case Word:
      return mnemonic16[index];
    case Byte:
      return mnemonic8[index];
    }
  } else if (index < 32) {
    assert(size == Quad || size == Long);
    return mnemonicFP[index - 16];
  }

  return NULL;
}

char mnemonic_suffix(const Width size) {
  assert(size >= Byte && size <= Quad);
  switch (size) {
  case Byte:
    return 'b';
  case Word:
    return 's';
  case Long:
    return 'l';
  case Quad:
    return 'q';
  default:
    return 'X';
  }
}

char fp_mnemonic_suffix(const Width size) {
  assert(size == Long || size == Quad);
  switch (size) {
  case Long:
    return 's';
  case Quad:
    return 'd';
  default:
    return 'X';
  }
}

const char* mnemonicFP[16] = {"%xmm0", "%xmm1", "%xmm2",  "%xmm3",  "%xmm4",  "%xmm5",  "%xmm6",  "%xmm7",
                              "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14", "%xmm15"};

const char* mnemonic64[16] = {"%rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi", "%rbp", "%rsp",
                              "%r8",  "%r9",  "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"};

const char* mnemonic32[16] = {"%eax", "%ebx", "%ecx",  "%edx",  "%esi",  "%edi",  "%ebp",  "%esp",
                              "%r8d", "%r9d", "%r10d", "%r11d", "%r12d", "%r13d", "%r14d", "%r15d"};

const char* mnemonic16[16] = {"%ax",  "%bx",  "%cx",   "%dx",   "%si",   "%di",   "%bp",   "%sp",
                              "%r8w", "%r9w", "%r10w", "%r11w", "%r12w", "%r13w", "%r14w", "%r15w"};

const char* mnemonic8[16] = {"%al",  "%bl",  "%cl",   "%dl",   "%sil",  "%dil",  "%bpl",  "%spl",
                             "%r8b", "%r9b", "%r10b", "%r11b", "%r12b", "%r13b", "%r14b", "%r15b"};

uint8_t argumentRegisters[6] = {rdi, rsi, rdx, rcx, r8, r9};
// no rbp, rsp
uint8_t registerPriority[14] = {rbx, rcx, rdx, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15, rax};

uint8_t calleeSavedRegisters[7] = {rbp, rbx, r12, r13, r14, r15, rsp};
uint8_t callerSavedRegisters[9] = {rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11};

// /%rbp, %rbx and %r12 through %r15 are callee-save registers. All other registers
