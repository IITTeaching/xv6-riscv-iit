#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "stddef.h"

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
    for (int i = 0; i < 5; i++) {
        pte_t pte = pagetable[i];
//        if (pte & PTE_V) { // 如果页表项有效
            uint64 pa = PTE2PA(pte); // 从页表项获取物理地址
            uint64 flags = PTE_FLAGS(pte); // 获取页表项的标志
            printf("VA: %p, PA: %p, flags: %x\n", (void*)((uint64)i * PGSIZE), (void*)pa, flags);
            if ((pte & ( PTE_U)) == ( PTE_U)) { // 如果是用户页，可能有子页表
                pagetable_t child_pagetable = (pagetable_t)(pa); // 转换物理地址到虚拟地址
                vmprint(child_pagetable); // 递归打印子页表
            }
//        }
    }
}
// 测试物理内存页是否可用的函数
int test_physical_page(void *page) {
    // 保存旧的数据
    unsigned int old_data = *(unsigned int*)page;

    // 写入测试数据
    *(unsigned int*)page = 0xDEADBEEF;

    // 读取测试数据
    if (*(unsigned int*)page == 0xDEADBEEF) {
        // 测试通过，打印消息
        printf("Physical page test PASSED: Address = %p, Data = 0x%X\n", page, V2P(page));

        // 恢复旧的数据
        *(unsigned int*)page = old_data;
        return 1; // 成功
    } else {
        // 测试失败，打印消息
        printf("Physical page test FAILED: Address = %p\n", page);
        return 0; // 失败
    }
}

uint64
sys_newsharedpage(void)
{
    int key;
    struct proc *p = myproc();

    // Get the key from system call arguments
    argint(0, &key);
    printf("sys_newsharedpage: key = %d\n", key);
    acquire(&pid_lock); // Assume you have a global pid_lock to protect the entire shared_pages array

    for(int i = 0; i < MAX_SHARED_PAGES; i++) {
        if(shared_pages[i].key == key && shared_pages[i].ref_count > 0) {
            release(&pid_lock);
            return -1; // Key already exists, return error code
        }
    }

    for(int i = 0; i < MAX_SHARED_PAGES; i++) {
        if(shared_pages[i].ref_count == 0) { // Look for a free shared page
            // Allocate a physical memory page in xv6 and initialize it to zero
            char *page = kalloc();
            if(page == 0){
                release(&pid_lock);
                return -1; // Unable to allocate physical page
            }
            test_physical_page(page); // Test if the physical page is usable
            memset(page, 0, PGSIZE); // PGSIZE is typically 4096 bytes

            // Initialize the shared page structure
            shared_pages[i].key = key;
            shared_pages[i].ref_count = 1;
            shared_pages[i].creator_pid = p->pid;
            initlock(&shared_pages[i].shared_pages_lock, "shared_page");
            shared_pages[i].physical_address = page; // Save the address of the allocated physical page
            uint64 pa = V2P(page);

            memset(shared_pages[i].shared_procs, 0, sizeof(shared_pages[i].shared_procs));
            shared_pages[i].num_shared_procs = 0;
            shared_pages[i].shared_procs[shared_pages[i].num_shared_procs++] = p->pid;

            // We need to map the physical page to the process's virtual address space
            // This requires a suitable virtual address, determined by some mapping strategy
            uint64 va = PGROUNDUP(p->sz); // Get the next page-aligned address in the process's address space
            shared_pages[i].virtual_address = va;

            // Print page table structure before mapping
            if(mappages(p->pagetable, va, PGSIZE, pa, PTE_V|PTE_W|PTE_R|PTE_X|PTE_U) != 0){
                kfree(page);
                shared_pages[i].ref_count = 0; // Decrement reference count, as mapping failed
                release(&pid_lock);
                return -1; // Mapping failed
            } else {
                // Force the mapping to physical page by accessing va
                printf("sys_newsharedpage: mappages va = %p, pa = %p\n", (void*)va, (void*)pa);
            }
            p->sz = va + PGSIZE; // Increase the size of the process's address space

            pte_t existing_pte;
            pte_t *pte1 = &existing_pte;
            *pte1 = PA2PTE((uint64)page) | PTE_V|PTE_W|PTE_R|PTE_X|PTE_U;
            printf("sys_newsharedpage: mappages va = %p, pte = %d\n", (void*)va, *pte1);

            printf("sys_newsharedpage: walk va = %p, page = %p\n", (void*)va, (void*)page);
            printf("sys_newsharedpage: walk va int = %d, page = %p\n", va, (void*)page);
            // Use walkaddr to check if mapping was successful
            uint64 padd = walkaddr(p->pagetable, va);
            pte_t* pte = walk2(p->pagetable, va, 0);
            uint64 pte_flags = PTE_FLAGS(*pte);
            uint64 pte_addr = PTE2PA(*pte);
            printf("sys_newsharedpage: pte_addr = %p, pte_flags = %p\n", (void*)pte_addr, (void*)pte_flags);

            printf("sys_newsharedpage: walk2 va = %p, pte = %d\n", (void*)va, (void*)*pte);
            printf("sys_newsharedpage: walk2 int va = %d, pte = %p\n", va, (void*)pte);
            if(pa == 0){
                // Mapping was not successful
                kfree(page);
                shared_pages[i].ref_count = 0;
                release(&pid_lock);
                return -1;
            }
            printf("sys_newsharedpage: va walkaddr = %p, pa = %p\n", (void*)va, (void*)padd);
            printf("sys_newsharedpage: va walkaddr = %p, pa = %d\n", (void*)va, padd);

            release(&pid_lock);
            return va; // Successful, return the virtual address of the shared page
        }
    }

    release(&pid_lock);
    return -1; // No available shared page
}


