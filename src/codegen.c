#include "codegen.h"

#include "register.h"

#include <string.h>

// no rbp
int registerPriority[15] = { rbx, rcx, rdx, rsi, rdi, rsp, r8, r9, r10, r11, r12, r13, r14, r15, rax };

void registers_init(Registers* registers, InstructionTable* table) {
  for (int i = 0; i < 16; ++i) {
    registers->registers[i] = false;
  }
  registers->offset = 0;
  registers->storage = malloc(sizeof(Storage) * table->allocations.len);
  registers->start = table->allocations.array;
  for (int i = 0; i < table->allocations.len; ++i) {
    registers->storage[i].location = L_None;
  }
}

void registers_free(Registers* registers) {
  free(registers->storage);
}

void registers_claim(Registers* registers, Allocation* allocation) {
  Storage* unknown = registers_get_storage(registers, allocation);
  switch (unknown->location) {
  case L_None: {
    if (!allocation->require_memory) {
      for (int i = 0; i < 15; ++i) {
        const int reg = registerPriority[i];
        if (!registers->registers[reg]) {
          registers->registers[reg] = true;
          unknown->location = L_Register;
          unknown->reg = reg;
          return;
        }
      }
    }
    registers->offset -= size_bytes(allocation->width);
    unknown->location = L_Stack;
    unknown->offset = registers->offset;
    break;
  }
  case L_Stack:
  case L_Register:
    break;
  }
}

Storage* registers_get_storage(const Registers* registers, Allocation* allocation) {
  return &registers->storage[allocation->index];
}

Allocation* registers_allocationfrom_register(const Registers* registers, InstructionTable* table, uint8_t reg) {
  for (int i = 0; i < table->allocations.len; ++i) {
    if (registers->storage[i].reg == reg) {
      return table->allocations.array + i;
    }
  }
  return NULL;
}

void registers_move_tostack(Registers* registers, Allocation* allocation, FILE* output) {
  Storage* storage = registers_get_storage(registers, allocation);
  if (storage->location == L_Register) {
    registers->offset -= size_bytes(allocation->width);

    fprintf(output, "mov%c %s, %i(%%rbp)\n", get_suffix(allocation->width),
            registers_get_mnemonic(registers, reference_direct(allocation)), registers->offset);
    registers->registers[storage->reg] = false;
    storage->location = L_Stack;
    storage->offset = registers->offset;
  }
}

void registers_claim_register(Registers* registers, Allocation* output, int8_t reg) {
  assert(!registers->registers[reg]);
  registers->registers[reg] = true;
  Storage* storage = registers_get_storage(registers, output);
  storage->location = L_Register;
  storage->reg = reg;
}

void registers_claim_stack(const Registers* registers, Allocation* output, int16_t offset) {
  Storage* storage = registers_get_storage(registers, output);
  storage->offset = offset;
  storage->location = L_Stack;
}

void registers_force_register(const Registers* registers, Reference allocation, uint8_t reg, FILE *output) {
  //fixme
  fprintf(output, "mov%c %s, %s\n", 'q', registers_get_mnemonic(registers, allocation), get_register_mnemonic(Quad, reg));
}

