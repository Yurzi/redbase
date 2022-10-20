//
// File: pf_hashtable.h
// Desc: PF_HashTable class interface
//

#ifndef __PF_HASHTABEL_H__
#define __PF_HASHTABEL_H__

#include <cstdint>

#include "pf_meta.h"
#include "redbase_meta.h"

#define INVALID_SLOT (-1)

class PF_HashTable {
  struct PF_HashEntry {
    PF_HashEntry *prev = nullptr;
    PF_HashEntry *next = nullptr;
    int32_t fd = -1;
    Page page_num = 0;
    int32_t slot = INVALID_SLOT;
  };

public:
  PF_HashTable(const int32_t buckets);
  ~PF_HashTable();

  RC find(const int32_t fd, const Page page_num, int32_t &slot);
  RC insert(const int32_t fd, const Page page_num, const int32_t slot);
  RC remove(const int32_t fd, const Page page_num);

private:
  int32_t hash(const int32_t fd, const Page page_num) const { return ((fd + page_num) % m_buckets); }

private:
  int32_t m_buckets;
  PF_HashEntry **m_hash_table = nullptr;
};


#endif  // !__PF_HASHTABEL_H__