uint64
sys_readsharedpage(void)
{
    int key;
    argint(0, &key);
    int offset;
    argint(1, &offset);
    int n;
    argint(2, &n);
    uint64 addr;  // Change buffer to be uint64 to match argaddr function signature
    argaddr(3, &addr);
    char *buffer = (char*)addr;
    char *dst = kalloc();
    if(copyin(myproc()->pagetable, dst, addr, n) < 0) {
        // Handle error: copy failed
    }
    printf("sys_readsharedpage: key = %d, offset = %d, n = %d, addr = %p\n", key, offset, n, (void*)addr);

    struct proc *p = myproc(); // Get the current process structure pointer
    struct shared_page *spage = &shared_pages[0];
    // Search for the matching shared page
    for (int i = 0; i < MAX_SHARED_PAGES; i++) {
        printf("sys_readsharedpage: key = %d, shared_pages[%d].key = %d\n", key, i, shared_pages[i].key);
        if (shared_pages[i].key == key) {
            acquire(&shared_pages[i].shared_pages_lock); // Acquire the lock for the specific shared page

            // Update process association
            int found = -1;
            for (int j = 0; j < shared_pages[i].num_shared_procs; j++) {
                if (shared_pages[i].shared_procs[j] == p->pid) {
                    found = j;
                    break;
                }
            }
            if (found == -1 && shared_pages[i].num_shared_procs < MAX_SHARED_PROC) {
                shared_pages[i].shared_procs[shared_pages[i].num_shared_procs++] = p->pid;
            }
            // Check if a mapping can be found in the process's address space
            pte_t *pte = walk(p->pagetable, shared_pages[i].virtual_address, 0);
            // Check if the mapping exists
            printf("*pte & PTE_V = %d, PTE2PA(*pte) = %p, shared_pages[%d].physical_address = %p\n", *pte & PTE_V, (void*)PTE2PA(*pte), i, (void*)shared_pages[i].physical_address);

            if (pte && (*pte & PTE_V) && PTE2PA(*pte) == (uint64)shared_pages[i].physical_address) {
                // Page is already mapped to the process's address space
                // Data can be directly read from the virtual address to the buffer
                if (offset + n <= PGSIZE) {
                    printf("sys_readsharedpage: memmove buffer = %p, shared_pages[%d].virtual_address = %p, offset = %d, n = %d\n", buffer, i, (void*)shared_pages[i].virtual_address, offset, n);

                    test_physical_page(P2V(PTE2PA(*pte)));
                    char *src = P2V(PTE2PA(*pte)) + offset;
                    for (int j = 0; j < n; j++) {
                        // Assuming buffer is a large enough array of characters, we can copy data byte by byte
                        dst[j] = src[j];
                    }
                    copyout(p->pagetable, addr, dst, n);
                } else {
                    release(&shared_pages[i].shared_pages_lock);
                    return -2; // Error: Request exceeds page boundary
                }
            } else if (!pte) {
                // Page is not yet mapped, mapping needs to be created
                if (mappages(p->pagetable, shared_pages[i].virtual_address, PGSIZE, (uint64)shared_pages[i].physical_address, PTE_V | PTE_U | PTE_R | PTE_W) < 0) {
                    release(&shared_pages[i].shared_pages_lock);
                    return -3; // Error: Unable to map page
                }
                // After successful mapping, read data from the shared page to the buffer
                memmove(buffer, (char*)shared_pages[i].virtual_address + offset, 1);
            }

            release(&shared_pages[i].shared_pages_lock);
            // spage = &shared_pages[i];
            break;
        } else {
            printf("Not equal\n");
        }
    }

    if (!spage) {
        return -1; // Error: Shared page not found
    }

    return 0; // Success
}

