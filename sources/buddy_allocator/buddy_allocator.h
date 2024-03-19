#ifndef _BUDDY_ALLOCATOR_H_
#define _BUDDY_ALLOCATOR_H_

#include <stdint.h>
#include <stdbool.h>
#include "../dllist/dllist.h"

/*
 * Implementation of buddy allocator.
 * It is oriented on using as physical memory allocator in the kernel, so it not using hosted functions (like malloc() or pow()).
 * 
 * == ATTENTION ==
 * It is oriented on 32-bit address space, probably there will be problems when working in 64-bit address space. I am testing only the 32-bit version.
 * 
 * Implementation details:
 * The allocator stores nodes for all blocks:
 * |   0   |
 * | 1 | 2 |
 * |3|4|5|6|
 * Total blocks (nodes) - 7
 * This is a bit wasteful, but for a maximum area size of 4GB, to store all nodes at a node size of 8 bytes would require about 16 megabytes of memory, not much.
 * 
 * If any block is free, it is placed in the free list according to its size.
 * The free lists allows to quickly get a block of the required size, if it.
 * Also allocator stores the size order of the allocated block.
 */

typedef struct {
    dll_node_t dll_node;
} memory_block_node_t;

typedef struct {
    // Main variables
    uintptr_t area_start_addr;
    // The size of the area rounded to largest block size
    size_t area_size;
    // Max order
    uint8_t max_order;
    // Page size
    uint32_t page_size;
    // Should all small blocks be marked as highlighted during initialization
    bool allocate_all_small_blocks;

    // Large block size (2^MAX_ORDER * PAGE_SIZE)
    size_t large_block_size;
    // Small block size (PAGE_SIZE)
    size_t small_block_size;
    // Total number of the large blocks
    uint32_t large_blocks_number;
    // Total number of the small blocks
    uint32_t small_blocks_number;
    // Total of all blocks of all sizes
    uint32_t total_blocks_number;

    // Array of all blocks nodes
    memory_block_node_t* blocks_nodes;
    // Size of this array
    size_t blocks_nodes_memory_size;

    // Array of allocations orders.
    // To free memory by address, we need to know the block size, this array contains the allocation orders, the allocation address is the offset in this array.
    // Stores order + 1, so that it can be determined whether a block is actually allocated. If the value is 0, the block has not been allocated and cannot be released.
    // Protects against re-releasing or releasing unallocated memory.
    uint8_t* allocations_orders;
    // Size of this array
    size_t allocations_orders_memory_size;

    /*
     * An array of free lists, each element of which represents a pointer to the beginning of a separate doubly-linked list for each order.
     * Example:
     * free_blocks_lists[0] is a list of free blocks of size 2^0 * PAGE_SIZE
     * free_blocks_lists[1] is a list of free blocks of size 2^1 * PAGE_SIZE
     * up to
     * free_blocks_lists[MAX_ORDER] is a list of free blocks of size 2^MAX_ORDER * PAGE_SIZE
     */
    doubly_linked_list_t* free_blocks_lists;
    // Size of this array
    uint32_t free_blocks_lists_memory_size;
} buddy_allocator_t;

/*
 * Pre-initializes the allocator, sets internal variables and calculates the size of memory needed.
 * After this function buddy_allocator_init function should be called, which completes initialization with allocated memory for allocator.
 * 
 * allocator_ptr pointer to allocator data
 * area_start_addr start of the memory area
 * area_size size of the memory area must to be larger than 2^max_order * page_size.
 * max_order max order
 * page_size page size must be power of 2
 * allocate_all_small_blocks if true, then all small blocks will be marked as allocated, if false, then all of them are free (normal state).
 * The true value is useful when there are areas occupied by something else in the area controlled by the allocator,
 * in this case, the entire area is marked as occupied and then free pages are released in it using the page-by-page call of the free function.
 * For example, in the OS kernel, we can have one allocator for the entire memory area, but it contains occupied areas (the kernel itself, ACPI, etc.),
 * in this case, we can use allocate_all_small_blocks = true, and then free up unoccupied pages using the free function.
 * required_memory_size_ptr total size of the memory required by the allocator WILL BE PLACED BY THIS FUNCTION in this variable, it is necessary to allocate at least.
 * If it contains 0 after the function call, then the initialization has failed.
 *
 * About required_memory_size_ptr:
 * In order for the allocator to work, it is necessary to allocate memory for its needs.
 * The amount of this memory depends on the size of area (area_size) that the allocator will manage.
 */
extern void buddy_allocator_preinit(buddy_allocator_t* allocator_ptr, uintptr_t area_start_addr, size_t area_size, uint8_t max_order, uint32_t page_size, bool allocate_all_small_blocks, size_t* required_memory_size_ptr);

/*
 * Finishes initialization by initializing the memory required to work the allocator.
 * allocator_ptr pointer to allocator data
 * required_memory_ptr pointer to the memory allocated for the needs of the allocator.
 * The size of this memory must be not less than the size specified in required_total_size_ptr after calling buddy_allocator_preinit function.
 * Outside the area that the allocator will control if allocate_all_small_blocks is false!
 */
extern void buddy_allocator_init(buddy_allocator_t* allocator_ptr, void* required_memory_ptr);

/*
 * Allocates a block of memory, returns a pointer to memory within the allocator memory area
 * allocator_ptr pointer to allocator data
 * size requested memory size, it is rounded down to the nearest block
 * For example, if PAGE_SIZE is 4096, size 4 will be rounded to 4096, 5000 to 8192, 15000 to 16384.
 * The size cannot be larger than PAGE_SIZE * 2^MAX_ORDER
 */
extern void* buddy_allocator_alloc(buddy_allocator_t* allocator_ptr, size_t size);

/*
 * Free the memory allocated by the allocator at the address
 * allocator_ptr pointer to allocator data
 * memory_ptr pointer to memory allocated by allocator
 */
extern void buddy_allocator_free(buddy_allocator_t* allocator_ptr, void* memory_ptr);

#endif
