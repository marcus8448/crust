#include "codegen.h"

#include "register.h"

#include <string.h>

void cullref(Registers *registers, const Instruction *instruction, Reference ref) {
  if (isAllocated(ref.access) && instruction->id == ref.allocation->lastInstr) {
    if (registers_get_storage(registers, ref.allocation)->location != L_None) {
      registers_free_register(registers, ref.allocation);
    }
  }
}

void clear_register(const InstructionTable *table, Registers *registers, int8_t reg, FILE *output) {
  for (int i = 0; i < table->allocations.len; ++i) {
    if (registers->storage[i].location == L_Register) {
      if (registers->storage[i].reg == reg) {
        registers_move_to_stack(registers, table->allocations.array[i], output);
      }
    }
  }
}

void write_jmp(const InstructionTable *table, const char *op, int label, FILE *output) {
  fprintf(output, "\t%s .LBL.%s.%i\n", op, table->name, label);
  fprintf(stdout, "\t%s .LBL.%s.%i\n", op, table->name, label);
}

void write_unary_op(const Registers *registers, const char *op, const Width width, const Reference ref, FILE *output) {
  if (isAllocated(ref.access) && ref.allocation->name != NULL) {
    fprintf(output, "\t%s%c %s # %s\n", op, mnemonic_suffix(width), registers_get_mnemonic(registers, ref),
            ref.allocation->name);
    fprintf(stdout, "\t%s%c %s # %s\n", op, mnemonic_suffix(width), registers_get_mnemonic(registers, ref),
            ref.allocation->name);
    return;
  }
  fprintf(output, "\t%s%c %s\n", op, mnemonic_suffix(width), registers_get_mnemonic(registers, ref));
  fprintf(stdout, "\t%s%c %s\n", op, mnemonic_suffix(width), registers_get_mnemonic(registers, ref));
}

void write_binary_op(const Registers *registers, const char *op, const Width width, const Reference a,
                     const Reference b, FILE *output) {
  if (isAllocated(a.access) && registers_get_storage(registers, a.allocation)->location == L_None) {
    printf("variable at index %i (%s) not allocated!\n", a.allocation->index, a.allocation->name);
  }
  if (isAllocated(b.access) && registers_get_storage(registers, b.allocation)->location == L_None) {
    printf("variable at index %i (%s) not allocated!\n", b.allocation->index, b.allocation->name);
  }
  if ((isAllocated(a.access) && a.allocation->name != NULL) || (isAllocated(b.access) && b.allocation->name != NULL)) {
    fprintf(output, "\t%s%c %s, %s # %s, %s\n", op, mnemonic_suffix(width), registers_get_mnemonic(registers, a),
            registers_get_mnemonic(registers, b), isAllocated(a.access) ? a.allocation->name : "<tmp>",
            isAllocated(b.access) ? b.allocation->name : "<tmp>");
    fprintf(stdout, "\t%s%c %s, %s # %s, %s\n", op, mnemonic_suffix(width), registers_get_mnemonic(registers, a),
            registers_get_mnemonic(registers, b), isAllocated(a.access) ? a.allocation->name : "<tmp>",
            isAllocated(b.access) ? b.allocation->name : "<tmp>");
    return;
  }
  fprintf(output, "\t%s%c %s, %s\n", op, mnemonic_suffix(width), registers_get_mnemonic(registers, a),
          registers_get_mnemonic(registers, b));
  fprintf(stdout, "\t%s%c %s, %s\n", op, mnemonic_suffix(width), registers_get_mnemonic(registers, a),
          registers_get_mnemonic(registers, b));
}

void write_binary_transform_op(const Registers *registers, const char *op, const Width width, const Width width2,
                               const Reference a, const Reference b, FILE *output) {
  if (isAllocated(a.access) && registers_get_storage(registers, a.allocation)->location == L_None) {
    printf("variable at index %i (%s) not allocated!\n", a.allocation->index, a.allocation->name);
  }
  if (isAllocated(b.access) && registers_get_storage(registers, b.allocation)->location == L_None) {
    printf("variable at index %i (%s) not allocated!\n", b.allocation->index, b.allocation->name);
  }
  fprintf(output, "\t%s%c%c %s, %s\n", op, mnemonic_suffix(width), mnemonic_suffix(width2),
          registers_get_mnemonic(registers, a), registers_get_mnemonic(registers, b));
  fprintf(stdout, "\t%s%c%c %s, %s\n", op, mnemonic_suffix(width), mnemonic_suffix(width2),
          registers_get_mnemonic(registers, a), registers_get_mnemonic(registers, b));
}

