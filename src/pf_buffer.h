#ifndef __PF_BUFFER_H__
#define __PF_BUFFER_H__

#include <cstdint>
#include <cstring>
#include <memory>

#include "pf_hashtable.h"
#include "pf_internal.h"
#include "pf_meta.h"
#include "redbase_meta.h"

#define INVALID_SLOT (-1)

class PFBuffer {
public:
  const static int32_t capacity = 40;
  const static int32_t pagesize = PF_PAGE_SIZE + sizeof(PFPageHdr);

private:
  struct BufferSlot {
    bool is_dirty;
    uint32_t count;  // 记录被操作的次数
    int32_t fd;
    Page num;
    int32_t prev;
    int32_t next;
    char buffer[pagesize];  // 内存空间
    BufferSlot()
      : is_dirty(false)
      , count(0)
      , fd(-1)
      , num(-1) {
      memset(buffer, 0, sizeof(buffer));
    }
  };

private:
  PFBuffer();
  // 删除所有可能的构造函数  确保单例模式
  PFBuffer(const PFBuffer &oth) = delete;
  PFBuffer(const PFBuffer &&oth) = delete;
  void operator=(const PFBuffer &oth) = delete;

public:
  static std::shared_ptr<PFBuffer> Instance() {
    if (instance_ == nullptr) {
      instance_ = std::shared_ptr<PFBuffer>(new PFBuffer());
    }
    return instance_;
  }
  static void DestroyBuffer() { instance_.reset(); }

public:
  RC get_page(int32_t fd, Page num, Ptr &addr);
  RC alloc_page(int32_t fd, Page num, Ptr &addr);
  RC mark_dirty(int32_t fd, Page num);
  RC unpin(int32_t fd, Page num);
  RC pin(int32_t fd, Page num);
  RC force_page(int32_t fd, Page num);
  void clear_file_pages(int32_t fd);
  RC flush(int32_t fd);

private:
  int32_t fetch_avaliable_slot();
  RC read_page(int32_t fd, Page num, Ptr dst);
  RC write_back(int32_t fd, Page num, Ptr src);

private:
  RC link_head(int32_t slot);
  RC unlink(int32_t slot);
  RC insert_free(int32_t slot);

private:
  static std::shared_ptr<PFBuffer> instance_;

private:
  BufferSlot slots_[capacity];
  PFHashTable table_;

  int32_t first_;
  int32_t last_;
  int32_t free_;
};

#endif
