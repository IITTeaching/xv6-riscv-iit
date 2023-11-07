#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

extern struct spinlock pid_lock; // Make sure pid_lock is declared and initialized elsewhere

uint64
sys_newsharedpage(void)
{
    int key;
    struct proc *p = myproc();

    // 从系统调用的参数中获取 key
    argint(0, &key);
    printf("sys_newsharedpage: key = %d\n", key);
    acquire(&pid_lock); // 假设你有一个全局的pid锁来保护整个shared_pages数组

    for(int i = 0; i < MAX_SHARED_PAGES; i++) {
        if(shared_pages[i].key == key && shared_pages[i].ref_count > 0) {
            release(&pid_lock);
            return -1; // 键已存在，返回错误码
        }
    }

    for(int i = 0; i < MAX_SHARED_PAGES; i++) {
        if(shared_pages[i].ref_count == 0) { // 查找空闲的共享页面
            // 在 xv6 中分配一页物理内存，并将其初始化为0
            char *page = kalloc();
            if(page == 0){
                release(&pid_lock);
                return -1; // 无法分配物理页
            }
            memset(page, 0, PGSIZE); // PGSIZE 是页面大小，通常是4096字节

            // 初始化共享页面结构
            shared_pages[i].key = key;
            shared_pages[i].ref_count = 1;
            shared_pages[i].creator_pid = p->pid;
            initlock(&shared_pages[i].shared_pages_lock, "shared_page");
            shared_pages[i].physical_address = page; // 保存分配的物理页的地址

            memset(shared_pages[i].shared_procs, 0, sizeof(shared_pages[i].shared_procs));
            shared_pages[i].num_shared_procs = 0;
            shared_pages[i].shared_procs[shared_pages[i].num_shared_procs] = p->pid;
            shared_pages[i].num_shared_procs++;

            // 这里需要将物理页映射到进程的虚拟地址空间中
            // 需要使用适当的虚拟地址，可能是通过某种映射策略来确定
            uint64 va = PGROUNDUP(p->sz); // 得到进程地址空间的下一页对齐地址
            if(mappages(p->pagetable, va, PGSIZE, (uint64)page, PTE_W|PTE_R|PTE_X|PTE_U) != 0){
                kfree(page);
                shared_pages[i].ref_count = 0; // 取消引用，因为映射失败
                release(&pid_lock);
                return -1; // 映射失败
            }

            // 更新进程的sz，表示已经映射了新的页
            p->sz = va + PGSIZE;

            release(&pid_lock);
            return va; // 成功，返回共享页面的虚拟地址
        }
    }

    release(&pid_lock);
    return -1; // 没有可用的共享页面
}
// ... 类似的实现其他函数


uint64
sys_readsharedpage(void)
{
//    int key = myproc()->trapframe->a0;
    return 1;
}
uint64
sys_writesharedpage(void)
{
//    int key = myproc()->trapframe->a0;
    return 1;
}

uint64
sys_freesharedpage(void)
{
//    int key = myproc()->trapframe->a0;
    return 1;
}

uint64
sys_get_shared_page_info(void)
{
    int key;
    // 获取参数 key
    argint(0, &key);

    // 锁定以同步对shared_pages数组的访问
    acquire(&pid_lock);

    for (int i = 0; i < MAX_SHARED_PAGES; i++) {
        // 检查是否有与key匹配的共享页面
        if (shared_pages[i].ref_count > 0 && shared_pages[i].key == key) {
            // 打印共享页面的信息
            printf("Shared Page Found: Key=%d, Address=0x%x, Ref Count=%d, Creator PID=%d, Number of Shared Procs=%d\n",
                    shared_pages[i].key,
                    shared_pages[i].physical_address,
                    shared_pages[i].ref_count,
                    shared_pages[i].creator_pid,
                    shared_pages[i].num_shared_procs);

            release(&pid_lock);
            return 0; // 成功找到并打印信息
        }
    }

    // 未找到匹配的key
    release(&pid_lock);
    return -1;
}