void write_mov_into_register(const Registers *registers, const Type type, const Reference ref, const int8_t reg,
                             FILE *output) {
  if (!isAllocated(ref.access) || type_width(ref.allocation->type) == type_width(type)) {
    fprintf(output, "\tmov%c %s, %s\n", mnemonic_suffix(type_width(type)), registers_get_mnemonic(registers, ref),
            get_register_mnemonic(type_width(type), reg));
    fprintf(stdout, "\tmov%c %s, %s\n", mnemonic_suffix(type_width(type)), registers_get_mnemonic(registers, ref),
            get_register_mnemonic(type_width(type), reg));
  } else if (type_width(type) <= type_width(ref.allocation->type)) {
    Type type2 = ref.allocation->type;
    ref.allocation->type = type; // todo make nicer
    fprintf(output, "\tmov%c %s, %s\n", mnemonic_suffix(type_width(type)), registers_get_mnemonic(registers, ref),
            get_register_mnemonic(type_width(type), reg));
    fprintf(stdout, "\tmov%c %s, %s\n", mnemonic_suffix(type_width(type)), registers_get_mnemonic(registers, ref),
            get_register_mnemonic(type_width(type), reg));
    ref.allocation->type = type2;
  } else {
    fprintf(output, "\tmov%c%c %s, %s\n", type_width(ref.allocation->type), mnemonic_suffix(type_width(type)),
            registers_get_mnemonic(registers, ref), get_register_mnemonic(type_width(type), reg));
    fprintf(stdout, "\tmov%c%c %s, %s\n", type_width(ref.allocation->type), mnemonic_suffix(type_width(type)),
            registers_get_mnemonic(registers, ref), get_register_mnemonic(type_width(type), reg));
  }
}

int16_t registers_find(const InstructionTable *table, Registers *registers, FILE *output) {
  for (int i = 0; i < 14; ++i) {
    if (!registers->registers[registerPriority[i]].inUse) {
      return i;
    }
  }

  clear_register(table, registers, registerPriority[7], output);
  return registerPriority[7];
}

void registers_move_into_register_tmp(const InstructionTable *table, Registers *registers, Type type, Reference ref,
                                      int8_t reg, FILE *output) {
  if (!registers->registers[reg].inUse) {
    if (isAllocated(ref.access)) {
      assert(registers_get_storage(registers, ref.allocation)->location != L_None);
    }

    write_mov_into_register(registers, type, ref, reg, output);

    // if (isAllocated(ref.access)) {
    //   registers_claim_register(registers, ref.allocation, reg);
    // }
  } else if (!isAllocated(ref.access) || registers_get_storage(registers, ref.allocation)->location != L_Register ||
             registers_get_storage(registers, ref.allocation)->reg != reg) {
    clear_register(table, registers, reg, output);

    registers_move_into_register_tmp(table, registers, type, ref, reg, output);
  }
}

void write_cmp_op(const Registers *registers, const char *op, const Reference a, const Reference b, FILE *output) {
  Type type = ref_infer_type(a, b);
  Type aType;
  Type bType;
  if (isAllocated(a.access)) {
    aType = a.allocation->type;
    a.allocation->type = type;
  }
  if (isAllocated(b.access)) {
    bType = b.allocation->type;
    b.allocation->type = type;
  }
  if ((isAllocated(a.access) && a.allocation->name != NULL) || (isAllocated(b.access) && b.allocation->name != NULL)) {
    fprintf(output, "\t%s%c %s, %s # %s, %s\n", op, mnemonic_suffix(type_width(type)),
            registers_get_mnemonic(registers, a), registers_get_mnemonic(registers, b),
            isAllocated(a.access) ? a.allocation->name : "<tmp>", isAllocated(b.access) ? b.allocation->name : "<tmp>");
    fprintf(stdout, "\t%s%c %s, %s # %s, %s\n", op, mnemonic_suffix(type_width(type)),
            registers_get_mnemonic(registers, a), registers_get_mnemonic(registers, b),
            isAllocated(a.access) ? a.allocation->name : "<tmp>", isAllocated(b.access) ? b.allocation->name : "<tmp>");
  } else {
    fprintf(output, "\t%s%c %s, %s\n", op, mnemonic_suffix(type_width(type)), registers_get_mnemonic(registers, a),
            registers_get_mnemonic(registers, b));
    fprintf(stdout, "\t%s%c %s, %s\n", op, mnemonic_suffix(type_width(type)), registers_get_mnemonic(registers, a),
            registers_get_mnemonic(registers, b));
  }

  if (isAllocated(a.access)) {
    a.allocation->type = aType;
  }
  if (isAllocated(b.access)) {
    b.allocation->type = bType;
  }
}

