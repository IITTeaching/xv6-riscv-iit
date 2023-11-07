#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "vm.h"
volatile static int started = 0;
struct shared_page shared_pages[MAX_SHARED_PAGES];
// start() jumps here in supervisor mode on all CPUs.
void
main()
{
  if(cpuid() == 0){
    consoleinit();
    printfinit();
    printf("\n");
    printf("xv6 kernel is booting\n");
    printf("\n");
    kinit();         // physical page allocator
    kvminit();       // create kernel page table
    kvminithart();   // turn on paging
    procinit();      // process table
    trapinit();      // trap vectors
    trapinithart();  // install kernel trap vector
    plicinit();      // set up interrupt controller
    plicinithart();  // ask PLIC for device interrupts
    binit();         // buffer cache
    iinit();         // inode table
    fileinit();      // file table
    virtio_disk_init(); // emulated hard disk
    userinit();      // first user process

      // 初始化 shared_pages 数组
      for (int i = 0; i < MAX_SHARED_PAGES; i++) {
          shared_pages[i].key = -1; // 表示共享页面未被使用
          shared_pages[i].physical_address = 0; // 或者为这里分配一个合适的物理地址
          shared_pages[i].ref_count = 0;
          shared_pages[i].creator_pid = 0;
          shared_pages[i].num_shared_procs = 0;
          memset(shared_pages[i].shared_procs, 0, sizeof(shared_pages[i].shared_procs));
          initlock(&shared_pages[i].shared_pages_lock, "shared_page");
      }
    __sync_synchronize();
    started = 1;
  } else {
    while(started == 0)
      ;
    __sync_synchronize();
    printf("hart %d starting\n", cpuid());
    kvminithart();    // turn on paging
    trapinithart();   // install kernel trap vector
    plicinithart();   // ask PLIC for device interrupts
  }

  scheduler();        
}
