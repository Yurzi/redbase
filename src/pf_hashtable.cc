#include "pf_hashtable.h"

PFHashTable::PFHashTable(uint32_t capacity)
  : capacity_(capacity)
  , table_(capacity) {}

/*
 * search - 在表中搜寻这样的三元组，找到了，返回，否则返回 -1
 */
int32_t PFHashTable::search(int32_t fd, Page num) {
  int32_t key = calc_hash(fd, num);
  if (key < 0)
    return -1;

  std::list<Triple> &lst = table_[key];
  // 迭代
  std::list<Triple>::const_iterator it;
  for (it = lst.begin(); it != lst.end(); ++it) {
    if ((it->fd == fd) && (it->num == num))
      return it->slot;
  }
  return -1;
}

/*
 * insert - 往hash表中插入一个元素
 */
bool PFHashTable::insert(int32_t fd, Page num, int32_t slot) {
  int32_t key = calc_hash(fd, num);
  if (key < 0)
    return false;

  std::list<Triple> &lst = table_[key];
  std::list<Triple>::const_iterator it;
  // table 中不能存在这样的entry
  for (it = lst.begin(); it != lst.end(); ++it) {
    if ((it->fd == fd) && (it->num == num)) {
      return false;
    }
  }
  lst.push_back(Triple(fd, num, slot));
  return true;
}

/*
 * remove - 从hash表中移除一个元素
 */
bool PFHashTable::remove(int32_t fd, Page num) {
  int key = calc_hash(fd, num);
  if (key < 0)
    return false;

  std::list<Triple> &lst = table_[key];
  std::list<Triple>::const_iterator it;
  // table 中不能存在这样的entry
  for (it = lst.begin(); it != lst.end(); ++it) {
    if ((it->fd == fd) && (it->num == num)) {
      lst.erase(it);
      return true;
    }
  }
  return false;
}
