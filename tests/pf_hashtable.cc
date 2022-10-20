#include <cassert>

#include "pf_buffer.h"
#include "pf_error.h"
#include "pf_hashtable.h"
#include "redbase_meta.h"

int main(int argc, char *argv[]) {
  PFHashTable hash_table(40);
  // test insert
  assert(hash_table.insert(1, 1, 114514) == OK_RC);
  assert(hash_table.insert(1, 1, 11451) == PF_HASHPAGEEXIST);
  // test search
  int32_t slot = INVALID_SLOT;
  RC rc = OK_RC;
  rc = hash_table.search(1, 1, slot);
  assert(slot == 114514 && rc == OK_RC);

  rc = hash_table.search(1, 2, slot);
  assert(rc == PF_HASHNOTFOUND && slot == INVALID_SLOT);
  // test remove
  assert(hash_table.remove(1, 1) == OK_RC);
  assert(hash_table.remove(1, 1) == PF_HASHNOTFOUND);
  return 0;
}
