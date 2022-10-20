#include "pf_buffer.h"
#include "pf_error.h"
#include "pf_internal.h"
#include "pf_meta.h"
#include "redbase_meta.h"
#include "utils.h"
#include <cstdint>

PFBuffer::PFBuffer()
  : table_(capacity / 2) {
  // 初始化slot链
  free_ = 0;
  first_ = INVALID_SLOT;
  last_ = INVALID_SLOT;
  for (int32_t i = 0; i < capacity; ++i) {
    slots_[i].prev = i - 1;
    slots_[i].next = (i == capacity - 1) ? -1 : (i + 1);
  }
}
/*
 *  get_page - 获取一个指向缓存空间的指针,  同时将其pin在buffer上
 */
RC PFBuffer::get_page(int32_t fd, Page num, Ptr &addr) {
  RC rc = OK_RC;
  int32_t slot = INVALID_SLOT;

  rc = table_.search(fd, num, slot);
  if (slot != INVALID_SLOT && rc != PF_HASHNOTFOUND) {
    // 页面已经在缓存中 增加pin计数
    slots_[slot].count++;
    addr = slots_[slot].buffer;
  } else {
    if ((rc = fetch_avaliable_slot(slot))) {
      return rc;
    }

    // read_page insert it into hashtable and initialize the slot entry
    if ((rc = read_page(fd, num, slots_[slot].buffer)) ||
        (rc = table_.insert(fd, num, slot)) ||
        (rc = init_slot(fd, num, slot))) {
      return rc;
    }
  }
  // Make this page the most recently used page
  if ((rc = this->unlink(slot)) || (rc = this->link_head(slot)))
    return rc;
  return OK_RC;
}

/*
 * alloc_page - 分配一个页面
 */
RC PFBuffer::alloc_page(int32_t fd, Page num, Ptr &addr) {
  RC rc = OK_RC;
  int32_t slot = INVALID_SLOT;

  rc = table_.search(fd, num, slot);
  if (slot != INVALID_SLOT && rc != PF_HASHNOTFOUND)
    return PF_PAGEINBUF;

  // 分配空页
  rc = fetch_avaliable_slot(slot);
  if (slot == INVALID_SLOT)
    return PF_NOBUF;

  if ((rc = table_.insert(fd, num, slot)) || (rc = init_slot(fd, num, slot))) {
    this->unlink(slot);
    this->link_head(slot);
  }

  addr = slots_[slot].buffer;
  return OK_RC;
}

/*
 * mark_dirty - 将页面标记为dirty
 */
RC PFBuffer::mark_dirty(int32_t fd, Page num) {
  RC rc = OK_RC;
  int32_t slot = INVALID_SLOT;

  rc = table_.search(fd, num, slot);
  if (slot == INVALID_SLOT)
    return PF_PAGENOTINBUF;

  if (slots_[slot].count == 0)
    return PF_PAGEUNPINNED;
  // 标记为脏页
  slots_[slot].is_dirty = true;

  // make this page the most recently used page
  if ((rc = this->unlink(slot)) || (rc = this->link_head(slot)))
    return rc;

  return OK_RC;
}

RC PFBuffer::unpin(int32_t fd, Page num) {
  RC rc = OK_RC;
  int32_t slot = INVALID_SLOT;

  rc = table_.search(fd, num, slot);
  // the page must be found and pinned in the buffer
  if (slot != INVALID_SLOT)
    return PF_PAGENOTINBUF;

  if (slots_[slot].count == 0) {
    return PF_PAGEUNPINNED;
  }

  // if unpinning the last pin, make it the most recently used page
  slots_[slot].count -= 1;
  if (slots_[slot].count == 0) {
    if ((rc = this->unlink(slot)) || (rc = this->link_head(slot)))
      return rc;
  }

  return OK_RC;
}


/*
 * pin
 *
 * Desc: public. manually pin a page in buffer to avoid it be cleand
 * In: fd - file descriptor
 *     num - page number
 * Ret: PF return code
 */
RC PFBuffer::pin(int32_t fd, Page num) {
  RC rc = OK_RC;
  int32_t slot = INVALID_SLOT;

  rc = table_.search(fd, num, slot);
  if (rc != PF_HASHNOTFOUND && slot != INVALID_SLOT) {
    // page in buffer
    slots_[slot].count++;
  } else {
    // add the page to buffer like get_page
    if ((rc = fetch_avaliable_slot(slot))) {
      return rc;
    }

    // read_page insert it into hashtable and initialize the slot entry
    if ((rc = read_page(fd, num, slots_[slot].buffer)) ||
        (rc = table_.insert(fd, num, slot)) ||
        (rc = init_slot(fd, num, slot))) {
      return rc;
    }
  }
  // make this page the most recently used
  if ((rc = this->unlink(slot)) || (rc = this->link_head(slot)))
    return rc;
  return OK_RC;
}

