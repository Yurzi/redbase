#ifndef __PF_INTERNAL_H__
#define __PF_INTERNAL_H__

#include "pf_meta.h"
#include <cstdint>

/* 指向下一个free的页面，此时该页面也是空闲的，可以有以下取值：
 * - PF_PAGE_LIST_END -> 页面是最后一个空闲页面
 * - PF_PAGE_USED -> 页面不是空闲的
 */
struct PFPageHdr {
  int free;
};

/*
 * 文件头的部分
 * |free| - PFFileHdr
 * |size|
 *  ...
 * |free| - PFPageHdr
 */
struct PFFileHdr {
  int32_t free;
  uint32_t size;
};

const int32_t PF_FILE_HDR_SIZE = PF_PAGE_SIZE + sizeof(PFPageHdr);

#define MEMORY_FD (-1)

#endif
