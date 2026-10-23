#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * A tiny stack-based bytecode VM.
 *
 * Instruction set (1 opcode byte, some have 1 operand byte after it):
 *
 *   OP_PUSH n   push n (operand is a signed 8-bit value)
 *   OP_POP      discard the top of the stack
 *   OP_DUP      push a copy of the top of the stack
 *   OP_ADD      pop b, pop a, push a + b
 *   OP_SUB      pop b, pop a, push a - b      (a was pushed first)
 *   OP_MUL      pop b, pop a, push a * b
 *   OP_JMP addr set pc = addr (operand is an unsigned 8-bit address)
 *   OP_JZ  addr pop v; if v == 0 set pc = addr, otherwise carry on
 *   OP_PRINT    pop v and record it as output
 *   OP_HALT     stop the program
 *
 * Example: push 2, push 3, add, print, halt
 *   { OP_PUSH, 2, OP_PUSH, 3, OP_ADD, OP_PRINT, OP_HALT }
 */

/*
 * The program gets instruction and operand from the code
 * program counter updates to the next instruction
 * the fetched instruction is executed (push -> put value on stack, pop ->
 * remove valie from stack) when values are put into stack the stack count is
 * incremented when OP_PRINT is executed the value is popped from the stack and
 * stored in the output buffer With HALT, you get VM_HALTED, which means
 * success. Without it, pc goes past the last byte and you get
 * VM_ERR_PC_OUT_OF_RANGE. The “Error: no HALT” program in the stepper shows
 * this.
 * */

// STATUS: COMPLETE!

typedef enum {
  OP_PUSH = 0x01,
  OP_POP = 0x02,
  OP_DUP = 0x03,
  OP_ADD = 0x04,
  OP_SUB = 0x05,
  OP_MUL = 0x06,
  OP_JMP = 0x07,
  OP_JZ = 0x08,
  OP_PRINT = 0x09,
  OP_HALT = 0xFF,
} opcode_t;

typedef enum {
  VM_OK,                  // instruction executed, keep going
  VM_HALTED,              // OP_HALT reached (normal end)
  VM_ERR_STACK_OVERFLOW,  // push onto a full stack
  VM_ERR_STACK_UNDERFLOW, // pop/peek from an empty stack
  VM_ERR_BAD_OPCODE,      // byte that is not a known opcode
  VM_ERR_PC_OUT_OF_RANGE, // pc (or a needed operand byte) outside the code
  VM_ERR_OUTPUT_BUFF_OVERFLOW,
} vm_status_t;

void err_msg(vm_status_t err) {
  switch (err) {
  case VM_OK:
    printf("Instruction executed successfully\n");
    break;
  case VM_ERR_BAD_OPCODE:
    printf("Err! Invalid instruction\n");
    break;
  case VM_ERR_PC_OUT_OF_RANGE:
    printf("Err! Program counter out of range\n");
    break;
  case VM_ERR_STACK_OVERFLOW:
    printf("Err! Stack overflow\n");
    break;
  case VM_ERR_STACK_UNDERFLOW:
    printf("Err! Stack underflow\n");
    break;
  case VM_HALTED:
    printf("Code executed successfully\n");
    break;
  case VM_ERR_OUTPUT_BUFF_OVERFLOW:
    printf("Err! output buffer overflow\n");
    break;
  }
}

#define STACK_SIZE 16
#define OUTPUT_SIZE 64

typedef struct {
  const uint8_t *code; // program bytes (not owned by the VM)
  size_t code_len;
  size_t pc; // index of the next byte to fetch
  int32_t stack[STACK_SIZE];
  size_t sp;                   // number of values currently on the stack
  int32_t output[OUTPUT_SIZE]; // values recorded by OP_PRINT
  size_t out_len;
} vm_t;

void vm_init(vm_t *vm, const uint8_t *code, size_t code_len) {
  // TODO: set up every field so the VM is ready to run from the first byte.
  vm->code = code;
  vm->code_len = code_len;
  vm->pc = 0;
  vm->sp = 0;
  vm->out_len = 0;
  for (size_t i = 0; i < STACK_SIZE; i++) {
    vm->stack[i] = 0;
  }
  for (size_t j = 0; j < OUTPUT_SIZE; j++) {
    vm->output[j] = 0;
  }
}

static vm_status_t vm_push(vm_t *vm, int32_t value) {
  // TODO: fail cleanly instead of writing past the end of the stack.
  if (vm->sp >= STACK_SIZE) {
    return VM_ERR_STACK_OVERFLOW;
  }
  vm->stack[vm->sp] = value;
  vm->sp++;
  return VM_OK;
}

static vm_status_t vm_pop(vm_t *vm, int32_t *out) {
  // TODO: fail cleanly instead of reading below the bottom of the stack.
  if (vm->sp == 0) {
    return VM_ERR_STACK_UNDERFLOW;
  }
  *out = vm->stack[vm->sp - 1];
  vm->sp--;
  return VM_OK;
}

vm_status_t vm_add(vm_t *vm) {
  if (vm->sp < 2) {
    return VM_ERR_STACK_UNDERFLOW;
  }
  int32_t a = 0;
  int32_t b = 0;

  vm_pop(vm, &a);
  vm_pop(vm, &b);
  vm_push(vm, (int32_t)(a + b));

  return VM_OK;
}