/*
 * force_page
 *
 * Desc: public. forcely write a page to file in any condition
 * In: fd - file descriptor
 *     num - page number if ALL_PAGES is be used all pages will be wrte back
 * Ret: PF return code
 */
RC PFBuffer::force_pages(int32_t fd, Page num) {
  RC rc = OK_RC;
  std::vector<int32_t> temp;
  for (int32_t slot = last_; slot != INVALID_SLOT; slot = slots_[slot].prev) {
    if (slots_[slot].fd == fd &&
        (num == ALL_PAGES || slots_[slot].num == num)) {
      if (slots_[slot].is_dirty) {
        if ((rc = write_back(slots_[slot].fd, slots_[slot].num,
                             slots_[slot].buffer)))
          return rc;
        slots_[slot].is_dirty = false;
        if (slots_[slot].count == 0) {
          // lazy for unused page recycle.
          temp.push_back(slot);
        }
      }
    }
  }

  // perform the recycle
  for (const int32_t idx : temp) {
    if ((rc = table_.remove(slots_[idx].fd, slots_[idx].num)) ||
        (rc = this->unlink(idx)) || (rc = this->insert_free(idx))) {
      return rc;
    }
  }

  return OK_RC;
}

/*
 * clear_file_pages
 *
 * Desc: public. flush all dirty page into disk for a file and
 *               recycle the unpinned page to free list
 * In: fd - file descriptor
 * Ret: PF return code
 */
RC PFBuffer::clear_file_pages(int32_t fd) {
  RC rc = OK_RC;
  std::vector<int32_t> temp;

  // iterator for the used list
  for (int32_t slot = last_; slot != INVALID_SLOT; slot = slots_[slot].prev) {
    if (slots_[slot].fd == fd && slots_[slot].is_dirty) {
      // write back the dirty pages
      if ((rc = write_back(slots_[slot].fd, slots_[slot].num,
                           slots_[slot].buffer)))
        return rc;
      slots_[slot].is_dirty = false;
    }

    if (slots_[slot].count == 0) {
      // recycle the unused page to free list. due to the list iterator, lazy.
      temp.push_back(slot);
    }
  }
  // perform the recycle
  for (const int32_t idx : temp) {
    if ((rc = table_.remove(slots_[idx].fd, slots_[idx].num)) ||
        (rc = this->unlink(idx)) || (rc = this->insert_free(idx))) {
      return rc;
    }
  }

  return OK_RC;
}

/*
 * flush_buffer
 *
 * Desc: public. flush all dirty pages to file and recycle all unpinned pages
 *       to free list;
 * In: Null
 * Ret: PF return code;
 */
RC PFBuffer::flush_buffer() {
  RC rc = OK_RC;
  std::vector<int32_t> temp;

  // iterator for the used list
  for (int32_t slot = last_; slot != INVALID_SLOT; slot = slots_[slot].prev) {
    if (slots_[slot].is_dirty && slots_[slot].fd != MEMORY_FD) {
      // write back the dirty pages except the MEMORY_FD
      if ((rc = write_back(slots_[slot].fd, slots_[slot].num,
                           slots_[slot].buffer)))
        return rc;
      slots_[slot].is_dirty = false;
    }

    if (slots_[slot].count == 0) {
      // recycle the unused page to free list. due to the list iterator, lazy.
      temp.push_back(slot);
    }
  }

  // perform the recycle
  for (const int32_t idx : temp) {
    if ((rc = table_.remove(slots_[idx].fd, slots_[idx].num)) ||
        (rc = this->unlink(idx)) || (rc = this->insert_free(idx))) {
      return rc;
    }
  }

  return OK_RC;
}

/*
 * get_block_size
 *
 * Desc: just return size of the page, since a block will take up a page in
 *       buffer pool
 * In: length - the length of block will return
 * Ret: PF return code
 */
RC PFBuffer::get_block_size(int32_t &length) const {
  length = pagesize;
  return OK_RC;
}

/*
 * alloc_block
 *
 * Desc: allocate a page in the buffer pool that is not associated with a
 *       particular file and returns the pointer to the data area back to
 *       the user.
 * In: buffer - a pointer to the allocated page
 * Ret: PF return code
 */
RC PFBuffer::alloc_block(Ptr &buffer) {
  RC rc = OK_RC;
  int32_t slot = INVALID_SLOT;
  if ((rc = fetch_avaliable_slot(slot)) != OK_RC)
    return rc;

  // create artificial page number (just needs to be unique for hash table)
  int32_t temp = (int64_t)(slots_[slot].buffer);
  Page num = Page(temp);

  // insert the page into the hash table, add initialize the page descriptor
  if ((rc = table_.insert(MEMORY_FD, num, slot) != OK_RC) ||
      (rc = init_slot(MEMORY_FD, num, slot) != OK_RC)) {
    this->unlink(slot);
    this->link_head(slot);
    return rc;
  }
  return OK_RC;
}

/*
 * dispose_block
 *
 * Desc: free the block of memory from the buffer pool
 * In: buffer - a pointer to the page
 * Ret: PF return code
 */