void write_ternary_op(const Registers *registers, const char *op, const Width width, const Reference a,
                      const Reference b, const Reference c, FILE *output) {
  fprintf(output, "\t%s%c %s, %s, %s\n", op, mnemonic_suffix(width), registers_get_mnemonic(registers, a),
          registers_get_mnemonic(registers, b), registers_get_mnemonic(registers, c));
  fprintf(stdout, "\t%s%c %s, %s, %s\n", op, mnemonic_suffix(width), registers_get_mnemonic(registers, a),
          registers_get_mnemonic(registers, b), registers_get_mnemonic(registers, c));
}

void write_mov_into_stack(const Registers *registers, const Width width, const Reference ref, const int16_t offset,
                          FILE *output) {
  assert(!isAllocated(ref.access) || registers_get_storage(registers, ref.allocation)->location != L_Stack);
  fprintf(output, "\tmov%c %s, %i(%%rsp)\n", mnemonic_suffix(width), registers_get_mnemonic(registers, ref), offset);
  fprintf(stdout, "\tmov%c %s, %i(%%rsp)\n", mnemonic_suffix(width), registers_get_mnemonic(registers, ref), offset);
}

void write_mov_into_register_FS(const Width width, const int16_t offset, const int8_t reg, FILE *output) {
  fprintf(output, "\tmov%c %i(%%rsp), %s\n", mnemonic_suffix(width), offset, get_register_mnemonic(width, reg));
  fprintf(stdout, "\tmov%c %i(%%rsp), %s\n", mnemonic_suffix(width), offset, get_register_mnemonic(width, reg));
}

void registers_init(Registers *registers, const InstructionTable *table) {
  for (int i = 0; i < 16; ++i) {
    registers->registers[i].inUse = false;
    registers->registers[i].mustRestore = calleeSavedRegistersI[i];
    registers->registers[i].restoreOffset = 0;
  }
  registers->parent = NULL;
  registers->offset = 0;
  registers->storage = malloc(sizeof(Storage) * table->allocations.len);
  for (int i = 0; i < table->allocations.len; ++i) {
    registers->storage[i].location = L_None;
  }
}

void registers_init_child(Registers *registers, InstructionTable *table, const Registers *parent) {
  for (int i = 0; i < 16; ++i) {
    registers->registers[i].inUse = parent->registers[i].inUse;
    registers->registers[i].mustRestore = parent->registers[i].inUse;
    registers->registers[i].restoreOffset = 0;
  }

  registers->parent = parent;

  registers->offset = parent->offset;
  registers->storage = malloc(sizeof(Storage) * table->allocations.len);
  for (int i = 0; i < table->allocations.len; ++i) {
    if (i < table->parentCutoff) {
      registers->storage[i] = parent->storage[i];
    } else {
      registers->storage[i].location = L_None;
    }
  }
}

void registers_free(Registers *registers) {
  free(registers->storage);
  registers->storage = NULL;
}

void registers_claim(Registers *registers, Allocation *allocation) {
  Storage *unknown = registers_get_storage(registers, allocation);

  assert(unknown->location == L_None);
  switch (allocation->source.prop) {
  case None: {
    for (int i = 0; i < 14; ++i) {
      const int8_t reg = registerPriority[i];
      if (!registers->registers[reg].inUse) {
        registers->registers[reg].inUse = true;
        unknown->location = L_Register;
        unknown->reg = reg;
        if (allocation->name != NULL) {
          printf("%i (%s) -> %s\n", allocation->index, allocation->name,
                 registers_get_mnemonic(registers, reference_direct(allocation)));
        } else {
          printf("%i -> %s\n", allocation->index, registers_get_mnemonic(registers, reference_direct(allocation)));
        }
        return;
      }
    }
    assert(unknown->location != L_Stack);
    registers->offset -= (int16_t)type_size(allocation->type);
    unknown->location = L_Stack;
    unknown->offset = registers->offset;
    if (allocation->name != NULL) { // A>>>
      printf("moved alloc %s to stack\n", allocation->name);
    }
    break;
  }
  case FnArgument: {
    if (allocation->source.reg == -1) {
      assert(unknown->location != L_Stack);
      unknown->location = L_Stack;
      unknown->offset = registers->offset;
      if (allocation->name != NULL) { // A>>>
        printf("moved alloc %s to stack\n", allocation->name);
      }
    } else {
      assert(!registers->registers[allocation->source.reg].inUse);
      registers->registers[allocation->source.reg].inUse = true;
      unknown->location = L_Register;
      unknown->reg = allocation->source.reg;
    }
    break;
  }
  case ForceStack: {
    registers->offset -= (int16_t)type_size(allocation->type);
    assert(unknown->location != L_Stack);
    unknown->location = L_Stack;
    unknown->offset = registers->offset;
    if (allocation->name != NULL) { // A>>>
      printf("moved alloc %s to stack\n", allocation->name);
    }
    break;
  }
  case ForceRegister: {
    for (int i = 0; i < 14; ++i) {
      const int8_t reg = registerPriority[i];
      if (!registers->registers[reg].inUse) {
        registers->registers[reg].inUse = true;
        unknown->location = L_Register;
        unknown->reg = reg;
        break;
      }
    }
    break;
  }
  }
  if (allocation->name != NULL) {
    printf("%i (%s) -> %s\n", allocation->index, allocation->name,
           registers_get_mnemonic(registers, reference_direct(allocation)));
  } else {
    printf("%i -> %s\n", allocation->index, registers_get_mnemonic(registers, reference_direct(allocation)));
  }
}

