// Test harness for the bytecode VM exercise.
// Usage:  sh run_tests.sh your_vm.c
// Your main() is ignored. Each test runs in its own process, so a crash
// in one test shows as CRASH and the rest still run.

#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#define main user_main_unused
#include VM_FILE
#undef main

static int checks_failed;

#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      printf("      FAIL line %d: %s\n", __LINE__, msg);                       \
      checks_failed++;                                                         \
    }                                                                          \
  } while (0)

// Run a whole program, return the final status. Output stays in vm.output.
#define RUN(vm, prog) \
  (vm_init(&(vm), (prog), sizeof(prog)), vm_run(&(vm)))

// ---------- tests ----------
static void t_init(void) {
  const uint8_t p[] = {OP_HALT};
  vm_t vm;
  memset(&vm, 0xAB, sizeof vm); // garbage: init must overwrite it
  vm_init(&vm, p, sizeof p);
  CHECK(vm.pc == 0, "pc starts at 0");
  CHECK(vm.sp == 0, "stack starts empty");
  CHECK(vm.out_len == 0, "no output yet");
  CHECK(vm.code == p && vm.code_len == sizeof p, "code and length stored");
}

static void t_halt(void) {
  const uint8_t p[] = {OP_HALT};
  vm_t vm;
  CHECK(RUN(vm, p) == VM_HALTED, "HALT returns VM_HALTED");
}

static void t_push_print(void) {
  const uint8_t p[] = {OP_PUSH, 42, OP_PRINT, OP_HALT};
  vm_t vm;
  CHECK(RUN(vm, p) == VM_HALTED, "halts");
  CHECK(vm.out_len == 1 && vm.output[0] == 42, "prints 42");
  CHECK(vm.sp == 0, "print consumed the value");
}

static void t_push_negative(void) {
  const uint8_t p[] = {OP_PUSH, 0xFB, OP_PRINT, OP_HALT}; // 0xFB = -5 as int8
  vm_t vm;
  RUN(vm, p);
  CHECK(vm.out_len == 1 && vm.output[0] == -5, "operand is sign-extended");
}

static void t_add(void) {
  const uint8_t p[] = {OP_PUSH, 2, OP_PUSH, 3, OP_ADD, OP_PRINT, OP_HALT};
  vm_t vm;
  RUN(vm, p);
  CHECK(vm.out_len == 1 && vm.output[0] == 5, "2 + 3 == 5");
}

static void t_sub_order(void) {
  const uint8_t p[] = {OP_PUSH, 10, OP_PUSH, 3, OP_SUB, OP_PRINT, OP_HALT};
  vm_t vm;
  RUN(vm, p);
  CHECK(vm.out_len == 1 && vm.output[0] == 7, "10 - 3 == 7 (not 3 - 10)");
}

static void t_mul(void) {
  const uint8_t p[] = {OP_PUSH, 6, OP_PUSH, 7, OP_MUL, OP_PRINT, OP_HALT};
  vm_t vm;
  RUN(vm, p);
  CHECK(vm.out_len == 1 && vm.output[0] == 42, "6 * 7 == 42");
}

static void t_dup_pop(void) {
  const uint8_t p[] = {OP_PUSH, 4, OP_DUP, OP_MUL, OP_PRINT,
                       OP_PUSH, 1, OP_PUSH, 2, OP_POP, OP_PRINT, OP_HALT};
  vm_t vm;
  RUN(vm, p);
  CHECK(vm.out_len == 2, "two outputs");
  CHECK(vm.output[0] == 16, "dup then mul: 4 * 4");
  CHECK(vm.output[1] == 1, "pop discarded the 2, leaving 1");
}

static void t_multiple_prints(void) {
  const uint8_t p[] = {OP_PUSH, 1, OP_PRINT, OP_PUSH, 2, OP_PRINT, OP_PUSH, 3,
                       OP_PRINT, OP_HALT};
  vm_t vm;
  RUN(vm, p);
  CHECK(vm.out_len == 3, "three outputs");
  CHECK(vm.output[0] == 1 && vm.output[1] == 2 && vm.output[2] == 3,
        "in order");
}

static void t_jmp(void) {
  // 0: JMP 5   2: PUSH 1  4: PRINT(skipped)... jumps over to PUSH 9
  const uint8_t p[] = {OP_JMP, 5, OP_PUSH, 1, OP_PRINT, OP_PUSH, 9, OP_PRINT,
                       OP_HALT};
  vm_t vm;
  CHECK(RUN(vm, p) == VM_HALTED, "halts");
  CHECK(vm.out_len == 1 && vm.output[0] == 9, "skipped the first print");
}

