#include "codegen.h"

#include "register.h"

#include <string.h>

void registers_init(Registers *registers, InstructionTable *table) {
  for (int i = 0; i < 16; ++i) {
    registers->registers[i] = false;
  }
  registers->offset = 0;
  registers->storage = malloc(sizeof(Storage) * table->allocations.len);
  for (int i = 0; i < table->allocations.len; ++i) {
    registers->storage[i].location = L_None;
  }
}

void registers_free(Registers *registers) {
  free(registers->storage);
}

void registers_claim(Registers *registers, Allocation *allocation) {
  Storage *unknown = registers_get_storage(registers, allocation);
  switch (unknown->location) {
  case L_None: {
    for (int i = 0; i < 14; ++i) {
      const uint8_t reg = registerPriority[i];
      if (!registers->registers[reg]) {
        registers->registers[reg] = true;
        unknown->location = L_Register;
        unknown->reg = reg;
        return;
      }
    }
    registers->offset -= type_size(allocation->type);
    unknown->location = L_Stack;
    unknown->offset = registers->offset;
    break;
  }
  case L_Stack:
  case L_Register:
    break;
  }
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
    uint8_t reg = unknown->reg;
    registers->offset -= type_size(allocation->type);
    unknown->location = L_Stack;
    unknown->offset = registers->offset;
    fprintf(output, "mov%c %s, %i(%%rbp)\n", mnemonic_suffix(type_width(allocation->type)),
            get_register_mnemonic(type_width(allocation->type), reg), registers->offset);
    break;
  }
  case L_Stack:
    break;
  }
}

Storage *registers_get_storage(const Registers *registers, Allocation *allocation) {
  return &registers->storage[allocation->index];
}

Allocation *registers_allocationfrom_register(const Registers *registers, InstructionTable *table, uint8_t reg) {
  for (int i = 0; i < table->allocations.len; ++i) {
    if (registers->storage[i].reg == reg) {
      return table->allocations.array[i];
    }
  }
  return NULL;
}

void registers_move_tostack(Registers *registers, Allocation *allocation, FILE *output) {
  Storage *storage = registers_get_storage(registers, allocation);
  if (storage->location == L_Register) {
    registers->offset -= type_size(allocation->type);

    fprintf(output, "mov%c %s, %i(%%rbp)\n", mnemonic_suffix(type_width(allocation->type)),
            registers_get_mnemonic(registers, reference_direct(allocation)), registers->offset);
    registers->registers[storage->reg] = false;
    storage->location = L_Stack;
    storage->offset = registers->offset;
  }
}

