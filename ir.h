#include "types.h"

enum InstrType {
  IF,
  CALL,
  DEFINE,
  RET,
  ASSIGN
}

typedef struct Instr {
  InstrType type;
  union {
    const char* name;
    LispExpr* expr;
  } data;
}