void registers_move_to_stack(Registers *registers, Allocation *allocation, FILE *output) {
  Storage *unknown = registers_get_storage(registers, allocation);
  switch (unknown->location) {
  case L_None: {
    registers->offset -= (int16_t)type_size(allocation->type);
    assert(unknown->location != L_Stack);
    unknown->location = L_Stack;
    unknown->offset = registers->offset;
    if (allocation->name != NULL) { // A>>>
      printf("moved alloc %s to stack\n", allocation->name);
    }
    break;
  }
  case L_Register: {
    const int8_t reg = unknown->reg;
    registers->registers[reg].inUse = false;
    registers->offset -= (int16_t)type_size(allocation->type);
    write_mov_into_stack(registers, type_width(allocation->type), reference_direct(allocation), registers->offset,
                         output);
    assert(unknown->location != L_Stack);
    unknown->location = L_Stack;
    unknown->offset = registers->offset;
    if (allocation->name != NULL) { // A>>>
      printf("moved alloc %s to stack\n", allocation->name);
    }
    break;
  }
  case L_Stack:
    break;
  }
}

void registers_free_register(Registers *registers, Allocation *allocation) {
  Storage *storage = registers_get_storage(registers, allocation);
  switch (storage->location) {
  case L_None:
    break;
  case L_Stack:
    break;
  case L_Register: {
    printf("Deallocated %s (%i-%s)\n", registers_get_mnemonic(registers, reference_direct(allocation)),
           allocation->index, allocation->name == NULL ? "null" : allocation->name);
    registers->registers[storage->reg].inUse = false;
    storage->location = L_None;
    break;
  }
  }
}

Storage *registers_get_storage(const Registers *registers, Allocation *allocation) {
  return &registers->storage[allocation->index];
}

void registers_move_tostack(Registers *registers, Allocation *allocation, FILE *output) {
  Storage *storage = registers_get_storage(registers, allocation);
  if (storage->location == L_Register) {
    registers->offset -= (int16_t)type_size(allocation->type);

    write_mov_into_stack(registers, type_width(allocation->type), reference_direct(allocation), registers->offset,
                         output);
    registers->registers[storage->reg].inUse = false;
    assert(storage->location != L_Stack);
    storage->location = L_Stack;
    storage->offset = registers->offset;
    if (allocation->name != NULL) { // A>>>
      printf("moved alloc %s to stack\n", allocation->name);
    }
  }
}

void registers_claim_register(Registers *registers, Allocation *output, const int8_t reg) {
  assert(!registers->registers[reg].inUse);
  registers->registers[reg].inUse = true;
  Storage *storage = registers_get_storage(registers, output);
  if (storage->location == L_Register) {
    registers->registers[storage->reg].inUse = false;
  }
  storage->location = L_Register;
  storage->reg = reg;
}

void registers_claim_stack(const Registers *registers, Allocation *output, const int16_t offset) {
  Storage *storage = registers_get_storage(registers, output);
  storage->offset = offset;
  assert(storage->location != L_Stack);
  storage->location = L_Stack;
  if (output->name != NULL) { // A>>>
    printf("moved alloc %s to stack\n", output->name);
  }
}

void registers_override(const Registers *registers, Allocation *output, Allocation *from) {
  Storage *storage = registers_get_storage(registers, from);
  *registers_get_storage(registers, output) = *storage;
  storage->location = L_None;
}

void registers_force_register(const Registers *registers, const Reference allocation, const int8_t reg, FILE *output) {
  // fixme
  write_mov_into_register(registers, (Type){.kind = i64, .inner = NULL}, allocation, reg, output);
}

