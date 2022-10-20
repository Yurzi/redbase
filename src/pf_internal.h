#ifndef __PF_INTERNAL_H__
#define __PF_INTERNAL_H__

struct PFPageHdr {
  /* 指向下一个free的页面，此时该页面也是空闲的，可以有以下取值：
   * - PF_PAGE_LIST_END -> 页面是最后一个空闲页面
   * - PF_PAGE_USED -> 页面不是空闲的
   */
  int free;
};

#endif
