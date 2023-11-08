#ifndef VM_H
#define VM_H

#include "spinlock.h"

#define MAX_SHARED_PAGES 100
#define MAX_SHARED_PROC 10

// Declaration of shared page data structure
struct shared_page {
    int key;
    char *physical_address; // Pointer to the physical address of the page
    uint64 virtual_address; // Virtual address in the creator's address space
    int ref_count;          // Reference count
    int creator_pid;        // PID of the process that created this shared page
    int shared_procs[MAX_SHARED_PROC];  // Array of process IDs associated with this page
    int num_shared_procs;   // Current number of processes sharing this page
    struct spinlock shared_pages_lock;   // Own lock for each shared_page
};

// External declaration of the array of shared pages
extern struct shared_page shared_pages[MAX_SHARED_PAGES];


#endif // VM_H
