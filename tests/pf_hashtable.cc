#include <cassert>

#include "pf_hashtable.h"

int main(int argc, char *argv[]) {
  PFHashTable hash_table(40);
  // test insert
  assert(hash_table.insert(1, 1, 114514));
  assert(!hash_table.insert(1, 1, 11451));
  // test search
  assert(hash_table.search(1, 1) == 114514);
  // test remove
  assert(hash_table.remove(1, 1));
  assert(!hash_table.remove(1, 1));
  return 0;
}
