#include "pf_hashtable.h"
#include "pf_buffer.h"
#include "pf_error.h"
#include "redbase_meta.h"

PFHashTable::PFHashTable(uint32_t capacity)
  : capacity_(capacity)
  , table_(capacity) {}

/*
 * search - 在表中搜寻这样的三元组，找到了，返回，否则返回 -1
 */
RC PFHashTable::search(int32_t fd, Page num, int32_t& slot) {
  slot = INVALID_SLOT;
  int32_t key = calc_hash(fd, num);
  if (key < 0)
    return PF_HASHNOTFOUND;

  std::list<Triple> &lst = table_[key];
  // 迭代
  std::list<Triple>::const_iterator it;
  for (it = lst.begin(); it != lst.end(); ++it) {
    if ((it->fd == fd) && (it->num == num)) {
      slot = it->slot;
      return OK_RC;
    }
  }
  return PF_HASHNOTFOUND;
}

/*
 * insert - 往hash表中插入一个元素
 */
RC PFHashTable::insert(int32_t fd, Page num, int32_t slot) {
  int32_t key = calc_hash(fd, num);
  if (key < 0)
    return false;

  std::list<Triple> &lst = table_[key];
  std::list<Triple>::const_iterator it;
  // table 中不能存在这样的entry
  for (it = lst.begin(); it != lst.end(); ++it) {
    if ((it->fd == fd) && (it->num == num)) {
      return PF_HASHPAGEEXIST;
    }
  }
  lst.push_back(Triple(fd, num, slot));
  return OK_RC;
}

/*
 * remove - 从hash表中移除一个元素
 */
RC PFHashTable::remove(int32_t fd, Page num) {
  int key = calc_hash(fd, num);
  if (key < 0)
    return PF_HASHNOTFOUND;

  std::list<Triple> &lst = table_[key];
  std::list<Triple>::const_iterator it;
  // table 中不能存在这样的entry
  for (it = lst.begin(); it != lst.end(); ++it) {
    if ((it->fd == fd) && (it->num == num)) {
      lst.erase(it);
      return OK_RC;
    }
  }
  return PF_HASHNOTFOUND;
}
