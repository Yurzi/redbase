#ifndef __PF_BUFFER_H__
#define __PF_BUFFER_H__

#include <cstdint>

#include "pf_hashtable.h"
#include "pf_internal.h"
#include "pf_meta.h"
#include "redbase_meta.h"

const int32_t PF_BUFFER_PAGE_SIZE = PF_PAGE_SIZE + sizeof(PF_PageHdr);

// the descriptor of buffer page
struct PF_BufferPageDesc {
  // for page manager
  bool is_dirty = false;
  int32_t pin_count = 0;
  int32_t visit_count = 0;

  // property
  int32_t fd = -1;
  Page page_num = -1;
  int32_t page_size = PF_BUFFER_PAGE_SIZE;

  // data field
  char *p_data = nullptr;

  RC init(const int32_t fd, const Page page_num) {
    this->fd = fd;
    this->page_num = page_num;
    this->is_dirty = false;
    this->pin_count = 1;
    this->visit_count = 1;
    return OK_RC;
  }
};

// forward declartion, refer to pf_buffer.cc for detail
// the worker for buffer pool manager
class PF_BufferWorker;


// the interface for pf buffer pool
class PF_BufferMgr {
public:
  // constructor and deconstructor
  PF_BufferMgr(const int32_t capacity, const int32_t page_size = PF_BUFFER_PAGE_SIZE);
  ~PF_BufferMgr();

public:
  // the interface for data operation
  RC get_page(const int32_t fd, const Page page_num, char *&pp_buffer, bool can_multi_pin = true);
  RC alloc_page(const int32_t fd, const Page page_num, char *&pp_buffer);

  // mark page dirty
  RC mark_dirty(const int32_t fd, const Page page_num);
  // unpin page from the buffer
  RC unpin_page(const int32_t fd, const Page page_num);
  // flush pages for file
  RC flush_pages(const int32_t fd);
  // force a page to disk, but do not remove from the buffer pool
  RC force_pages(const int32_t fd, const Page page_num);

  // remove all entirs from the buffer manager
  RC clear();
  // display all entries in the buffer
  RC print();

  // attempts to resize the buffer to new size
  RC resize(const int32_t new_capacity);

  // memory blocks operation

  // return the size of the block tha can be allocated
  RC get_block_size(int32_t &length) const;

  // allocate a memory chunk that lives in buffer manager
  RC alloc_block(char *&buffer);

  // dispose of a memory chunk managed by the buffer manager
  RC dispose_block(const char *buffer);

private:
  PF_BufferPageDesc **m_pool = nullptr;
  PF_BufferWorker *m_work = nullptr;

  int32_t page_size = PF_BUFFER_PAGE_SIZE;
};

#endif  // !__PF_BUFFER_H__
