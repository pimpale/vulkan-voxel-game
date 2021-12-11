#ifndef SRC_BLOCK_H_
#define SRC_BLOCK_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  bool transparent;
  const char *name;
  // later add uv
} BlockDef;

const static BlockDef BLOCKS[] = {
  (BlockDef) {.transparent=true, .name="air"},
  (BlockDef) {.transparent=false, .name="soil"},
  (BlockDef) {.transparent=false, .name="stone"},
};

typedef uint8_t BlockIndex;

#endif // block_h_INCLUDED
