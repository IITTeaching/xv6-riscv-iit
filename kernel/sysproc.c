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
void vmprint(pagetable_t pagetable) {
    printf("page table %p\n", pagetable);
    for (int i = 0; i < 512; i++) {
        pte_t pte = pagetable[i];
        if (pte & PTE_V) { // 如果页表项有效
            uint64 pa = PTE2PA(pte); // 从页表项获取物理地址
            uint64 flags = PTE_FLAGS(pte); // 获取页表项的标志
            printf("VA: %p, PA: %p, flags: %x\n", (void*)((uint64)i * PGSIZE), (void*)pa, flags);
            if ((pte & (PTE_V | PTE_U)) == (PTE_V | PTE_U)) { // 如果是用户页，可能有子页表
                pagetable_t child_pagetable = (pagetable_t)(pa); // 转换物理地址到虚拟地址
                vmprint(child_pagetable); // 递归打印子页表
            }
        }
    }
}
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
            shared_pages[i].virtual_address = va;
            if(mappages(p->pagetable, va, PGSIZE, (uint64)page, PTE_V|PTE_W|PTE_R|PTE_X|PTE_U) != 0){
                kfree(page);
                shared_pages[i].ref_count = 0; // 取消引用，因为映射失败
                release(&pid_lock);
                return -1; // 映射失败
            }

            // 更新进程的sz，表示已经映射了新的页
            p->sz = va + PGSIZE;
            printf("sys_newsharedpage:打印页表\n");
            vmprint(p->pagetable);

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
    int key;

    // First, retrieve the key from the system call argument.
    argint(0, &key);



    for(int i = 0; i < MAX_SHARED_PAGES; i++) {
        acquire(&shared_pages[i].shared_pages_lock);
        if(shared_pages[i].key == key && shared_pages[i].creator_pid == myproc()->pid) {
            if(--shared_pages[i].ref_count == 0) {

                // 获取当前进程的页表
                pte_t *pagetable = myproc()->pagetable;
                printf("sys_freesharedpage:打印页表\n");
                vmprint(pagetable);
                // 获取映射的虚拟地址

                uint64 va = shared_pages[i].virtual_address;/* 需要一个方法来确定这个页面的虚拟地址 */;
                printf("sys_freesharedpage: va = %d\n", va);
//                pte_t *pte = walk(pagetable, va, 0);
//                if (pte && (*pte & PTE_V)) {
                printf("sys_freesharedpage前调用\n");
                uvmunmap(pagetable, va, 0, 1);
//                } else {
//                    printf("sys_freesharedpage: va = 0x%x 没有映射\n", va);
//                }

                // 释放物理内存
//                kfree(shared_pages[i].physical_address);
                // Here you would also handle the deallocation of the page and any other resources.
//                 kfree(shared_pages[i].physical_address);

                // Reset all the fields of the shared page to indicate it is free
                shared_pages[i].key = 0;
                shared_pages[i].physical_address = 0;
                shared_pages[i].creator_pid = 0;
                memset(shared_pages[i].shared_procs, 0, sizeof(shared_pages[i].shared_procs)); // Clear the shared process list
                shared_pages[i].num_shared_procs = 0;
                release(&shared_pages[i].shared_pages_lock); // Release the lock for this specific shared page
                return 0; // Successfully freed the page
            } else {
                // If ref_count is not greater than 0 or does not reach 0 after decrement, do not free the page.
                release(&shared_pages[i].shared_pages_lock);
                return -1; // Indicate that page cannot be freed
            }
        } else {
            // If the current page is not the one we're looking for, release the lock and continue.
            release(&shared_pages[i].shared_pages_lock);
        }
    }
    return -1; // Key not found or PID does not match.
}
void print_page_mapping(pagetable_t pagetable, uint64 va) {
    pte_t *pte;
    pte = walk(pagetable, va, 0);  // Get the PTE for the virtual address.

    if (pte && (*pte & PTE_R)) {
        uint64 pa = PTE2PA(*pte);  // Extract the physical address from the PTE.
        printf("Virtual Address: %d is mapped to Physical Address: 0x%x\n", va, pa);
    } else {
        printf("Virtual Address: %d is not mapped\n", va);
    }
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
            print_page_mapping(myproc()->pagetable, (uint64)shared_pages->virtual_address);
            // 打印共享页面的信息
            printf("Shared Page Found: Key=%d, physical_address=0x%x, virtual_address=%d, Ref Count=%d, Creator PID=%d, Number of Shared Procs=%d\n",
                    shared_pages[i].key,
                    shared_pages[i].physical_address,
                    shared_pages[i].virtual_address,
                    shared_pages[i].ref_count,
                    shared_pages[i].creator_pid,
                    shared_pages[i].num_shared_procs);

            release(&pid_lock);
            return 0; // 成功找到并打印信息
        } else {
            printf("Not Shared Page Found:\n");
        }
    }

    // 未找到匹配的key
    release(&pid_lock);
    return -1;
}