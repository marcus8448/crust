#include "codegen.h"

#include "register.h"
#include "util.h"

#include <string.h>

void write_jmp(const InstructionTable *table, const char *op, int label, FILE *output) {
  fprintf(output, "\t%s .LBL.%s.%i\n", op, table->name, label);
  fprintf(stdout, "\t%s .LBL.%s.%i\n", op, table->name, label);
}

void write_unary_op(const Registers *registers, const char *op, const Width width, const Reference ref, FILE *output) {
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
  fprintf(output, "\t%s%c %s, %s\n", op, mnemonic_suffix(width), registers_get_mnemonic(registers, a),
          registers_get_mnemonic(registers, b));
  fprintf(stdout, "\t%s%c %s, %s\n", op, mnemonic_suffix(width), registers_get_mnemonic(registers, a),
          registers_get_mnemonic(registers, b));
}

void write_cmp_op(const Registers *registers, const char *op, const Reference a, const Reference b, FILE *output) {
  fprintf(output, "\t%s%c %s, %s\n", op, mnemonic_suffix(type_width(ref_infer_type(a, b))),
          registers_get_mnemonic(registers, a), registers_get_mnemonic(registers, b));
  fprintf(stdout, "\t%s%c %s, %s\n", op, mnemonic_suffix(type_width(ref_infer_type(a, b))),
          registers_get_mnemonic(registers, a), registers_get_mnemonic(registers, b));
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
  fprintf(output, "\tmov%c %s, %i(%%rbp)\n", mnemonic_suffix(width), registers_get_mnemonic(registers, ref), offset);
  fprintf(stdout, "\tmov%c %s, %i(%%rbp)\n", mnemonic_suffix(width), registers_get_mnemonic(registers, ref), offset);
}

void write_mov_into_register(const Registers *registers, const Width width, const Reference ref, const int8_t reg,
                             FILE *output) {
  fprintf(output, "\tmov%c %s, %s\n", mnemonic_suffix(width), registers_get_mnemonic(registers, ref),
          get_register_mnemonic(width, reg));
  fprintf(stdout, "\tmov%c %s, %s\n", mnemonic_suffix(width), registers_get_mnemonic(registers, ref),
          get_register_mnemonic(width, reg));
}

void registers_init(Registers *registers, InstructionTable *table) {
  for (int i = 0; i < 16; ++i) {
    registers->registers[i].inUse = false;
    registers->registers[i].mustRestore = calleeSavedRegistersI[i];
    registers->registers[i].restoreOffset = 0;
  }
  registers->parentCutoff = 0;
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

  registers->parentCutoff = table->parentCutoff;

  registers->offset = parent->offset;
  registers->storage = malloc(sizeof(Storage) * table->allocations.len);
  for (int i = 0; i < table->allocations.len; ++i) {
    if (i < registers->parentCutoff) {
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
        printf("%i -> %s\n", allocation->index, registers_get_mnemonic(registers, reference_direct(allocation)));
        return;
      }
    }
    registers->offset -= type_size(allocation->type);
    unknown->location = L_Stack;
    unknown->offset = registers->offset;
    break;
  }
  case FnArgument: {
    if (allocation->source.reg == -1) {
      unknown->location = L_Stack;
      unknown->offset = registers->offset;
    } else {
      assert(!registers->registers[allocation->source.reg].inUse);
      registers->registers[allocation->source.reg].inUse = true;
      unknown->location = L_Register;
      unknown->reg = allocation->source.reg;
    }
    break;
  }
  case ForceStack: {
    registers->offset -= type_size(allocation->type);
    unknown->location = L_Stack;
    unknown->offset = registers->offset;
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
  printf("%i -> %s\n", allocation->index, registers_get_mnemonic(registers, reference_direct(allocation)));
}