void registers_claim_register(Registers *registers, Allocation *output, uint8_t reg) {
  assert(!registers->registers[reg]);
  registers->registers[reg] = true;
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

void registers_force_register(const Registers *registers, Reference allocation, uint8_t reg, FILE *output) {
  // fixme
  fprintf(output, "mov%c %s, %s\n", mnemonic_suffix(Quad), registers_get_mnemonic(registers, allocation),
          get_register_mnemonic(Quad, reg));
}

char *registers_get_mnemonic(const Registers *registers, Reference reference) {
  switch (reference.access) {
  case Direct: {
    const Storage *storage = registers_get_storage(registers, reference.allocation);
    switch (storage->location) {
    case L_Stack: {
      char *str = malloc(1 + (storage->offset / 10 + 1) + 6 + 1);
      sprintf(str, "%i(%%rbp)", storage->offset);
      str[1 + (storage->offset / 10 + 1) + 6] = '\0';
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
    const Storage *storage = registers_get_storage(registers, reference.allocation);
    switch (storage->location) {
    case L_Stack: {
      char *str = malloc(2 + (storage->offset / 10 + 1) + 7 + 1);
      sprintf(str, "(%i(%%rbp))", storage->offset); // todo fixme oh no
      str[1 + (storage->offset / 10 + 1) + 6] = '\0';
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
  default: {
    assert(false);
    break;
  }
  }
  return NULL;
}

void binary_mov(const char *instr, Registers *registers, Instruction *instruction, int index, FILE *output) {
  if (isAllocated(instruction->inputs[0].access) && instruction->inputs[0].allocation->lastInstr == index) {
    registers_override(registers, instruction->output.allocation, instruction->inputs[0].allocation);
    return;
  }
  fprintf(output, "%s%c %s, %s\n", instr, mnemonic_suffix(type_width(instruction->output.allocation->type)),
          registers_get_mnemonic(registers, instruction->inputs[0]),
          registers_get_mnemonic(registers, instruction->output));
}

void ret_op(const Registers *registers, const Instruction *instruction, FILE *output) {
  registers_force_register(registers, instruction->output, rax, output);
  fputs("ret\n", output);
}

void ternary_op(const char *instr, Registers *registers, Instruction *instruction, int index, FILE *output) {
  int src = -1;
  if (registers_get_storage(registers, instruction->output.allocation)->location == L_None &&
      instruction->output.allocation->source.location == None) {
    if (isAllocated(instruction->inputs[0].access) && instruction->inputs[0].allocation->lastInstr == index) {
      instruction->output = instruction->inputs[0];
      src = 1;
    } else if (isAllocated(instruction->inputs[1].access) && instruction->inputs[1].allocation->lastInstr == index) {
      instruction->output = instruction->inputs[1];
      src = 0;
    }
  }
  if (src == -1) {
    registers_claim(registers, instruction->output.allocation);
    fprintf(output, "mov%c %s, %s\n", mnemonic_suffix(type_width(instruction->output.allocation->type)),
            registers_get_mnemonic(registers, instruction->inputs[0]),
            registers_get_mnemonic(registers, instruction->output));
    src = 1;
  }
  fprintf(output, "%s%c %s, %s\n", instr, mnemonic_suffix(type_width(instruction->output.allocation->type)),
          registers_get_mnemonic(registers, instruction->inputs[src]),
          registers_get_mnemonic(registers, instruction->output));
}

void binary_lea(const char *instr, Registers *registers, Instruction *instruction, int index, FILE *output) {
  if (registers_get_storage(registers, instruction->output.allocation)->location == L_None) {
    registers_claim(registers, instruction->output.allocation);
  }
  if (registers_get_storage(registers, instruction->inputs[0].allocation)->location != L_Stack) {
    registers_make_stack(registers, instruction->inputs[0].allocation, output);
  }
  fprintf(output, "%s%c %s, %s\n", instr, mnemonic_suffix(type_width(instruction->output.allocation->type)),
          registers_get_mnemonic(registers, instruction->inputs[0]),
          registers_get_mnemonic(registers, instruction->output));
}

void binary_op(const char *instr, Registers *registers, Instruction *instruction, FILE *output) {
  // if (instruction->inputs[1].allocation == instruction->output) {
  //   if (registers_get_storage(registers, instruction->output)->location == L_None) {
  //     // if (instruction->inputs[1].allocation->lastInstr == index) {
  //     //   registersreg()
  //     // }
  //     // if (instruction->inputs[1].allocation->lastInstr == index) {
  //     //   registers_force_register()
  //     // }
  //     registers_claim(registers, instruction->output);
  //   }
  // }
  fprintf(output, "%s %s, %s\n", instr, registers_get_mnemonic(registers, instruction->inputs[0]),
          registers_get_mnemonic(registers, instruction->inputs[1]));
}

void unary_op(const char *instr, Registers *registers, Instruction *instruction, FILE *output) {
  if (instruction->inputs[0].allocation != instruction->output.allocation) {
    fprintf(output, "mov%c %s, %s\n", mnemonic_suffix(type_width(instruction->output.allocation->type)),
            registers_get_mnemonic(registers, instruction->inputs[0]),
            registers_get_mnemonic(registers, instruction->output));
  }

  fprintf(output, "%s %s\n", instr, registers_get_mnemonic(registers, instruction->output));
}

void generate_statement(Registers *registers, const char *contents, InstructionTable *table, VarList *globals,
                        FunctionList *functions, StrList *literals, FILE *output) {
  for (int i = 0; i < table->instructions.len; ++i) {
    Instruction *instruction = table->instructions.array + i;
    switch (instruction->output.allocation->source.location) {

    case None:
      registers_claim(registers, instruction->output.allocation);
      break;
    case Stack: {
      registers_claim_stack(registers, instruction->output.allocation, instruction->output.allocation->source.offset);
      break;
    }
    case Register: {
      if (registers->registers[instruction->output.allocation->source.reg]) {
        Allocation *from_register =
            registers_allocationfrom_register(registers, table, instruction->output.allocation->source.reg);
        assert(from_register != NULL);
        if (from_register->lastInstr > i) {
          registers_move_tostack(registers, from_register, output);
        }
      }
      registers_claim_register(registers, instruction->output.allocation, instruction->output.allocation->source.reg);
      break;
    }
    case Copy: {
      if (instruction->output.allocation->lastInstr > i) {
        registers_claim_register(registers, instruction->output.allocation, instruction->output.allocation->source.reg);
        fprintf(output, "mov%c %s, %s\n", mnemonic_suffix(type_width(instruction->output.allocation->type)),
                registers_get_mnemonic(registers, instruction->output.allocation->source.reference),
                registers_get_mnemonic(registers, instruction->output));
      } else {
        *registers_get_storage(registers, instruction->output.allocation) =
            *registers_get_storage(registers, instruction->output.allocation->source.reference.allocation);
        registers_get_storage(registers, instruction->output.allocation->source.reference.allocation)->location =
            L_None;
        for (int j = 0; j < 2; ++j) {
          if (instruction->inputs[j].allocation == instruction->output.allocation->source.reference.allocation) {
            instruction->inputs[j] = instruction->output;
            puts("mov copy to input");
          }
        }
      }
      break;
    }
    }
    for (int j = 0; j < 2; ++j) {
      const Reference reference = instruction->inputs[j];

      switch (reference.access) {
      case Direct:
      case Dereference: {
        if (registers_get_storage(registers, reference.allocation)->location != L_None) {
          break;
        }
        if (reference.allocation->source.start == i) {
          if (reference.allocation->source.location != None) {
            switch (reference.allocation->source.location) {
            case Stack:
              fprintf(output, "mov%c %i(%%rbp), %s\n", mnemonic_suffix(type_width(reference.allocation->type)),
                      reference.allocation->source.offset, registers_get_mnemonic(registers, reference));
              break;
            case Register:
              if (registers->registers[reference.allocation->source.reg]) {
                Allocation *from_register =
                    registers_allocationfrom_register(registers, table, reference.allocation->source.reg);
                assert(from_register != NULL);
                if (from_register->lastInstr > i) {
                  registers_move_tostack(registers, from_register, output);
                }
              }
              fprintf(output, "mov%c %s, %s\n", mnemonic_suffix(type_width(reference.allocation->type)),
                      get_register_mnemonic(type_width(reference.allocation->type), reference.allocation->source.reg),
                      registers_get_mnemonic(registers, reference));
              break;
            case Copy:
              fprintf(output, "mov%c %s, %s\n", mnemonic_suffix(type_width(reference.allocation->type)),
                      registers_get_mnemonic(registers, reference.allocation->source.reference),
                      registers_get_mnemonic(registers, reference));
              break;
            default:
              assert(false);
            }
          } else {
            registers_claim(registers, reference.allocation);
          }
        }
        break;
      }
      case ConstantI:
      case ConstantS:
        break;
      case UNINIT:
        break;
      }
    }

    switch (instruction->type) {
    case NEG:
      unary_op("neg", registers, instruction, output);
      break;
    case SETE:
      unary_op("sete", registers, instruction, output);
      break;
    case SETL:
      unary_op("setl", registers, instruction, output);
      break;
    case SETG:
      unary_op("setg", registers, instruction, output);
      break;
    case SETNE:
      unary_op("setne", registers, instruction, output);
      break;
    case SETLE:
      unary_op("setle", registers, instruction, output);
      break;
    case SETGE:
      unary_op("setge", registers, instruction, output);
      break;
    case CALL:
      unary_op("call", registers, instruction, output);
      break;

    case MOV:
      binary_mov("mov", registers, instruction, i, output);
      break;
    case LEA:
      binary_lea("lea", registers, instruction, i, output);
      break;
    case ADD:
      ternary_op("add", registers, instruction, i, output);
      break;
    case SUB:
      ternary_op("sub", registers, instruction, i, output);
      break;
    case IMUL:
      ternary_op("imul", registers, instruction, i, output);
      break;
    case OR:
      ternary_op("or", registers, instruction, i, output);
      break;
    case XOR:
      ternary_op("xor", registers, instruction, i, output);
      break;
    case AND:
      ternary_op("and", registers, instruction, i, output);
      break;
    case NOT:
      ternary_op("not", registers, instruction, i, output);
      break;
    case SAL:
      ternary_op("sal", registers, instruction, i, output);
      break;
    case SAR:
      ternary_op("sar", registers, instruction, i, output);
      break;
    case CMP:
      ternary_op("cmp", registers, instruction, i, output);
      break;
    case TEST:
      ternary_op("test", registers, instruction, i, output);
      break;
    case RET:
      ret_op(registers, instruction, output);
      break;
    default:
      assert(false);
      break;
    }
    fflush(output);
  }
}
