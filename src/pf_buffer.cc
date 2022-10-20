#include "pf_buffer.h"

#include <deque>
#include <stack>

#include "pf_error.h"
#include "pf_hashtable.h"
#include "pf_internal.h"
#include "redbase_meta.h"
#include "utils.h"

// PF_BufferScheduler(Abstrct)
class PF_BufferWorker {
public:
  PF_BufferWorker(PF_BufferMgr *parent, const int32_t capacity, PF_BufferPageDesc *pool, PF_HashTable *table)
    : m_parent(parent)
    , m_capacity(capacity)
    , m_pool(pool)
    , m_table(table) {}
  virtual ~PF_BufferWorker() {}

  // interface
  virtual RC get_page(const int32_t fd, const Page page_num, char *&pp_buffer, bool can_multi_pin = true) = 0;
  virtual RC alloc_page(const int32_t fd, const Page page_num, char *&pp_buffer) = 0;

  // flush pages for file
  virtual RC flush_pages(const int32_t fd) = 0;
  // force a page to disk, but do not remove from the buffer pool
  virtual RC force_pages(const int32_t fd, const Page page_num) = 0;

  // remove all entires from the buffer
  virtual RC clear() = 0;

  // attempts to resize the buffer to new size
  virtual RC resize(const int32_t new_capacity) = 0;

  // memory blocks operation

  // allocate a memory chunk that lives in buffer manager
  virtual RC alloc_block(char *&buffer) = 0;

  // dispose of a memory chunk managed by the buffer manager
  virtual RC dispose_block(const char *buffer) = 0;

protected:
  virtual RC internal_alloc(int32_t &slot) = 0;
  // read page
  RC read_page(const int32_t fd, const Page page_num, char *dest) {
    // seek file pointer to appropriate place
    int64_t offset = page_num * m_parent->page_size + PF_FILE_HDR_SIZE;
    if (redbase::lseek(fd, offset, L_SET) < 0) {
      return PF_UNIX;
    }

    // read the data
    ssize_t num_bytes = redbase::read(fd, dest, m_parent->page_size);
    if (num_bytes < 0) {
      return PF_UNIX;
    } else if (num_bytes != m_parent->page_size) {
      return PF_INCOMPLETEREAD;
    } else {
      return OK_RC;
    }
  }
  // write a page
  RC write_page(const int32_t fd, const Page page_num, char *source) {
    // seek to appropriate place
    int64_t offset = page_num * m_parent->page_size + PF_FILE_HDR_SIZE;
    if (redbase::lseek(fd, offset, L_SET) < 0) {
      return PF_UNIX;
    }

    // write the data
    ssize_t num_bytes = redbase::write(fd, source, m_parent->page_size);
    if (num_bytes < 0) {
      return PF_UNIX;
    } else if (num_bytes != m_parent->page_size) {
      return PF_INCOMPLETEWRITE;
    } else {
      return OK_RC;
    }
  }

protected:
  PF_BufferMgr *m_parent = nullptr;
  PF_BufferPageDesc *m_pool = nullptr;
  PF_HashTable *m_table = nullptr;

  int32_t m_capacity = 0;
};

// PF_BufferSchedulerLRU
class PF_BufferWorkerLRU : public PF_BufferWorker {
public:
  PF_BufferWorkerLRU(PF_BufferMgr *parent, const int32_t capacity, PF_BufferPageDesc *pool, PF_HashTable *table)
    : PF_BufferWorker(parent, capacity, pool, table) {
    for (int32_t i = 0; i < m_capacity; ++i) {
      int32_t slot = INVALID_SLOT;
      RC rc = m_table->find(m_pool[i].fd, m_pool[i].page_num, slot);
      if (rc || slot == INVALID_SLOT) {
        m_free.push(i);
      } else {
        m_lru.push_front(slot);
      }
    }
  }
  ~PF_BufferWorkerLRU() {}

  // interface
  virtual RC get_page(const int32_t fd, const Page page_num, char *&pp_buffer, bool can_multi_pin = true) override {
    RC rc = OK_RC;                // rt code
    int32_t slot = INVALID_SLOT;  // buffer slot where page located

    // search for page in buffer
    if ((rc = m_table->find(fd, page_num, slot)) && (rc != PF_HASHNOTFOUND)) {
      return rc;  // unexpected error
    }

    // if page not in buffer
    if (rc == PF_HASHNOTFOUND) {
    }
    return rc;
  }
  virtual RC alloc_page(const int32_t fd, const Page page_num, char *&pp_buffer) override {}

  // flush pages for file
  virtual RC flush_pages(const int32_t fd) override {}
  // force a page to disk, but do not remove from the buffer pool
  virtual RC force_pages(const int32_t fd, const Page page_num) override {}

  // remove all entires from the buffer
  virtual RC clear() override {}

  // attempts to resize the buffer to new size
  virtual RC resize(const int32_t new_capacity) override {}

  // memory blocks operation

  // allocate a memory chunk that lives in buffer manager
  virtual RC alloc_block(char *&buffer) override {}

  // dispose of a memory chunk managed by the buffer manager
  virtual RC dispose_block(const char *buffer) override {}

private:
  virtual RC internal_alloc(int32_t &slot) override {
    RC rc = OK_RC;
    slot = INVALID_SLOT;

    if (!m_free.empty()) {
      // if free stack is not empty, choose a slot from the free stack;
      slot = m_free.top();
      m_free.pop();
    } else {
      // choose the leaset-recently used page that is unpinned
      for (std::deque<int32_t>::const_reverse_iterator it = m_lru.rbegin(); it != m_lru.rend(); ++it) {
        if (m_pool[*it].pin_count == 0) {
          slot = *it;
          break;
        }
      }

      // return error if all buffer were pinned
      if (slot == INVALID_SLOT) {
        return PF_NOBUF;
      }

      // wirte back if the page is dirty
      if (m_pool[slot].is_dirty) {
        if ((rc = write_page(m_pool[slot].fd, m_pool[slot].page_num, m_pool[slot].p_data))) {
          return rc;
        }
        m_pool[slot].is_dirty = false;
      }

      // remove page from the hash table and slot from the used queue
      if ((rc = m_table->remove(m_pool[slot].fd, m_pool[slot].page_num))) {
        return rc;
      }
      std::erase(m_lru, slot);
    }
    // add the slot to the queue head
    m_lru.push_front(slot);
    return OK_RC;
  }

private:
  std::deque<int32_t> m_lru;
  std::stack<int32_t> m_free;
};


// PF_BufferMgr

/*
 * PF_BufferMgr constructer
 *
 * Desc: public, the construter for PF_BufferMgr
 * In: capacity - the capacity of buffer
       page_size - the page size of buffer page
       sched_method - the method to schedule the memory
 */
PF_BufferMgr::PF_BufferMgr(const int32_t capacity, const PF_BufferSchedulerMethod &sched_method, const int32_t page_size) {}

/*
 * PF_BufferMgr deconstructer
 *
 * Desc: public, the deconstructer for PF_BufferMgr
 */
PF_BufferMgr::~PF_BufferMgr() {}