char *registers_get_mnemonic(const Registers *registers, const Reference reference) {
  switch (reference.access) {
  case Direct: {
    // return format_str("%i", reference.allocation->index);
    const Storage *storage = registers_get_storage(registers, reference.allocation);
    switch (storage->location) {
    case L_Stack: {
      char *str = malloc(1 + (abs(storage->offset) / 10 + 1 + (abs(storage->offset) < 0 ? 1 : 0)) + 6 + 1);
      sprintf(str, "%i(%%rsp)", storage->offset);
      str[1 + (abs(storage->offset) / 10 + 1 + (abs(storage->offset) < 0 ? 1 : 0)) + 6] = '\0';
      return str;
    }
    case L_Register: {
      return strdup(get_register_mnemonic(type_width(reference.allocation->type), storage->reg));
    }
    default: {
      assert(false);
      break;
    }
    }
    break;
  }
  case Dereference: {
    // return format_str("(%i)", reference.allocation->index);

    const Storage *storage = registers_get_storage(registers, reference.allocation);
    switch (storage->location) {
    case L_Stack: {
      char *str = malloc(abs(storage->offset) / 10 + 1 + (abs(storage->offset) < 0 ? 1 : 0) + 7 + 1);
      sprintf(str, "%i(%%rsp)", storage->offset); // todo fixme oh no
      str[1 + (abs(storage->offset) / 10 + 1 + (abs(storage->offset) < 0 ? 1 : 0)) + 6] = '\0';
      return str;
    }
    case L_Register: {
      const char *register_mnemonic = get_register_mnemonic(Quad, storage->reg);
      char *str = malloc(1 + sizeof(register_mnemonic) + 1 + 1);
      sprintf(str, "(%s)", register_mnemonic);
      str[1 + sizeof(register_mnemonic) + 1] = '\0';
      return str;
    }
    default: {
      assert(false);
      break;
    }
    }
    break;
  }
  case ConstantI: {
    const size_t len = strlen(reference.value);
    char *str = malloc(1 + len + 1);
    sprintf(str, "$%s", reference.value);
    str[1 + len] = '\0';
    return str;
  }
  case ConstantS: {
    char *str = malloc(7 + (reference.str / 10 + 1) + 1);
    sprintf(str, "$.L.STR%i", reference.str);
    str[7 + (reference.str / 10 + 1)] = '\0';
    return str;
  }
  case Global: {
    const size_t len = strlen(reference.value);
    char *str = malloc(len + 6 + 1);
    sprintf(str, "%s(%%rip)", reference.value);
    str[1 + len] = '\0';
    return str;
  }
  case GlobalRef: {
    const size_t len = strlen(reference.value);
    char *str = malloc(1 + len + 1);
    sprintf(str, "$%s", reference.value);
    str[1 + len] = '\0';
    return str;
  }
  default: {
    assert(false);
    break;
  }
  }
  return NULL;
}

void binary_lea(Registers *registers, Instruction *instruction, FILE *output) {
  if (registers_get_storage(registers, instruction->output.allocation)->location == L_None) {
    registers_claim(registers, instruction->output.allocation);
  }
  if (registers_get_storage(registers, instruction->inputs[0].allocation)->location != L_Stack) {
    registers_move_to_stack(registers, instruction->inputs[0].allocation, output);
  }

  write_binary_op(registers, "lea", type_width(instruction->output.allocation->type), instruction->inputs[0],
                  instruction->output, output);

  if (isAllocated(instruction->inputs[0].access) && instruction->inputs[0].allocation->lastInstr == instruction->id) {
    registers_free_register(registers, instruction->inputs[0].allocation);
  }
}

void cmp_output(const char *op, Registers *registers, Instruction *instruction, FILE *output) {
  if (registers_get_storage(registers, instruction->output.allocation)->location == L_None) {
    registers_claim(registers, instruction->output.allocation);
  }
  write_unary_op(registers, op, Byte, instruction->output, output);
}

void registers_move_into_register(const InstructionTable *table, Registers *registers, Type type, Reference ref,
                                  int8_t reg, FILE *output) {
  if (!registers->registers[reg].inUse) {
    if (isAllocated(ref.access)) {
      assert(registers_get_storage(registers, ref.allocation)->location != L_None);
    }

    write_mov_into_register(registers, type, ref, reg, output);

    if (isAllocated(ref.access)) {
      registers_claim_register(registers, ref.allocation, reg);
    }
  } else if (!isAllocated(ref.access) || registers_get_storage(registers, ref.allocation)->location != L_Register ||
             registers_get_storage(registers, ref.allocation)->reg != reg) {
    clear_register(table, registers, reg, output);

    registers_move_into_register(table, registers, type, ref, reg, output);
  }
}

