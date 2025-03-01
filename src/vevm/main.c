#include "../../src/common/assets/assets.h"
#include "../../src/common/cmd/argparse/argparse.h"
#include "../../src/common/cmd/cmd.h"
#include "../../src/common/io/io.h"
#include "../../src/common/math/bigint/bigint.h"
#include "../../src/common/utils/hex_utils/hex_utils.h"
#include "../../src/core/config.h"
#include "../../src/core/vm/vm.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

// To get the compiler to not complain about formating for our custom integer
// types
#pragma GCC diagnostic ignored "-Wformat"

// argparse
static const char *const usages[] = {
    vevm_logo,
    NULL,
};

void print_exit(const char *msg) {
    printf(YELLOW "%s\n" RESET, msg);
    exit(1);
}

// write to program buffer
// @param program[]: program buffer to write to
// @param bytecode: bytecode char to read from
// @param bytecode_size: size of bytecode char
void write2_prog_buff(uint256_t program[], char *bytecode, long bytecode_size) {
  const static char bytecode_max_err[34] = "bytecode size exceeds 3072 bytes\0";
  // 3073: max bytecode length in bytes + '\0'
  bytecode_size > 3073 ? print_exit(bytecode_max_err) : 0;

  int elements = (bytecode_size / 16) + 1;

  int i = 0;
  for (; i < elements; ++i) {
    change_uint256(&program[i / 4], i % 4, hex_char2uint(bytecode, 16 * i, 16));
  }
}

// load bytecode from file
// @param program[]: program buffer to write to
// @param file: size of file to read from
void load_bytecode_file(uint256_t program[], char *file) {
  FILE *fd;

  long file_size;

  file = read_file_fmmap(fd, file, &file_size);

  file == NULL ? exit(1) : write2_prog_buff(program, file, file_size);

  safe_munmap(file, file_size);
}

// main program
int main(int argc, const char *argv[]) {
  static uint256_t program[MAX_BYTECODE_LEN];

  // to hold input and file chars from argparse
  char *file = NULL;
  char *input = NULL;

  // param variables
  int debug = 0;
  int perms = 0;

  struct argparse_option options[] = {
      OPT_GROUP("Commands"),
      OPT_STRING('i', "input", &input, "input bytecode"),
      OPT_STRING('f', "file", &file, "file with bytecode", NULL, 0, 0),
      OPT_BOOLEAN('d', "debug", &debug, "print EVM debug", NULL, 0, 0),
      OPT_HELP(),
      OPT_END(),
  };

  // parse arguments
  struct argparse argparse;
  argparse_init(&argparse, options, usages, 0);
  argc = argparse_parse(&argparse, argc, argv);

  // ┌───────────────────┐
  // │   INPUT BYTECODE  │
  // └───────────────────┘
  if (input != NULL) {

    write2_prog_buff(program, input, strlen(input));
    run_vm(program);
  }

  // ┌───────────────────────────┐
  // │   READ FILE WITH BYTCODE  │
  // └───────────────────────────┘
  if (file != NULL) {
    load_bytecode_file(program, file);
    run_vm(program);
  }

  // ┌───────────────────────────────────┐
  // │   IF NO COMMAND EXISTS            │
  // └───────────────────────────────────┘
  if (argv != 0 && file == NULL && input == NULL) {
    argparse_usage(&argparse);
  }

  return 0;
}