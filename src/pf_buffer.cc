#include "pf_buffer.h"
#include "pf_error.h"
#include "pf_internal.h"
#include "redbase_meta.h"
#include "utils.h"
#include <unistd.h>

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
 *  get_page - 获取一个指向缓存空间的指针
 */
RC PFBuffer::get_page(int32_t fd, Page num, Ptr &addr) {
  int32_t slot = table_.search(fd, num);
  if (slot != INVALID_SLOT) {
    // 页面已经在缓存中 增加pin计数
    slots_[slot].count++;
    addr = slots_[slot].buffer;
    return OK_RC;
  }
  // 分配一个空的页
  RC rc = alloc_page(fd, num, addr);
  if (rc != OK_RC)
    return rc;
  // 读入数据
  read_page(fd, num, addr);

  return OK_RC;
}

/*
 * alloc_page - 分配一个页面
 */
RC PFBuffer::alloc_page(int32_t fd, Page num, Ptr &addr) {
  int32_t slot = table_.search(fd, num);
  if (slot != INVALID_SLOT)
    return PF_PAGEINBUF;

  // 分配空页
  slot = fetch_avaliable_slot();
  if (slot == INVALID_SLOT)
    return PF_NOBUF;

  table_.insert(fd, num, slot);
  slots_[slot].fd = fd;
  slots_[slot].num = num;
  slots_[slot].count = 1;
  slots_[slot].is_dirty = false;
  this->unlink(slot);
  this->insert_free(slot);

  addr = slots_[slot].buffer;
  return OK_RC;
}

/*
 * mark_dirty - 将页面标记为dirty
 */
RC PFBuffer::mark_dirty(int32_t fd, Page num) {
  int32_t slot = table_.search(fd, num);
  if (slot == INVALID_SLOT)
    return PF_PAGENOTINBUF;

  if (slots_[slot].count == 0)
    return PF_PAGEUNPINNED;
  // 标记为脏页
  slots_[slot].is_dirty = true;
  return OK_RC;
}

RC PFBuffer::unpin(int32_t fd, Page num) {
  int32_t slot = table_.search(fd, num);
  if (slot != INVALID_SLOT)
    return PF_PAGENOTINBUF;
  slots_[slot].count -= 1;
  return OK_RC;
}
RC PFBuffer::pin(int32_t fd, Page num) { return OK_RC; }
RC PFBuffer::force_page(int32_t fd, Page num) { return OK_RC; }
void PFBuffer::clear_file_pages(int32_t fd) {}
RC PFBuffer::flush(int32_t fd) { return OK_RC; }

/*
 * search_avaliable_node - 获取一个可用的slot 使用LRU
 */
int32_t PFBuffer::fetch_avaliable_slot() {
  RC rc = OK_RC;
  int32_t slot = 0;
  if (free_ != INVALID_SLOT) {
    slot = free_;
    free_ = slots_[slot].next;
    return slot;
  }
  // 如果缓存中已经没有一个空的页了，则需要进行淘汰。
  for (slot = last_; slot != INVALID_SLOT; slot = slots_[slot].prev) {
    if (slots_[slot].count == 0)
      break;
  }

  if (slot == INVALID_SLOT) {
    return INVALID_SLOT;
  }

  // 如果是脏页则写入文件
  if (slots_[slot].is_dirty) {
    if ((rc =
           write_back(slots_[slot].fd, slots_[slot].num, slots_[slot].buffer)))
      return rc;

    slots_[slot].is_dirty = false;
  }

  // remove page from the hash table and slot from the used buffer list
  if ((rc = table_.remove(slots_[slot].fd, slots_[slot].num)) || (rc = this->unlink(slot)))
    return rc;
  
  if ((rc = link_head(slot)))
    return rc;
  return slot;
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
 *       Set prev and next pointers to INVALID_SLOT. The caller is responsible to
 *       either place the unlink page into the free list or the used list.
 * In: slot - slot number to unlink
 * Ret: PF return code
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
