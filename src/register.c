#include "register.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LIST_IMPL(StackAlloc, stackalloc, ValueRef)

int nullable_strcmp(char const* a, char const* b) {
  if (a == NULL)
    return 1;
  if (b == NULL)
    return -1;
  return strcmp(a, b);
}

void stackframe_init(StackFrame* frame, const StackFrame* parent) {
  // TODO: rbx, rsp, rbp, r12, r13, r14, and r15
  stackalloclist_init(&frame->allocations, 2);
  frame->parent = parent;
  for (int i = 0; i < 16; i++) {
    if (parent != NULL) {
      frame->activeRegisters[i] = parent->activeRegisters[i];
    } else {
      frame->activeRegisters[i] = false;
    }
  }
  if (parent != NULL) {
    frame->curOffset = parent->curOffset;
    for (int i = 0; i < parent->allocations.len; i++) {
      stackalloclist_add(&frame->allocations, parent->allocations.array[i]);
    }
  } else {
    frame->curOffset = 0;
  }
}

ValueRef* stackframe_claim_or_copy_from(StackFrame* frame, const Variable variable, const char reg, FILE* output) {
  if (!frame->activeRegisters[reg]) {
    for (int i = 0; i < frame->allocations.len; ++i) {
      if (nullable_strcmp(variable.name, frame->allocations.array[i].name) == 0) {
        if (frame->allocations.array[i].reg != -1) {
          frame->activeRegisters[frame->allocations.array[i].reg] = false;
        }
        frame->allocations.array[i].reg = reg;
        frame->activeRegisters[reg] = true;
        return &frame->allocations.array[i];
      }
    }

    ValueRef ref;
    ref_init_var(&ref, variable);

    ref.reg = reg;
    frame->activeRegisters[reg] = true;

    stackalloclist_add(&frame->allocations, ref);
    return &frame->allocations.array[frame->allocations.len - 1];
  }
  for (int i = 0; i < frame->allocations.len; ++i) {
    if (nullable_strcmp(variable.name, frame->allocations.array[i].name) == 0) {
      if (frame->allocations.array[i].reg != -1) {
        if (frame->allocations.array[i].reg != reg) {
          const Width size = typekind_width(variable.type.kind);
          fprintf(output, "mov%c %s, %s\n", get_suffix(size), get_register_mnemonic(size, reg),
                  get_register_mnemonic(size, frame->allocations.array[i].reg));
        }
      } else {
        frame->allocations.array[i].reg = stackframe_make_register_available(frame, output);
        frame->activeRegisters[frame->allocations.array[i].reg] = true;
        const Width size = typekind_width(variable.type.kind);
        fprintf(output, "mov%c %s, %s\n", get_suffix(size), get_register_mnemonic(size, reg),
                get_register_mnemonic(size, frame->allocations.array[i].reg));
        return &frame->allocations.array[i];
      }
    }
  }

  ValueRef ref;
  ref_init_var(&ref, variable);
  stackframe_set_register(frame, &ref, stackframe_make_register_available(frame, output));

  const Width size = typekind_width(variable.type.kind);
  fprintf(output, "mov%c %s, %s\n", get_suffix(size), get_register_mnemonic(size, reg),
          get_register_mnemonic(size, ref.reg));
  stackalloclist_add(&frame->allocations, ref);
  return &frame->allocations.array[frame->allocations.len - 1];
}

ValueRef* stackframe_allocate(StackFrame* frame, const Variable variable) {
  for (int i = 0; i < frame->allocations.len; ++i) {
    assert(frame->allocations.array[i].type != ref_constant);
    if (nullable_strcmp(variable.name, frame->allocations.array[i].name) == 0) {
      exit(54);
      return &frame->allocations.array[i];
    }
  }

  ValueRef ref;
  ref_init_var(&ref, variable);

  stackalloclist_add(&frame->allocations, ref);
  return &frame->allocations.array[frame->allocations.len - 1];
}

ValueRef* stackframe_allocate_temporary(StackFrame* frame, Type type, FILE* output) {
  ValueRef ref;
  ref_init_temp(&ref, type);
  stackframe_set_register(frame, &ref, stackframe_make_register_available(frame, output));
  stackalloclist_add(&frame->allocations, ref);
  return &frame->allocations.array[frame->allocations.len - 1];
}

void stackframe_allocate_temporary_from(StackFrame* frame, ValueRef** maybe_temp, FILE* output) {
  switch ((*maybe_temp)->type) {
  case ref_temporary:
    break;
  case ref_variable:
  case ref_constant: {
    ValueRef* temp = stackframe_allocate_temporary(frame, (*maybe_temp)->value_type, output);
    char* mnemonicN = allocation_mnemonic(temp);
    char* mnemonicL = allocation_mnemonic(*maybe_temp);
    fprintf(output, "mov%c %s, %s\n", get_suffix(typekind_width(temp->value_type.kind)), mnemonicL, mnemonicN);
    free(mnemonicL);
    free(mnemonicN);
    if ((*maybe_temp)->type == ref_constant)
      stackframe_free_ref(frame, *maybe_temp);
    *maybe_temp = temp;
  } break;
  }
}