char* registers_get_mnemonic(const Registers* registers, Reference reference) {
  switch (reference.access) {
  case Direct: {
    Storage* storage = registers_get_storage(registers, reference.allocation);
    switch (storage->location) {
    case L_Stack: {
      char* str = malloc(1 + (storage->offset / 10 + 1) + 6 + 1);
      sprintf(str, "%i(%%rbp)", storage->offset);
      str[1 + (storage->offset / 10 + 1) + 6] = '\0';
      return str;
    }
    case L_Register: {
      return strdup(get_register_mnemonic(reference.allocation->width, storage->reg));
    }
    default: {
      assert(false);
      break;
    }
    }
    break;
  }
  case Dereference: {
    Storage* storage = registers_get_storage(registers, reference.allocation);
    switch (storage->location) {
    case L_Stack: {
      char* str = malloc(2 + (storage->offset / 10 + 1) + 7 + 1);
      sprintf(str, "(%i(%%rbp))", storage->offset); // todo fixme oh no
      str[1 + (storage->offset / 10 + 1) + 6] = '\0';
      return str;
    }
    case L_Register: {
      const char* register_mnemonic = get_register_mnemonic(reference.allocation->width, storage->reg);
      char* str = malloc(1 + sizeof(register_mnemonic) + 1 + 1);
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
    size_t len = strlen(reference.value);
    char* str = malloc(1 + len + 1);
    sprintf(str, "$%s", reference.value);
    str[1 + len] = '\0';
    return str;
  }
  case ConstantS: {
    char* str = malloc(4 + (reference.str / 10 + 1) + 1);
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

void binary_mov(const char *instr, Registers* registers, Instruction* instruction, FILE* output) {
  if (instruction->inputs[1].allocation == instruction->output) {
    if (registers_get_storage(registers, instruction->output)->location == L_None) {
      registers_claim(registers, instruction->output);
    }
  }
  fprintf(output, "%s %s, %s\n", instr, registers_get_mnemonic(registers, instruction->inputs[0]),
          registers_get_mnemonic(registers, instruction->inputs[1]));
}

void binary_self(const char *instr, Registers* registers, Instruction* instruction, int index, FILE* output) {
  if ((instruction->inputs[1].access == Direct || instruction->inputs[1].access == Dereference) && instruction->inputs[1].allocation->lastInstr == index) {
    //fixme
  }
  fprintf(output, "%s %s, %s\n", instr, registers_get_mnemonic(registers, instruction->inputs[0]),
          registers_get_mnemonic(registers, instruction->inputs[1]));
}

void binary_op(const char *instr, Registers* registers, Instruction* instruction, FILE* output) {
  if (instruction->inputs[1].allocation == instruction->output) {
    if (registers_get_storage(registers, instruction->output)->location == L_None) {
      registers_claim(registers, instruction->output);
    }
  }
  fprintf(output, "%s %s, %s\n", instr, registers_get_mnemonic(registers, instruction->inputs[0]),
          registers_get_mnemonic(registers, instruction->inputs[1]));
}

void unary_op(const char *instr, Registers* registers, Instruction* instruction, FILE* output) {
  fprintf(output, "%s %s\n", instr, registers_get_mnemonic(registers, instruction->inputs[0]));
}

void generate_statement(Registers* registers, const char* contents, InstructionTable* table, VarList* globals, FunctionList* functions,
                   StrList* literals, FILE* output) {
  for (int i = 0; i < table->instructions.len; ++i) {
    Instruction* instruction = table->instructions.array + i;
    switch (instruction->output->source.location) {

    case None:
      registers_claim(registers, instruction->output);
      break;
    case Stack: {
      registers_claim_stack(registers, instruction->output, instruction->output->source.offset);
      break;
    }
    case Register: {
      if (registers->registers[instruction->output->source.reg]) {
        Allocation* from_register = registers_allocationfrom_register(registers, table, instruction->output->source.reg);
        assert(from_register != NULL);
        if (from_register->lastInstr > i) {
          registers_move_tostack(registers, from_register, output);
        }
      }
      registers_claim_register(registers, instruction->output, instruction->output->source.reg);
      break;
    }
    case Copy: {
      if (instruction->output->lastInstr > i) {
        registers_claim_register(registers, instruction->output, instruction->output->source.reg);
        fprintf(output, "mov%c %s, %s\n", 'q', registers_get_mnemonic(registers, instruction->output->source.reference), registers_get_mnemonic(registers, reference_direct(instruction->output)));
      } else {
        *registers_get_storage(registers, instruction->output) = *registers_get_storage(registers, instruction->output->source.reference.allocation);
        registers_get_storage(registers, instruction->output->source.reference.allocation)->location = L_None;
        for (int j = 0; j < 3; ++j) {
          if (instruction->inputs[j].allocation == instruction->output->source.reference.allocation) {
            instruction->inputs[j].allocation = instruction->output;
            puts("mov copy to input");
          }
        }
      }
      break;
    }
    }
    for (int j = 0; j < 3; ++j) {
      const Reference reference = instruction->inputs[j];

      switch (reference.access) {
      case Direct:
      case Dereference: {
        if (registers_get_storage(registers, reference.allocation)->location != None) {
          break;
        }
        if (reference.allocation->source.start == i) {
          if (reference.allocation->source.location != None) {
            switch (reference.allocation->source.location) {
            case Stack:
              fprintf(output, "mov%c %i(%%rbp), %s\n", get_suffix(reference.allocation->width), reference.allocation->source.offset, registers_get_mnemonic(registers, reference));
              break;
            case Register:
              if (registers->registers[reference.allocation->source.reg]) {
                Allocation* from_register = registers_allocationfrom_register(registers, table, reference.allocation->source.reg);
                assert(from_register != NULL);
                if (from_register->lastInstr > i) {
                  registers_move_tostack(registers, from_register, output);
                }
              }
              fprintf(output, "mov%c %s, %s\n", get_suffix(reference.allocation->width), get_register_mnemonic(reference.allocation->width, reference.allocation->source.reg), registers_get_mnemonic(registers, reference));
              break;
            case Copy:
              fprintf(output, "mov%c %s, %s\n", get_suffix(reference.allocation->width), registers_get_mnemonic(registers, reference.allocation->source.reference), registers_get_mnemonic(registers, reference));
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
    case PUSH:
      unary_op("push", registers, instruction, output);
      break;
    case POP:
      unary_op("pop", registers, instruction, output);
      break;
    case CALL:
      unary_op("call", registers, instruction, output);
      break;

    case MOV:
      binary_op("mov", registers, instruction, output);
      break;
    case LEA:
      binary_op("lea", registers, instruction, output);
      break;
    case ADD:
      binary_op("add", registers, instruction, output);
      break;
    case SUB:
      binary_op("sub", registers, instruction, output);
      break;
    case IMUL:
      binary_op("imul", registers, instruction, output);
      break;
    case OR:
      binary_op("or", registers, instruction, output);
      break;
    case XOR:
      binary_op("xor", registers, instruction, output);
      break;
    case AND:
      binary_op("and", registers, instruction, output);
      break;
    case NOT:
      binary_op("not", registers, instruction, output);
      break;
    case SAL:
      binary_op("sal", registers, instruction, output);
      break;
    case SAR:
      binary_op("sar", registers, instruction, output);
      break;
    case CMP:
      binary_op("cmp", registers, instruction, output);
      break;
    case TEST:
      binary_op("test", registers, instruction, output);
      break;
    default:
      assert(false);
      break;
    }
    fflush(output);
  }
}