vm_status_t vm_sub(vm_t *vm) {
  if (vm->sp < 2) {
    return VM_ERR_STACK_UNDERFLOW;
  }
  int32_t a = 0;
  int32_t b = 0;

  vm_pop(vm, &b);
  vm_pop(vm, &a);
  vm_push(vm, a - b);

  return VM_OK;
}

vm_status_t vm_mult(vm_t *vm) {
  if (vm->sp < 2) {
    return VM_ERR_STACK_UNDERFLOW;
  }
  int32_t a = 0;
  int32_t b = 0;

  vm_pop(vm, &a);
  vm_pop(vm, &b);
  vm_push(vm, (int32_t)(a * b));

  return VM_OK;
}

vm_status_t vm_print(vm_t *vm) {
  if (vm->sp <= 0) {
    return VM_ERR_STACK_UNDERFLOW;
  }
  if (vm->out_len >= OUTPUT_SIZE) {
    return VM_ERR_OUTPUT_BUFF_OVERFLOW;
  }
  int32_t v = 0;
  vm_pop(vm, &v);
  vm->output[vm->out_len] = v;
  vm->out_len++;
  return VM_OK;
}

vm_status_t vm_dup(vm_t *vm) {
  if (vm->sp <= 0) {
    return VM_ERR_STACK_UNDERFLOW;
  }
  return vm_push(vm, vm->stack[vm->sp - 1]);
}

vm_status_t vm_jmp(vm_t *vm, uint8_t addr) {
  if (addr >= vm->code_len) {
    return VM_ERR_PC_OUT_OF_RANGE;
  }
  vm->pc = addr;
  return VM_OK;
}

vm_status_t vm_jz(vm_t *vm, uint8_t addr) {
  if (addr >= vm->code_len) {
    return VM_ERR_PC_OUT_OF_RANGE;
  }
  if (vm->sp < 1) {
    return VM_ERR_STACK_UNDERFLOW;
  }
  int32_t v = 0;
  vm_pop(vm, &v);
  if (v == 0) {
    vm->pc = addr;
  } else {
    vm->pc += 2;
  }
  return VM_OK;
}

vm_status_t vm_step(vm_t *vm) {
  // TODO: fetch the opcode at pc and advance pc.
  // TODO: decode with a switch and execute each instruction.
  //       Instructions with an operand must check the operand byte exists.
  vm_status_t ret_code = VM_ERR_BAD_OPCODE;
  if (vm->pc >= vm->code_len) {
    return VM_ERR_PC_OUT_OF_RANGE;
  }
  int32_t out;
  switch (vm->code[vm->pc]) {
  case 0x01:
    if (vm->pc + 1 >= vm->code_len) {
      ret_code = VM_ERR_PC_OUT_OF_RANGE;
      break;
    }
    ret_code = vm_push(vm, (int8_t)vm->code[vm->pc + 1]);
    vm->pc += 2;
    break;
  case 0x02:
    ret_code = vm_pop(vm, &out);
    vm->pc++;
    break;
  case 0x03:
    ret_code = vm_dup(vm);
    vm->pc++;
    break;
  case 0x04:
    ret_code = vm_add(vm);
    vm->pc++;
    break;
  case 0x05:
    ret_code = vm_sub(vm);
    vm->pc++;
    break;
  case 0x06:
    ret_code = vm_mult(vm); // pop b, pop a, push a * b
    vm->pc++;
    break;
  case 0x07:
    if (vm->pc + 1 >= vm->code_len) {
      ret_code = VM_ERR_PC_OUT_OF_RANGE;
      break;
    }
    ret_code = vm_jmp(vm, vm->code[vm->pc + 1]); // set pc = addr
    break;
  case 0x08:
    if (vm->pc + 1 >= vm->code_len) {
      ret_code = VM_ERR_PC_OUT_OF_RANGE;
      break;
    }
    ret_code = vm_jz(vm, vm->code[vm->pc + 1]); // pop v; if v == 0 set pc =
                                                // addr otherwise carry on
    break;
  case 0x09:
    ret_code = vm_print(vm); // pop v and record it as output
    vm->pc++;
    break;
  case 0xff:
    ret_code = VM_HALTED;
    break;
  }
  return ret_code;
}

// Keep stepping until something other than VM_OK comes back, and return it.
vm_status_t vm_run(vm_t *vm) {
  vm_status_t ret_code = VM_OK;
  while (ret_code == VM_OK) {
    ret_code = vm_step(vm);
  }
  err_msg(ret_code);
  return ret_code;
}

void print_stack(vm_t *vm) {
  printf("Stack values: \n");
  for (size_t i = 0; i < vm->sp; i++) {
    printf("addr%zu ========= %d\n", i, vm->stack[i]);
  }
}

int main(void) {
  // push 2, push 3, add, print, halt  ->  prints 5
  const uint8_t program[] = {OP_PUSH, 2, OP_PUSH, 3, OP_ADD, OP_PRINT, OP_HALT};

  vm_t vm;
  vm_init(&vm, program, sizeof(program) / sizeof(program[0]));
  vm_status_t status = vm_run(&vm);
  print_stack(&vm);

  for (size_t i = 0; i < vm.out_len; i++)
    printf("out: %d\n", (int)vm.output[i]);
  printf("status: %d\n", (int)status);

  return 0;
}