static void t_jz_taken(void) {
  const uint8_t p[] = {OP_PUSH, 0, OP_JZ, 7, OP_PUSH, 1, OP_PRINT,
                       OP_PUSH, 2, OP_PRINT, OP_HALT};
  vm_t vm;
  RUN(vm, p);
  CHECK(vm.out_len == 1 && vm.output[0] == 2, "zero: jump taken");
  CHECK(vm.sp == 0, "JZ consumed its value");
}

static void t_jz_not_taken(void) {
  const uint8_t p[] = {OP_PUSH, 5, OP_JZ, 9, OP_PUSH, 1, OP_PRINT, OP_HALT,
                       OP_HALT, OP_PUSH, 2, OP_PRINT, OP_HALT};
  vm_t vm;
  RUN(vm, p);
  CHECK(vm.out_len == 1 && vm.output[0] == 1, "non-zero: falls through");
  CHECK(vm.sp == 0, "JZ consumed its value");
}

static void t_countdown(void) {
  // push 3
  // loop(2): dup, print, push 1, sub, dup, jz end(13), jmp loop(2)
  const uint8_t p[] = {OP_PUSH, 3,
                       OP_DUP, OP_PRINT, OP_PUSH, 1, OP_SUB, OP_DUP,
                       OP_JZ, 14, OP_JMP, 2,
                       OP_HALT, OP_HALT, OP_HALT};
  vm_t vm;
  CHECK(RUN(vm, p) == VM_HALTED, "loop terminates");
  CHECK(vm.out_len == 3, "prints three times");
  CHECK(vm.output[0] == 3 && vm.output[1] == 2 && vm.output[2] == 1,
        "prints 3 2 1");
}

static void t_step(void) {
  const uint8_t p[] = {OP_PUSH, 1, OP_PUSH, 2, OP_HALT};
  vm_t vm;
  vm_init(&vm, p, sizeof p);
  CHECK(vm_step(&vm) == VM_OK, "first step ok");
  CHECK(vm.sp == 1 && vm.pc == 2, "one value pushed, pc past operand");
  CHECK(vm_step(&vm) == VM_OK, "second step ok");
  CHECK(vm.sp == 2, "two values");
  CHECK(vm_step(&vm) == VM_HALTED, "third step halts");
}

static void t_underflow(void) {
  const uint8_t p1[] = {OP_ADD, OP_HALT};
  const uint8_t p2[] = {OP_PUSH, 1, OP_ADD, OP_HALT};
  const uint8_t p3[] = {OP_POP, OP_HALT};
  const uint8_t p4[] = {OP_PRINT, OP_HALT};
  const uint8_t p5[] = {OP_DUP, OP_HALT};
  const uint8_t p6[] = {OP_JZ, 0, OP_HALT};
  vm_t vm;
  CHECK(RUN(vm, p1) == VM_ERR_STACK_UNDERFLOW, "ADD on empty");
  CHECK(RUN(vm, p2) == VM_ERR_STACK_UNDERFLOW, "ADD with one value");
  CHECK(RUN(vm, p3) == VM_ERR_STACK_UNDERFLOW, "POP on empty");
  CHECK(RUN(vm, p4) == VM_ERR_STACK_UNDERFLOW, "PRINT on empty");
  CHECK(RUN(vm, p5) == VM_ERR_STACK_UNDERFLOW, "DUP on empty");
  CHECK(RUN(vm, p6) == VM_ERR_STACK_UNDERFLOW, "JZ on empty");
}

static void t_overflow(void) {
  uint8_t p[STACK_SIZE * 2 + 3];
  size_t n = 0;
  for (int i = 0; i < STACK_SIZE + 1; i++) {
    p[n++] = OP_PUSH;
    p[n++] = 1;
  }
  p[n++] = OP_HALT;
  vm_t vm;
  vm_init(&vm, p, n);
  CHECK(vm_run(&vm) == VM_ERR_STACK_OVERFLOW, "17th push overflows");
  CHECK(vm.sp == STACK_SIZE, "stack stayed within bounds");
}

static void t_overflow_dup(void) {
  uint8_t p[STACK_SIZE * 2 + 3];
  size_t n = 0;
  for (int i = 0; i < STACK_SIZE; i++) {
    p[n++] = OP_PUSH;
    p[n++] = 1;
  }
  p[n++] = OP_DUP;
  p[n++] = OP_HALT;
  vm_t vm;
  vm_init(&vm, p, n);
  CHECK(vm_run(&vm) == VM_ERR_STACK_OVERFLOW, "DUP on a full stack overflows");
}

