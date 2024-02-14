#ifndef _BUDDY_ALLOCATOR_
#define _BUDDY_ALLOCATOR_

#include <stdint.h>
#include "../dllist/dllist.h"
#include "../stack/stack.h"

/*
 * Implementation of buddy allocator.
 * It is oriented on using physical memory as an allocator in the kernel, so it avoids using hosted functions (like malloc or pow).
 * == ATTENTION ==
 * It is oriented on 32-bit address space, probably there will be problems when working in 64-bit address space. I am testing only the 32-bit version.
 * 
 * Implementation details:
 * 
 */

#define BUDDY_ALLOCATOR_MAX_ORDER 10
#define BUDDY_ALLOCATOR_PAGE_SIZE 4096

typedef struct {
    dll_node_t dll_node;
    struct block_node_t* buddy_block_ptr;
} block_node_t;

typedef struct {
    // Main variables
    uint32_t area_start_addr;
    // The size of the area rounded to largest block size.
    uint32_t area_size;

    // Large block size (2^MAX_ORDER * PAGE_SIZE)
    uint32_t large_block_size;
    // Small block size (PAGE_SIZE)
    uint32_t small_block_size;
    // Total number of the large blocks
    uint32_t large_blocks_max_number;
    // Total number of the small blocks
    uint32_t small_blocks_max_number;

    // Total number of free for use doubly-linked list nodes
    uint32_t free_dll_nodes_max_number;
    // Array of nodes that are free to use. These nodes are used in doubly-linked lists of free blocks.
    block_node_t* free_dll_nodes_memory;
    // Size of this array
    uint32_t free_dll_nodes_memory_size;
    // Stack storing pointers to nodes of a doubly-linked list that are free for use (free_dll_nodes). These nodes are used in doubly-linked lists of free blocks.
    stack_t free_dll_nodes_ptrs_stack;
    // Memory for pointers stack
    block_node_t** free_dll_nodes_ptrs_stack_memory;
    // Size of this memory (stack)
    uint32_t free_dll_nodes_ptrs_stack_memory_size;

    // Array of allocations orders.
    // To free memory by address, we need to know the block size, this array contains the allocation orders, the allocation address is the offset in this array.
    uint8_t* allocations_orders_memory;
    // Size of this array
    uint32_t allocations_orders_memory_size;

    /*
     * An array of free lists, each element of which represents a pointer to the beginning of a separate doubly-linked list for each order.
     * Example:
     * free_blocks_lists[0] is a list of free blocks of size 2^0 * PAGE_SIZE
     * free_blocks_lists[1] is a list of free blocks of size 2^1 * PAGE_SIZE
     * up to
     * free_blocks_lists[MAX_ORDER] is a list of free blocks of size 2^MAX_ORDER * PAGE_SIZE
     */
    doubly_linked_list_t free_blocks_lists[BUDDY_ALLOCATOR_MAX_ORDER + 1];
} buddy_allocator_t;

/*
 * Pre-initializes the allocator, sets internal variables and calculates the size of memory needed.
 * After this function buddy_allocator_init function should be called, which completes initialization with allocated memory for allocator.
 * 
 * allocator_ptr pointer to allocator data
 * area_start_addr must be page aligned
 * area_size size of the memory area must to be larger than 2^max_order * page_size.
 * required_memory_size_ptr total size of the memory required by the allocator WILL BE PLACED BY THIS FUNCTION in this variable, it is necessary to allocate at least.
 * If it contains 0 after the function call, then the initialization has failed.
 *
 * About required_memory_size_ptr:
 * In order for the allocator to work, it is necessary to allocate memory for its needs.
 * The amount of this memory depends on the size of area (area_size) that the allocator will manage.
 */
extern void buddy_allocator_preinit(buddy_allocator_t* allocator_ptr, uint32_t area_start_addr, uint32_t area_size, uint32_t* required_memory_size_ptr);

/*
 * Finishes initialization by initializing the memory required to work the allocator.
 * allocator_ptr pointer to allocator data
 * required_memory_ptr pointer to the memory allocated for the needs of the allocator.
 * The size of this memory must be not less than the size specified in required_total_size_ptr after calling buddy_allocator_preinit function.
 */
extern void buddy_allocator_init(buddy_allocator_t* allocator_ptr, void* required_memory_ptr);

#endif
