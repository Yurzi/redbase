#include "pf_hashtable.h"
#include "pf_error.h"
#include "pf_meta.h"
#include "redbase_meta.h"

#include <cstdint>

PF_HashTable::PF_HashTable(int32_t buckets) : m_buckets(buckets) {
  // Allocate memory for hash tabe
  m_hash_table = new PF_HashEntry *[m_buckets];

  // Initialize all buckets to empty
  for (int32_t i = 0; i < m_buckets; ++i) {
    m_hash_table[i] = nullptr;
  }
}

PF_HashTable::~PF_HashTable() {
  // Clear all buckets
  for (int32_t i = 0; i < m_buckets; ++i) {
    // Delete all entries in this bucket
    PF_HashEntry *entry = m_hash_table[i];
    while (entry != nullptr) {
      PF_HashEntry *next = entry->next;
      delete entry;
      entry = next;
    }
  }

  // Finally delete the table
  delete[] m_hash_table;
}

/*
 * find
 *
 * Desc: Find a hash tabe entry.
 * In: fd - file descriptor
 *     page_num - page number
 *     slot - set to slot associated with fd and page_num
 * Ret: PF return code
 */
RC PF_HashTable::find(int32_t fd, Page page_num, int32_t &slot) {
  // get which bucket it should be in
  int32_t bucket = this->hash(fd, page_num);
  slot = INVALID_SLOT;

  if (bucket < 0) {
    return PF_HASHNOTFOUND;
  }

  // go through the linked list of this bucket
  for (PF_HashEntry *entry = m_hash_table[bucket]; entry != nullptr; entry = entry->next) {
    if (entry->fd == fd && entry->page_num == page_num) {
      // find it
      slot = entry->slot;
      return OK_RC;
    }
  }

  return PF_HASHNOTFOUND;
}

/*
 * insert
 *
 * Desc: insert a hash table entry
 * In: fd - file descriptor
 *     page_num - page number
 *     slot - slot associated with fd and page_num
 * Ret: PF return code
 */
RC PF_HashTable::insert(int32_t fd, Page page_num, int32_t slot) {
  // get which bucket it should be in
  int32_t bucket = this->hash(fd, page_num);

  // check entry doesn't already exist in the bucket
  PF_HashEntry *entry = nullptr;
  for (entry = m_hash_table[bucket]; entry != nullptr; entry = entry->next) {
    if (entry->fd == fd && entry->page_num == page_num) {
      return PF_HASHPAGEEXIST;
    }
  }
  // allocate memory for new hash entry
  if ((entry = new PF_HashEntry) == nullptr) {
    return PF_NOMEM;
  }

  // insert entry at the head of list for this bucket
  entry->fd = fd;
  entry->page_num = page_num;
  entry->slot = slot;
  entry->next = m_hash_table[bucket];
  entry->prev = nullptr;
  if (m_hash_table[bucket] != nullptr) {
    m_hash_table[bucket]->prev = entry;
  }
  m_hash_table[bucket] = entry;
  return OK_RC;
}

/*
 * remove
 *
 * Desc: remove a hash tabe entry
 * In: fd - file descriptor
 *     page_num - page number
 * Ret: PF return code
 */
RC PF_HashTable::remove(int32_t fd, Page page_num) {
  // get which bucket it should be in
  int32_t bucket = this->hash(fd, page_num);

  // find the entry is in this bucket
  PF_HashEntry *entry = nullptr;
  for (entry = m_hash_table[bucket]; entry != nullptr; entry = entry->next) {
    if (entry->fd == fd && entry->page_num == page_num) {
      break;
    }
  }

  // check the search result
  if (entry == nullptr) {
    return PF_HASHNOTFOUND;
  }

  // remove this entry
  if (entry == m_hash_table[bucket]) {
    // if the entry is the head of this list
    m_hash_table[bucket] = entry->next;
  }
  if (entry->prev != nullptr) {
    // if the entry has prev entry
    entry->prev->next = entry->next;
  }
  if (entry->next != nullptr) {
    // if the entry has next entry
    entry->next->prev = entry->prev;
  }

  // preform the delete
  delete entry;

  // return OK
  return OK_RC;
}