uint64
sys_writesharedpage(void)
{
    int key;
    argint(0, &key);
    int offset;
    argint(1, &offset);
    int n;
    argint(2, &n);
    uint64 addr;  // Change buffer to be uint64 to match argaddr function signature
    argaddr(3, &addr);
    char *buffer = (char*)addr;
    char *dst = kalloc();
    if(copyin(myproc()->pagetable, dst, addr, n) < 0) {
        // 处理错误: 复制失败
    }
    printf("sys_readsharedpage: key = %d, offset = %d, n = %d, addr = %p\n", key, offset, n, (void*)addr);

    struct proc *p = myproc(); // 获取当前进程结构体指针
    struct shared_page *spage = &shared_pages[0];
    // 查找匹配的共享页面
    for (int i = 0; i < MAX_SHARED_PAGES; i++) {
        printf("sys_readsharedpage: key = %d, shared_pages[%d].key = %d\n", key, i, shared_pages[i].key);
        if (shared_pages[i].key == key) {
            acquire(&shared_pages[i].shared_pages_lock); // 获取特定共享页面的锁


            // 更新进程关联
            int found = -1;
            for (int j = 0; j < shared_pages[i].num_shared_procs; j++) {
                if (shared_pages[i].shared_procs[j] == p->pid) {
                    found = j;
                    break;
                }
            }
            if (found == -1 && shared_pages[i].num_shared_procs < MAX_SHARED_PROC) {
                shared_pages[i].shared_procs[shared_pages[i].num_shared_procs++] = p->pid;
            }
// 检查是否能在进程地址空间中找到映射
            pte_t *pte = walk(p->pagetable, shared_pages[i].virtual_address, 0);
            // 检查映射是否存在
            printf("*pte & PTE_V = %d, PTE2PA(*pte) = %p, shared_pages[%d].physical_address = %p\n", *pte & PTE_V, (void*)PTE2PA(*pte), i, (void*)shared_pages[i].physical_address);

            if (pte && (*pte & PTE_V) && PTE2PA(*pte) == (uint64)shared_pages[i].physical_address) {
                // 页面已经映射到进程的地址空间
                // 可以直接从虚拟地址读取数据到缓冲区
//                test_physical_page((void*)shared_pages[i].physical_address);
                if (offset + n <= PGSIZE) {
                    printf("sys_readsharedpage: memmove buffer = %p, shared_pages[%d].virtual_address = %p, offset = %d, n = %d\n", buffer, i, (void*)shared_pages[i].virtual_address, offset, n);

                    test_physical_page(P2V(PTE2PA(*pte)));
                    char *src = P2V(PTE2PA(*pte)) + offset;
                    for (int j = 0; j < n; j++) {
                        // 假设 buffer 是一个足够大的字符数组，我们可以逐字节拷贝数据
                        src[j] = dst[j];
                    }
//                    copyout(p->pagetable, addr, dst, n);
//                    memmove(buffer, (char*)shared_pages[i].virtual_address + offset, n);
                } else {
                    release(&shared_pages[i].shared_pages_lock);
                    return -2; // 错误：请求超出页面边界
                }
            } else if (!pte) {
                // 页面尚未映射，需要创建映射
                if (mappages(p->pagetable, shared_pages[i].virtual_address, PGSIZE, (uint64)shared_pages[i].physical_address, PTE_V | PTE_U | PTE_R | PTE_W) < 0) {
                    release(&shared_pages[i].shared_pages_lock);
                    return -3; // 错误：无法映射页面
                }
                // 映射成功后，从共享页面读取数据到缓冲区
                memmove(buffer, (char*)shared_pages[i].virtual_address + offset, 1);
            }

            release(&shared_pages[i].shared_pages_lock);
//            spage = &shared_pages[i];
            break;
        } else {
            printf("no equal\n");
        }
    }

    if (!spage) {
        return -1; // 错误：未找到共享页面
    }



    return 0; // 成功
}
uint64 sys_freesharedpage(void)
{
    int key;

    // First, retrieve the key from the system call argument.
    argint(0, &key);

    for(int i = 0; i < MAX_SHARED_PAGES; i++) {
        acquire(&shared_pages[i].shared_pages_lock);
        if(shared_pages[i].key == key && shared_pages[i].creator_pid == myproc()->pid) {
            if(--shared_pages[i].ref_count == 0) {

                // Get the pagetable of the current process
                pte_t *pagetable = myproc()->pagetable;
                // Get the virtual address that is mapped

                uint64 va = shared_pages[i].virtual_address; // Need a method to determine the virtual address of this page
                printf("sys_freesharedpage: va = %d\n", va);
                // pte_t *pte = walk(pagetable, va, 0);
                // if (pte && (*pte & PTE_V)) {
                printf("sys_freesharedpage calling before\n");
                pte_t *pte = walk(pagetable, va, 0);
                uint64 pa = PTE2PA(*pte);
                test_physical_page(P2V(pa));
                uvmunmap(pagetable, va, 0, 0);
                myproc()->sz -= PGSIZE;
                kfree(P2V(pa));
                *pte = 0;

                // } else {
                //    printf("sys_freesharedpage: va = 0x%x is not mapped\n", va);
                // }

                // Free the physical memory
                // kfree(shared_pages[i].physical_address);
                // Here you would also handle the deallocation of the page and any other resources.
                // kfree(shared_pages[i].physical_address);

                // Reset all the fields of the shared page to indicate it is free
                shared_pages[i].key = 0;
                shared_pages[i].physical_address = 0;
                shared_pages[i].creator_pid = 0;
                memset(shared_pages[i].shared_procs, 0, sizeof(shared_pages[i].shared_procs)); // Clear the shared process list
                shared_pages[i].num_shared_procs = 0;
                release(&shared_pages[i].shared_pages_lock); // Release the lock for this specific shared page
                return 0; // Successfully freed the page
            } else {
                // If ref_count is not zero or does not become zero after decrement, do not free the page.
                release(&shared_pages[i].shared_pages_lock);
                return -1; // Indicate that the page cannot be freed
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

uint64 sys_get_shared_page_info(void)
{
    int key;
    // Get the argument 'key'
    argint(0, &key);

    // Lock to synchronize access to the shared_pages array
    acquire(&pid_lock);

    for (int i = 0; i < MAX_SHARED_PAGES; i++) {
        // Check if there's a shared page matching the key
        if (shared_pages[i].ref_count > 0 && shared_pages[i].key == key) {
            print_page_mapping(myproc()->pagetable, (uint64)shared_pages->virtual_address);
            // Print the information of the shared page
            printf("Shared Page Found: Key=%d, physical_address=0x%x, virtual_address=%d, Ref Count=%d, Creator PID=%d, Number of Shared Procs=%d\n",
                   shared_pages[i].key,
                   shared_pages[i].physical_address,
                   shared_pages[i].virtual_address,
                   shared_pages[i].ref_count,
                   shared_pages[i].creator_pid,
                   shared_pages[i].num_shared_procs);

            release(&pid_lock);
            return 0; // Successfully found and printed information
        } else {
            printf("No Shared Page Found:\n");
        }
    }

    // No matching key found
    release(&pid_lock);
    return -1;
}
