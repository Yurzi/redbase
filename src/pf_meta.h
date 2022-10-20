#ifndef __PF_META_H__
#define __PF_META_H__

#include <cstdint>

#define CREATION_MASK 0600  // r/w privileges to owner only

#define PF_PAGE_LIST_END -1  // end of list of free pages
#define PF_PAGE_USED     -2  // page is being used

/*
 * Page 页面标识
 */

typedef int32_t Page;
/*
 * Page Size
 * 每个页面的大小，减去页面头部的信息（PageHdr），由于文件循环引用
 * 这里使用int代替，其大小一致
 */

const int PF_PAGE_SIZE = 4096 - sizeof(int32_t);

#endif
