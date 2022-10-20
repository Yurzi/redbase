#ifndef __REDBASE_META_H__
#define __REDBASE_META_H__

/*
 * some global variable
 * 一些全局变量
 *
 */

// Return codes
typedef int RC;
// Pointer def
typedef char *Ptr;

#define OK_RC 0

#define PAGE_LIST_END   -1
#define PAGE_FULLY_USED -2

#define START_PF_ERR (-1)
#define END_PF_ERR   (-100)

#define START_PF_WARN 1
#define END_PF_WARN   100

// ALL_PAGES is defined and used by the ForcePages method defined in RM
// and PF layers
const int ALL_PAGES = -1;

#endif