void registers_move_into_stack(Registers *registers, Allocation *allocation, int16_t offset, FILE *output) {
  Storage *storage = registers_get_storage(registers, allocation);
  if (storage->location == L_Stack && storage->offset == offset)
    return;

  write_mov_into_stack(registers, type_width(allocation->type), reference_direct(allocation), offset, output);
  if (storage->location == L_Register) {
    registers->registers[storage->reg].inUse = false;
  }
  assert(storage->location != L_Stack);
  storage->location = L_Stack;
  storage->offset = offset;
}

int16_t push_function_arguments(const InstructionTable *table, Registers *registers, Instruction *instruction,
                                FILE *output) {
  for (int i = 0; i < instruction->function->arguments.len && i < 6; ++i) {
    assert(instruction->arguments[i].access <= 6);
    registers_move_into_register_tmp(table, registers, instruction->function->arguments.array[i].type,
                                     instruction->arguments[i], argumentRegisters[i], output);
  }

  int16_t offset = registers->offset;

  fputs("#STOR\n", output);
  for (int i = 0; i < table->allocations.len; ++i) {
    if (registers->storage[i].location == L_Register &&
        ((Allocation *)table->allocations.array[i])->lastInstr != instruction->id) {
      bool b = false;
      for (int j = 0; j < 7; ++j) {
        if (registers->storage[i].reg == calleeSavedRegisters[j]) {
          b = true;
          break;
        }
      }
      if (b)
        continue;
      offset -= (int16_t)type_size(((Allocation *)table->allocations.array[i])->type);
      write_mov_into_stack(registers, type_width(((Allocation *)table->allocations.array[i])->type),
                           reference_direct(table->allocations.array[i]), offset, output);
    }
  }
  fputs("#eSTOR\n", output);

  const int16_t base = offset;

  for (int i = instruction->function->arguments.len - 1; i >= 6; --i) {
    offset -= (int16_t)type_size(instruction->function->arguments.array[i].type);
    write_mov_into_stack(registers, type_width(instruction->function->arguments.array[i].type),
                         instruction->arguments[i], offset, output);
  }

  fprintf(output, "\tsubq $%i, %%rsp\n", (-base / 16 + 1) * 16 + 8);
  fprintf(stdout, "\tsubq $%i, %%rsp\n", (-base / 16 + 1) * 16 + 8);

  return base;
}

void write_op_no_args(Registers *registers, char *str, FILE *file) {
  fprintf(stdout, "\t%s\n", str);
  fprintf(file, "\t%s\n", str);
}