static void t_bad_opcode(void) {
  const uint8_t p[] = {OP_PUSH, 1, 0x77, OP_HALT};
  vm_t vm;
  CHECK(RUN(vm, p) == VM_ERR_BAD_OPCODE, "unknown opcode reported");
  const uint8_t q[] = {0x00};
  CHECK(RUN(vm, q) == VM_ERR_BAD_OPCODE, "0x00 is not an opcode");
}

static void t_run_off_end(void) {
  const uint8_t p[] = {OP_PUSH, 1, OP_POP}; // no HALT
  vm_t vm;
  CHECK(RUN(vm, p) == VM_ERR_PC_OUT_OF_RANGE, "falling off the end is an error");
  vm_init(&vm, p, 0);
  CHECK(vm_run(&vm) == VM_ERR_PC_OUT_OF_RANGE, "empty program is an error");
}

static void t_truncated_operand(void) {
  const uint8_t a[] = {OP_PUSH};
  const uint8_t b[] = {OP_PUSH, 1, OP_JMP};
  const uint8_t c[] = {OP_PUSH, 0, OP_JZ};
  vm_t vm;
  CHECK(RUN(vm, a) == VM_ERR_PC_OUT_OF_RANGE, "PUSH missing its operand");
  CHECK(RUN(vm, b) == VM_ERR_PC_OUT_OF_RANGE, "JMP missing its operand");
  CHECK(RUN(vm, c) == VM_ERR_PC_OUT_OF_RANGE, "JZ missing its operand");
}

static void t_jump_out_of_range(void) {
  const uint8_t a[] = {OP_JMP, 200, OP_HALT};
  const uint8_t b[] = {OP_PUSH, 0, OP_JZ, 200, OP_HALT};
  vm_t vm;
  CHECK(RUN(vm, a) == VM_ERR_PC_OUT_OF_RANGE, "JMP far past the end");
  CHECK(RUN(vm, b) == VM_ERR_PC_OUT_OF_RANGE, "JZ far past the end");
}

static void t_infinite_loop_is_caller_problem(void) {
  // vm_step must still be usable one instruction at a time on a loop.
  const uint8_t p[] = {OP_JMP, 0};
  vm_t vm;
  vm_init(&vm, p, sizeof p);
  int ok = 1;
  for (int i = 0; i < 100; i++)
    if (vm_step(&vm) != VM_OK) ok = 0;
  CHECK(ok, "100 steps of an infinite loop all return VM_OK");
  CHECK(vm.pc == 0, "pc is back at the JMP target");
}

// ---------- runner ----------
typedef struct {
  const char *name;
  void (*fn)(void);
} test_t;

static const test_t tests[] = {
    {"init", t_init},
    {"HALT", t_halt},
    {"PUSH + PRINT", t_push_print},
    {"PUSH negative operand", t_push_negative},
    {"ADD", t_add},
    {"SUB operand order", t_sub_order},
    {"MUL", t_mul},
    {"DUP and POP", t_dup_pop},
    {"several prints in order", t_multiple_prints},
    {"JMP", t_jmp},
    {"JZ taken", t_jz_taken},
    {"JZ not taken", t_jz_not_taken},
    {"countdown loop", t_countdown},
    {"vm_step one at a time", t_step},
    {"stack underflow", t_underflow},
    {"stack overflow (PUSH)", t_overflow},
    {"stack overflow (DUP)", t_overflow_dup},
    {"bad opcode", t_bad_opcode},
    {"running off the end", t_run_off_end},
    {"truncated operand", t_truncated_operand},
    {"jump target out of range", t_jump_out_of_range},
    {"stepping a loop", t_infinite_loop_is_caller_problem},
};

int main(void) {
  int n = (int)(sizeof tests / sizeof tests[0]);
  int passed = 0;
  for (int i = 0; i < n; i++) {
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
      alarm(5); // an infinite loop in your VM shows up as TIMEOUT, not a hang
      checks_failed = 0;
      tests[i].fn();
      fflush(stdout);
      _exit(checks_failed ? 1 : 0);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
      printf("  PASS   %s\n", tests[i].name);
      passed++;
    } else if (WIFEXITED(status) && WEXITSTATUS(status) == 1) {
      printf("  FAIL   %s\n", tests[i].name);
    } else if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM) {
      printf("  TIMEOUT %s (probably an infinite loop)\n", tests[i].name);
    } else if (WIFSIGNALED(status)) {
      printf("  CRASH  %s (signal %d)\n", tests[i].name, WTERMSIG(status));
    } else {
      printf("  ERROR  %s (exit %d: sanitizer report above)\n", tests[i].name,
             WEXITSTATUS(status));
    }
  }
  printf("\n%d/%d tests passed\n", passed, n);
  return passed == n ? 0 : 1;
}
