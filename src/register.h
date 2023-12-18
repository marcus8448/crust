#ifndef REGISTER_H
#define REGISTER_H
#include "types.h"

#include <stdint.h>

enum RegisterFP {
  // don't overlap with normal registers
  xmm0 = 16,
  xmm1,
  xmm2,
  xmm3,
  xmm4,
  xmm5,
  xmm6,
  xmm7,
  xmm8,
  xmm9,
  xmm10,
  xmm11,
  xmm12,
  xmm13,
  xmm14,
  xmm15
};

enum Register64 { rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, r8, r9, r10, r11, r12, r13, r14, r15 };

enum Register32 {
  eax,
  ebx,
  ecx,
  edx,
  esi,
  edi,
  ebp,
  esp,
  r8d,
  r9d,
  r10d,
  r11d,
  r12d,
  r13d,
  r14d,
  r15d,
};

enum Register16 {
  ax,
  bx,
  cx,
  dx,
  si,
  di,
  bp,
  sp,
  r8w,
  r9w,
  r10w,
  r11w,
  r12w,
  r13w,
  r14w,
  r15w,
};

enum Register8 {
  al,
  bl,
  cl,
  dl,
  sil,
  dil,
  bpl,
  spl,
  r8b,
  r9b,
  r10b,
  r11b,
  r12b,
  r13b,
  r14b,
  r15b,
};

char mnemonic_suffix(Width size);
char fp_mnemonic_suffix(Width size);
const char* get_register_mnemonic(Width size, uint8_t index);

extern const char* mnemonicFP[16];
extern const char* mnemonic64[16];
extern const char* mnemonic32[16];
extern const char* mnemonic16[16];
extern const char* mnemonic8[16];
extern uint8_t argumentRegisters[6];
extern uint8_t registerPriority[14];
extern uint8_t calleeSavedRegisters[7];
extern uint8_t callerSavedRegisters[9];
#endif // REGISTER_H
