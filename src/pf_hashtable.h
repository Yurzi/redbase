#ifndef __PF_HASHTABLE_H__
#define __PF_HASHTABLE_H__

#include <cstdint>
#include <list>
#include <vector>

#include "redbase_meta.h"
#include "pf_meta.h"

/*
 *  PFHashTable 记录了fd指代的文件的第num个页面在哪个slot里
 *  使用拉链法的HashTable
 */
class PFHashTable {
  struct Triple {
    int32_t fd;
    Page num;
    int32_t slot;
    Triple(int32_t fd, Page num, int32_t slot)
      : fd(fd)
      , num(num)
      , slot(slot) {}
  };

public:
  PFHashTable(uint32_t capacity);
  ~PFHashTable() {}

public:
  RC search(int32_t fd, Page num, int32_t& slot);
  RC insert(int32_t fd, Page num, int32_t slot);
  RC remove(int32_t fd, Page num);

private:
  int32_t calc_hash(int32_t fd, Page num) { return (fd + num) % capacity_; }

private:
  uint32_t capacity_;
  std::vector<std::list<Triple>> table_;
};

#endif /* __PF_HASHTABLE_H__ */