ValueRef* stackframe_allocate_variable_from(StackFrame* frame, ValueRef* value, Variable variable, FILE* output) {
  switch (value->type) {
  case ref_temporary: {
    value->name = variable.name;
    value->value_type = variable.type;
    value->type = ref_variable;
    return value;
  }
  case ref_constant: {
    ValueRef* temp = stackframe_allocate(frame, variable);
    stackframe_set_register(frame, temp, stackframe_make_register_available(frame, output));

    char* mnemonicN = allocation_mnemonic(temp);
    char* mnemonicL = allocation_mnemonic(value);
    fprintf(output, "mov%c %s, %s # store variable %s \n", get_suffix(typekind_width(variable.type.kind)), mnemonicL,
            mnemonicN, variable.name);
    free(mnemonicL);
    free(mnemonicN);
    stackframe_free_ref(frame, value);
    return temp;
  }
  case ref_variable: {
    ValueRef* temp = stackframe_allocate(frame, variable);

    char* mnemonicN = allocation_mnemonic(temp);
    char* mnemonicL = allocation_mnemonic(value);
    fprintf(output, "mov%c %s, %s\n # copy variable %s into %s", get_suffix(typekind_width(variable.type.kind)),
            mnemonicL, mnemonicN, value->name, variable.name);
    free(mnemonicL);
    free(mnemonicN);
    return temp;
  }
  }
  exit(53);
}

void stackframe_free_ref(StackFrame* frame, ValueRef* ref) {
  switch (ref->type) {
  case ref_variable:
  case ref_temporary:
    if (ref->reg != -1) {
      frame->activeRegisters[ref->reg] = false;
      ref->reg = -1;
    }
    break;
  case ref_constant:
    free(ref->repr);
    free(ref);
    break;
  }
}

ValueRef* stackframe_get(StackFrame* frame, const Variable variable) {
  for (int i = 0; i < frame->allocations.len; ++i) {
    assert(frame->allocations.array[i].type != ref_constant);
    if (nullable_strcmp(variable.name, frame->allocations.array[i].name) == 0) {
      return &frame->allocations.array[i];
    }
  }
  return NULL;
}

ValueRef* stackframe_get_by_name(StackFrame* frame, const char* name) {
  for (int i = 0; i < frame->allocations.len; ++i) {
    assert(frame->allocations.array[i].type != ref_constant);
    if (strcmp(name, frame->allocations.array[i].name) == 0) {
      return &frame->allocations.array[i];
    }
  }
  return NULL;
}

ValueRef* stackframe_get_by_token(const StackFrame* frame, const char* contents, const Token* id) {
  for (int i = 0; i < frame->allocations.len; ++i) {
    assert(frame->allocations.array[i].type != ref_constant);

    if (frame->allocations.array[i].name != NULL) {
      bool pass = false;
      for (int j = 0; j < id->len; j++) {
        if (contents[id->index + j] != frame->allocations.array[i].name[j]) {
          pass = false;
          break;
        }
        pass = true;
      }
      if (pass) {
        return &frame->allocations.array[i];
      }
    }
  }
  return NULL;
}

char* allocation_mnemonic(const ValueRef* alloc) {
  if (alloc->type == ref_constant) {
    return strdup(alloc->repr);
  }
  if (alloc->reg != -1) {
    return strdup(get_register_mnemonic(typekind_width(alloc->value_type.kind), alloc->reg));
  }

  assert(alloc->on_stack);
  int len = snprintf(NULL, 0, "%i(%%rbp)", alloc->offset);
  char* mnemonic = malloc(len + 1);
  snprintf(mnemonic, len, "%i(%%rbp)", alloc->offset);
  mnemonic[len] = '\0';
  return mnemonic;
}

char* stackframe_mnemonic(StackFrame* frame, const Variable variable) {
  return allocation_mnemonic(stackframe_get(frame, variable));
}

char stackframe_make_register_available(StackFrame* frame, FILE* output) {
  for (int i = 0; i < 16; i++) {
    if (i == rbp || i == rsp || i == rbx)
      continue;
    if (!frame->activeRegisters[i]) {
      return i;
    }
  }

  for (int i = 0; i < frame->allocations.len; i++) {
    ValueRef alloc = frame->allocations.array[i];
    if (alloc.type != ref_constant && alloc.reg != -1) {
      const Width size = typekind_width(alloc.value_type.kind);
      if (!alloc.on_stack) {
        alloc.on_stack = true;
        frame->curOffset -= size_bytes(size);
        alloc.offset = frame->curOffset;
      }
      fprintf(output, "mov%c %s, %i(%s)\n", get_suffix(size), get_register_mnemonic(size, alloc.reg), alloc.offset,
              "%rbp");

      frame->activeRegisters[alloc.reg] = false;
      alloc.reg = -1;
      return i;
    }
  }
  // impossible
  abort();
}