void registers_make_stack(Registers *registers, Allocation *allocation, FILE *output) {
  Storage *unknown = registers_get_storage(registers, allocation);
  switch (unknown->location) {
  case L_None: {
    registers->offset -= type_size(allocation->type);
    unknown->location = L_Stack;
    unknown->offset = registers->offset;
    break;
  }
  case L_Register: {
    if (allocation->index < registers->parentCutoff) {
      assert(false); // fixme
    }

    const int8_t reg = unknown->reg;
    registers->registers[reg].inUse = false;
    registers->offset -= type_size(allocation->type);
    write_mov_into_stack(registers, type_width(allocation->type), reference_direct(allocation), registers->offset,
                         output);
    unknown->location = L_Stack;
    unknown->offset = registers->offset;
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
  case L_Register:
    registers->registers[storage->reg].inUse = false;
    storage->location = L_None;
    break;
  }
}

Storage *registers_get_storage(const Registers *registers, Allocation *allocation) {
  return &registers->storage[allocation->index];
}

void registers_move_tostack(Registers *registers, Allocation *allocation, FILE *output) {
  Storage *storage = registers_get_storage(registers, allocation);
  if (storage->location == L_Register) {
    registers->offset -= type_size(allocation->type);

    write_mov_into_stack(registers, type_width(allocation->type), reference_direct(allocation), registers->offset,
                         output);
    registers->registers[storage->reg].inUse = false;
    storage->location = L_Stack;
    storage->offset = registers->offset;
  }
}

void registers_claim_register(Registers *registers, Allocation *output, const int8_t reg) {
  assert(!registers->registers[reg].inUse);
  registers->registers[reg].inUse = true;
  Storage *storage = registers_get_storage(registers, output);
  storage->location = L_Register;
  storage->reg = reg;
}

void registers_claim_stack(const Registers *registers, Allocation *output, const int16_t offset) {
  Storage *storage = registers_get_storage(registers, output);
  storage->offset = offset;
  storage->location = L_Stack;
}

void registers_override(const Registers *registers, Allocation *output, Allocation *from) {
  Storage *storage = registers_get_storage(registers, from);
  *registers_get_storage(registers, output) = *storage;
  storage->location = L_None;
}

void registers_force_register(const Registers *registers, const Reference allocation, const int8_t reg, FILE *output) {
  // fixme
  write_mov_into_register(registers, Quad, allocation, reg, output);
}

char *registers_get_mnemonic(const Registers *registers, const Reference reference) {
  switch (reference.access) {
  case Direct: {
    // return format_str("%i", reference.allocation->index);
    const Storage *storage = registers_get_storage(registers, reference.allocation);
    switch (storage->location) {
    case L_Stack: {
      char *str = malloc(1 + (abs(storage->offset) / 10 + 1 + (abs(storage->offset) < 0 ? 1 : 0)) + 6 + 1);
      sprintf(str, "%i(%%rbp)", storage->offset);
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
      sprintf(str, "%i(%%rbp)", storage->offset); // todo fixme oh no
      str[1 + (abs(storage->offset) / 10 + 1 + (abs(storage->offset) < 0 ? 1 : 0)) + 6] = '\0';
      return str;
    }
    case L_Register: {
      const char *register_mnemonic = get_register_mnemonic(type_width(reference.allocation->type), storage->reg);
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
    char *str = malloc(4 + (reference.str / 10 + 1) + 1);
    sprintf(str, "$.LC%i", reference.str);
    str[4 + (reference.str / 10 + 1)] = '\0';
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
    registers_make_stack(registers, instruction->inputs[0].allocation, output);
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

void generate_statement(Registers *registers, const char *contents, InstructionTable *table, VarList *globals,
                        FunctionList *functions, StrList *literals, FILE *output) {
  for (int i = 0; i < table->allocations.len; ++i) {
    Allocation *allocation = table->allocations.array[i];
    if (allocation->index >= registers->parentCutoff && allocation->source.prop == FnArgument) {
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
      assert(false);
      break;
    }
    case MOV: {
      Reference from = instruction->inputs[0];
      Reference to = instruction->output;
      if (from.access == to.access && from.allocation == to.allocation) {
        break;
      }
      if (isAllocated(from.access) && from.allocation->lastInstr == instruction->id) {
        registers_override(registers, to.allocation, from.allocation);
        printf("Inlined MOV from %i to %i\n", from.allocation->index, to.allocation->index);
        break;
      }
      if (registers_get_storage(registers, to.allocation)->location == L_None) {
        registers_claim(registers, to.allocation);
      }
      fflush(output);
      fflush(stdout);
      write_binary_op(registers, "mov", type_width(instruction->output.allocation->type), from, to, output);
      break;
    }
    case LEA: {
      binary_lea(registers, instruction, output);
      break;
    }
    case ADD:
      write_binary_op(registers, "add", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      break;
    case SUB:
      write_binary_op(registers, "sub", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      break;
    case IMUL:
      write_binary_op(registers, "imul", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      break;
    case OR:
      write_binary_op(registers, "or", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      break;
    case XOR:
      write_binary_op(registers, "xor", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      break;
    case AND:
      write_binary_op(registers, "and", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      break;
    case NOT:
      write_unary_op(registers, "not", type_width(instruction->output.allocation->type), instruction->inputs[0],
                     output);
      break;
    case SAL:
      write_binary_op(registers, "sal", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      break;
    case SAR:
      write_binary_op(registers, "sar", type_width(instruction->output.allocation->type), instruction->inputs[0],
                      instruction->output, output);
      break;
    case CMP:
      write_cmp_op(registers, "cmp", instruction->inputs[0], instruction->inputs[1], output);
      break;
    case TEST:
      write_cmp_op(registers, "test", instruction->inputs[0], instruction->inputs[1], output);
      break;
    case RET:
      write_mov_into_register(registers, Quad, instruction->inputs[0], rax, output);
      fputs("\tret\n", output);
      break;
    case LABEL:
      fprintf(output, ".LBL.%s.%i:\n", table->name, instruction->label);
      fprintf(stdout, ".LBL.%s.%i:\n", table->name, instruction->label);
      break;
    case JMP:
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
    Instruction *instruction = table->instructions.array + i;
    switch (instruction->type) {
    case JMP:
    case JE:
    case JNE:
    case JG:
    case JL:
    case JGE:
    case JLE:
      if (instruction->instructions.parent != NULL) {
        printf("Processing label .%s.%i:\n", table->name, instruction->label);
        Registers subregisters;
        registers_init_child(&subregisters, &instruction->instructions, registers);
        generate_statement(&subregisters, contents, &instruction->instructions, globals, functions, literals, output);
      }
      break;
    default:
      break;
    }
    fflush(stdout);
    fflush(output);
  }
}