void generate_statement(Registers *registers, const char *contents, InstructionTable *table, VarList *globals,
                        FunctionList *functions, StrList *literals, FILE *output) {
  for (int i = 0; i < table->allocations.len; ++i) {
    Allocation *allocation = table->allocations.array[i];
    if (allocation->index >= table->parentCutoff && allocation->source.prop == FnArgument) {
      registers_claim(registers, allocation);
    } else {
      break;
    }
  }
  for (int i = 0; i < table->instructions.len; ++i) {
    printf("\n--- Instruction %i ---\n", i);
    Instruction *instruction = table->instructions.array + i;
    switch (instruction->type) {
    case NEG:
      write_unary_op(registers, "neg", type_width(instruction->output.allocation->type), instruction->output, output);
      break;
    case SETE:
      cmp_output("sete", registers, instruction, output);
      break;
    case SETL:
      cmp_output("setl", registers, instruction, output);
      break;
    case SETG:
      cmp_output("setg", registers, instruction, output);
      break;
    case SETNE:
      cmp_output("setne", registers, instruction, output);
      break;
    case SETLE:
      cmp_output("setle", registers, instruction, output);
      break;
    case SETGE:
      cmp_output("setge", registers, instruction, output);
      break;
    case CALL: {
      clear_register(table, registers, rax, output);
      fputs("\tmovq $0, %rax\n", output);
      fputs("\tmovq $0, %rax\n", stdout);

      const int16_t base = push_function_arguments(table, registers, instruction, output);

      fprintf(output, "\tcall %s\n", instruction->function->name);
      fprintf(stdout, "\tcall %s\n", instruction->function->name);

      fprintf(output, "\taddq $%i, %%rsp\n", (-base / 16 + 1) * 16 + 8);
      fprintf(stdout, "\taddq $%i, %%rsp\n", (-base / 16 + 1) * 16 + 8);

      fputs("#RST\n", output);
      int16_t offset = base;
      for (int j = table->allocations.len - 1; j >= 0; --j) {
        if (registers->storage[j].location == L_Register &&
            ((Allocation *)table->allocations.array[j])->lastInstr != instruction->id) {
          bool b = false;
          for (int k = 0; k < 7; ++k) {
            if (registers->storage[j].reg == calleeSavedRegisters[k]) {
              b = true;
              break;
            }
          }
          if (b)
            continue;

          write_mov_into_register_FS(type_width(((Allocation *)table->allocations.array[j])->type), offset,
                                     registers->storage[j].reg, output);
          offset += (int16_t)type_size(((Allocation *)table->allocations.array[j])->type);
        }
      }
      fputs("#eRST\n", output);
      if (instruction->function->retVal.kind != 0 && instruction->retVal.allocation->lastInstr != -1) {
        registers_claim_register(registers, instruction->retVal.allocation, rax);
      }
      break;
    }
    case MOV: {
      Reference from = instruction->inputs[0];
      Reference to = instruction->output;
      if (from.access == to.access && from.allocation == to.allocation) {
        break;
      }
      if (isAllocated(from.access) && from.access == Direct && from.allocation->lastInstr == instruction->id &&
          to.allocation->index >= table->parentCutoff &&
          type_width(to.allocation->type) == type_width(from.allocation->type)) {
        registers_override(registers, to.allocation, from.allocation);
        printf("Inlined MOV from %i (%s) to %i (%s)\n", from.allocation->index,
               from.allocation->name != NULL ? from.allocation->name : "null", to.allocation->index,
               to.allocation->name != NULL ? to.allocation->name : "null");
        if (to.allocation->name == NULL)
          to.allocation->name = from.allocation->name;
        break;
      }
      if (registers_get_storage(registers, to.allocation)->location == L_None) {
        registers_claim(registers, to.allocation);
      }
      fflush(output);
      fflush(stdout);
      if (!isAllocated(from.access) ||
          (to.access == Dereference ? to.allocation->type.inner->kind : to.allocation->type.kind) ==
              from.allocation->type.kind) {
        write_binary_op(registers, "mov",
                        type_width(to.access == Dereference ? *to.allocation->type.inner : to.allocation->type), from,
                        to, output);
      } else if (type_width(to.access == Dereference ? *to.allocation->type.inner : to.allocation->type) <=
                 type_width(from.allocation->type)) {
        Type type = from.allocation->type;
        from.allocation->type =
            to.access == Dereference ? *to.allocation->type.inner : to.allocation->type; // todo make nicer
        write_binary_op(registers, "mov", type_width(from.allocation->type), from, to, output);
        from.allocation->type = type;
      } else {
        write_binary_transform_op(
            registers, "movs", type_width(from.allocation->type),
            type_width(to.access == Dereference ? *to.allocation->type.inner : to.allocation->type), from, to, output);
      }
      cullref(registers, instruction, from);
      break;
    }
    case LEA: {
      binary_lea(registers, instruction, output);
      break;
    }
    case ADD:
      write_binary_op(registers, "add", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      cullref(registers, instruction, instruction->inputs[0]);
      break;
    case SUB:
      write_binary_op(registers, "sub", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      cullref(registers, instruction, instruction->inputs[0]);
      break;
    case IMUL:
      write_binary_op(registers, "imul", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      cullref(registers, instruction, instruction->inputs[0]);
      break;
    case IDIV:
      clear_register(table, registers, rax, output);
      clear_register(table, registers, rdx, output);

      registers_move_into_register_tmp(table, registers, instruction->inputs[0].allocation->type,
                                       instruction->inputs[0], rax, output);
      write_op_no_args(registers, "cqto", output);

      write_unary_op(registers, "idiv", type_width(instruction->inputs[0].allocation->type), instruction->inputs[1],
                     output);
      registers_claim_register(registers, instruction->output.allocation, rax);
      cullref(registers, instruction, instruction->inputs[0]);
      cullref(registers, instruction, instruction->inputs[1]);
      break;
    case IDIV_mod:
      clear_register(table, registers, rax, output);
      clear_register(table, registers, rdx, output);

      registers_move_into_register_tmp(table, registers, instruction->inputs[0].allocation->type,
                                       instruction->inputs[0], rax, output);
      write_op_no_args(registers, "cqto", output);

      write_unary_op(registers, "idiv", type_width(instruction->inputs[0].allocation->type), instruction->inputs[1],
                     output);
      registers_claim_register(registers, instruction->output.allocation, rdx);
      cullref(registers, instruction, instruction->inputs[0]);
      cullref(registers, instruction, instruction->inputs[1]);
      break;
    case OR:
      write_binary_op(registers, "or", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      cullref(registers, instruction, instruction->inputs[0]);
      break;
    case XOR:
      write_binary_op(registers, "xor", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      cullref(registers, instruction, instruction->inputs[0]);
      break;
    case AND:
      write_binary_op(registers, "and", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      cullref(registers, instruction, instruction->inputs[0]);
      break;
    case NOT:
      write_unary_op(registers, "not", type_width(instruction->output.allocation->type), instruction->inputs[0],
                     output);
      break;
    case SAL:
      write_binary_op(registers, "sal", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      cullref(registers, instruction, instruction->inputs[0]);
      break;
    case SAR:
      write_binary_op(registers, "sar", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      cullref(registers, instruction, instruction->inputs[0]);
      break;
    case CMP:
      write_cmp_op(registers, "cmp", instruction->inputs[0], instruction->inputs[1], output);
      cullref(registers, instruction, instruction->inputs[0]);
      cullref(registers, instruction, instruction->inputs[1]);
      break;
    case TEST:
      write_cmp_op(registers, "test", instruction->inputs[0], instruction->inputs[1], output);
      break;
    case RET:
      write_mov_into_register(registers,
                              isAllocated(instruction->inputs[0].access) ? instruction->inputs[0].allocation->type
                                                                         : (Type){.kind = i64, .inner = NULL},
                              instruction->inputs[0], rax, output);
      fputs("\tret\n", output);
      fputs("\tret\n", stdout);

      puts("Leaked allocations:");
      for (int j = 0; j < table->allocations.len; ++j) {
        if (j < table->parentCutoff)
          continue;
        registers_free_register(registers, table->allocations.array[j]);
      }
      puts("end leaked allocations");
      break;
    case LABEL:
      for (int j = 0; j < i; ++j) {
        Instruction *instr = table->instructions.array + j;
        switch (instr->type) {
        case JMP:
        case JE:
        case JNE:
        case JG:
        case JL:
        case JGE:
        case JLE:
          if (instr->instructions.parent != NULL && !instr->processed) {
            instr->processed = true;
            printf("Processing label .%s.%i:\n", table->name, instr->label);
            Registers subregisters;
            registers_init_child(&subregisters, &instr->instructions, registers);
            generate_statement(&subregisters, contents, &instr->instructions, globals, functions, literals, output);
          }
          break;
        default:
          break;
        }
        fflush(stdout);
        fflush(output);
      }

      for (int j = 0; j < table->allocations.len; ++j) {
        if (((Allocation *)table->allocations.array[j])->lastInstr == instruction->id) {
          registers_free_register(registers, table->allocations.array[j]);
        }
      }

      fprintf(output, ".LBL.%s.%i:\n", table->name, instruction->label);
      fprintf(stdout, ".LBL.%s.%i:\n", table->name, instruction->label);
      break;
    case JMP:
      if (registers->parent != NULL) {
        fputs("#restore frame\n", output);
        for (int j = 0; j < table->allocations.len; ++j) {
          Allocation *allocation = table->allocations.array[j];
          if (allocation->index < table->parentCutoff) {
            const Storage *old = registers_get_storage(registers->parent, allocation);
            const Storage *new = registers_get_storage(registers, allocation);
            switch (old->location) {
            case L_Register: {
              assert(new->location != L_None);
              registers_move_into_register(table, registers, allocation->type, reference_direct(allocation), old->reg,
                                           output);
              break;
            }
            case L_Stack: {
              assert(new->location != L_None);
              registers_move_into_stack(registers, allocation, old->offset, output);
              break;
            }
            case L_None:
              break;
            }
          } else {
            break;
          }
        }
        fputs("#end restore frame\n", output);
      }

      write_jmp(table, "jmp", instruction->label, output);
      break;
    case JE:
      write_jmp(table, "je", instruction->label, output);
      break;
    case JNE:
      write_jmp(table, "jne", instruction->label, output);
      break;
    case JG:
      write_jmp(table, "jg", instruction->label, output);
      break;
    case JL:
      write_jmp(table, "jl", instruction->label, output);
      break;
    case JGE:
      write_jmp(table, "jge", instruction->label, output);
      break;
    case JLE:
      write_jmp(table, "jle", instruction->label, output);
      break;
    }
    fflush(output);
  }

  for (int i = 0; i < table->instructions.len; ++i) {
    const Instruction *instruction = table->instructions.array + i;
    switch (instruction->type) {
    case JMP:
    case JE:
    case JNE:
    case JG:
    case JL:
    case JGE:
    case JLE: {
      assert(instruction->instructions.parent == NULL || instruction->processed);
      break;
    }
    default:
      break;
    }
  }
}