void stackframe_load_arguments(StackFrame* frame, const Function* function) {
  // rdi, rsi, rdx, rcx, r8, r9 -> stack
  for (int i = 0; i < function->arguments.len; i++) {
    ValueRef ref;
    ref_init_var(&ref, function->arguments.array[i]);
    if (i < 6) {
      stackframe_set_register(frame, &ref, argumentRegisters[i]);
    } else {
      ref.on_stack = true;
      ref.offset = 16 * (i - 6);
    }
    stackalloclist_add(&frame->allocations, ref);
  }
}

void stackframe_moveto_register(StackFrame* frame, ValueRef* alloc, FILE* output) {
  assert(alloc->type != ref_constant);
  if (alloc->reg == -1) {
    alloc->reg = stackframe_make_register_available(frame, output);
    const Width size = typekind_width(alloc->value_type.kind);
    fprintf(output, "mov%c %i(%%rbp), %s\n", get_suffix(size), alloc->offset, get_register_mnemonic(size, alloc->reg));
  }
}

void stackframe_force_into_register(StackFrame* frame, ValueRef* alloc, const char reg, FILE* output) {
  if (alloc->reg == reg)
    return;
  if (frame->activeRegisters[reg]) {
    for (int i = 0; i < frame->allocations.len; ++i) {
      if (frame->allocations.array[i].reg == reg) {
        stackframe_moveto_stack(frame, &frame->allocations.array[i], output);
        frame->allocations.array[i].reg = -1;
        break;
      }
    }
  }

  Width size = typekind_width(alloc->value_type.kind);
  if (alloc->type == ref_temporary)
    size = Quad;
  fprintf(output, "mov%c %s, %s\n", get_suffix(size), get_register_mnemonic(size, alloc->reg),
          get_register_mnemonic(size, reg));
  alloc->reg = reg;
}

void stackframe_set_or_copy_register(StackFrame* frame, ValueRef* ref, char reg, FILE* output) {
  if (!frame->activeRegisters[reg]) {
    ref->reg = reg;
    frame->activeRegisters[reg] = true;
  } else {
    ref->reg = stackframe_make_register_available(frame, output);
    frame->activeRegisters[ref->reg] = true;
    const Width size = typekind_width(ref->value_type.kind);
    fprintf(output, "mov%c %s, %s\n", get_suffix(size), get_register_mnemonic(size, reg),
            get_register_mnemonic(size, ref->reg));
  }
}

void stackframe_moveto_stack(StackFrame* frame, ValueRef* ref, FILE* output) {
  if (ref->type == ref_constant)
    return;
  Width size = typekind_width(ref->value_type.kind);
  if (ref->type == ref_temporary)
    size = Quad; // fixme
  if (!ref->on_stack) {
    ref->on_stack = true;
    frame->curOffset -= size_bytes(size);
    ref->offset = frame->curOffset;
  }

  if (ref->reg != -1) {
    fprintf(output, "mov%c %s, %i(%%rbp) # store variable '%s' to stack\n", get_suffix(size),
            get_register_mnemonic(size, ref->reg), ref->offset, ref->name);
  }
  ref->reg = -1;
}

void stackframe_set_register(StackFrame* frame, ValueRef* ref, const char reg) {
  assert(!frame->activeRegisters[reg]);
  frame->activeRegisters[reg] = true;
  ref->reg = reg;
}

void stackframe_moveto_register_v(StackFrame* frame, const Variable variable, FILE* output) {
  stackframe_moveto_register(frame, stackframe_get(frame, variable), output);
}

void stackframe_free(const StackFrame* frame, FILE* output) {
  if (frame->parent == NULL)
    return;
  for (int i = 0; i < frame->parent->allocations.len; i++) {
    frame->parent->allocations.array[i] = frame->allocations.array[i]; // pass changes to previous variables
  }
}

char ref_get_register(const ValueRef* ref) {
  switch (ref->type) {
  case ref_variable:
  case ref_temporary:
    return ref->reg;
  default:
    return -1;
  }
}

void ref_init_var(ValueRef* ref, Variable variable) {
  ref->type = ref_variable;
  ref->name = variable.name;
  ref->value_type = variable.type;
  ref->on_stack = false;
  ref->offset = 0;
  ref->reg = -1;
}

void ref_init_temp(ValueRef* ref, Type type) {
  ref->type = ref_temporary;
  ref->name = NULL;
  ref->value_type = type;
  ref->on_stack = false;
  ref->offset = 0;
}

const char* get_register_mnemonic(const Width size, const int index) {
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

  return NULL;
}

char get_suffix(const Width size) {
  switch (size) {
  case Byte:
    return 'b';
  case Word:
    return 's';
  case Long:
    return 'l';
  case Quad:
    return 'q';
  }
  exit(30);
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

int argumentRegisters[6] = {rdi, rsi, rdx, rcx, r8, r9};
