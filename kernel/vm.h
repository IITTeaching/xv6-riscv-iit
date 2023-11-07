#ifndef VM_H
#define VM_H

#include "spinlock.h"  // 替换为正确的头文件名

#define MAX_SHARED_PAGES 100
#define MAX_SHARED_PROC 10

// 共享页面数据结构的声明
struct shared_page {
    int key;
    char *physical_address; // 页的物理地址指针
    uint64 virtual_address; // Virtual address in the creator's address space
    int ref_count;          // 引用计数
    int creator_pid;        // 创建此共享页面的进程的PID
    int shared_procs[MAX_SHARED_PROC];  // 与该页面关联的进程ID数组。
    int num_shared_procs;     // 当前与此页面共享的进程数量
    struct spinlock shared_pages_lock;   // 每个 shared_page 的自己的锁
};

// 共享页面数组的外部声明
extern struct shared_page shared_pages[MAX_SHARED_PAGES];

#endif // VM_H
