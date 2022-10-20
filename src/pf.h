#ifndef __PF_H__
#define __PF_H__

#include <cstdint>
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