RC PFBuffer::dispse_block(Ptr buffer) {
  int32_t temp = (int64_t)buffer;
  return unpin(MEMORY_FD, temp);
}

/*
 * fetch_avaliable_slot - 获取一个可用的slot 使用LRU
 */
RC PFBuffer::fetch_avaliable_slot(int32_t &slot) {
  RC rc = OK_RC;
  slot = INVALID_SLOT;
  if (free_ != INVALID_SLOT) {
    slot = free_;
    free_ = slots_[slot].next;
  } else {
    // 如果缓存中已经没有一个空的页了，则需要进行淘汰。
    for (slot = last_; slot != INVALID_SLOT; slot = slots_[slot].prev) {
      if (slots_[slot].count == 0)
        break;
    }

    if (slot == INVALID_SLOT) {
      return PF_NOBUF;
    }

    // 如果是脏页则写入文件
    if (slots_[slot].is_dirty) {
      if ((rc = write_back(slots_[slot].fd, slots_[slot].num,
                           slots_[slot].buffer)))
        return rc;

      slots_[slot].is_dirty = false;
    }

    // remove page from the hash table and slot from the used buffer list
    if ((rc = table_.remove(slots_[slot].fd, slots_[slot].num)) ||
        (rc = this->unlink(slot)))
      return rc;
  }

  // link slot at the head of the used list
  if ((rc = link_head(slot)))
    return rc;

  return OK_RC;
}

/*
 * read_page - 从文件中读取一个页面
 */
RC PFBuffer::read_page(int32_t fd, Page num, Ptr dst) {
  int64_t offset = num * pagesize + PF_FILE_HDR_SIZE;
  redbase::lseek(fd, offset, L_SET);
  int32_t n = redbase::read(fd, dst, pagesize);
  if (n != pagesize)
    return PF_INCOMPLETEREAD;
  return OK_RC;
}

RC PFBuffer::write_back(int32_t fd, Page num, Ptr src) {
  int64_t offset = num * pagesize + PF_FILE_HDR_SIZE;
  redbase::lseek(fd, offset, L_SET);

  int32_t n = redbase::write(fd, src, pagesize);
  if (n != pagesize)
    return PF_INCOMPLETEWRITE;
  return OK_RC;
}

/*
 * link_head
 *
 * Desc: private. Insert a slot at the head of the used list,
 *       making it the most-recently used slot;
 * In: slot - slot nummber to insert
 * Ret: PF return code
 */
RC PFBuffer::link_head(int32_t slot) {
  // set next and prev pointer of slot entry
  slots_[slot].next = first_;
  slots_[slot].prev = INVALID_SLOT;

  // if list isn't empty, point old first back to this entry
  if (first_ != INVALID_SLOT) {
    slots_[first_].prev = slot;
  }

  first_ = slot;

  // if list was empty, set last to this entry/**
  if (last_ == INVALID_SLOT) {
    last_ = first_;
  }

  return OK_RC;
}

/*
 * unlink
 *
 * Desc: private. unlink the slot from the used list. Assume that slot is valid
 *       Set prev and next pointers to INVALID_SLOT. The caller is responsible
 * to either place the unlink page into the free list or the used list. In: slot
 * - slot number to unlink Ret: PF return code
 */
RC PFBuffer::unlink(int32_t slot) {
  // if slot is at the head of list, set first to next element
  if (first_ == slot)
    first_ = slots_[slot].next;

  // if slot is at the end of list, set last to previous element
  if (last_ == slot)
    last_ = slots_[slot].prev;

  // if slot not at end of list, point next back to previous
  if (slots_[slot].next != INVALID_SLOT)
    slots_[slots_[slot].next].prev = slots_[slot].prev;

  // if slot not at head of list. point prev forward to next
  if (slots_[slot].prev != INVALID_SLOT)
    slots_[slots_[slot].prev].next = slots_[slot].next;

  slots_[slot].prev = INVALID_SLOT;
  slots_[slot].next = INVALID_SLOT;

  return OK_RC;
}

/*
 * insert_free
 *
 * Desc: private. Insert a slot at the head of the free list
 * In: slot - slot number to insert
 * Ret: PF return code
 */
RC PFBuffer::insert_free(int32_t slot) {
  slots_[slot].next = free_;
  slots_[slot].prev = INVALID_SLOT;
  free_ = slot;

  return OK_RC;
}

/*
 * init_slot
 *
 * Desc: private. Initializ PF_Buffer slot to a newly-pinned page for
 *       a newly pinned page
 * In: fd - file descriptor
 *     num - page number
 *     slot - slot number
 * Ret: PF return code
 */
RC PFBuffer::init_slot(int32_t fd, Page num, int32_t slot) {
  slots_[slot].fd = fd;
  slots_[slot].num = num;
  slots_[slot].is_dirty = false;
  slots_[slot].count = 1;

  return OK_RC;
}
