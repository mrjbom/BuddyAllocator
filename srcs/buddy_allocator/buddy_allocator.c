#include "buddy_allocator.h"
#include <string.h>

void buddy_allocator_preinit(buddy_allocator_t* allocator_ptr, uintptr_t area_start_addr, size_t area_size, size_t* required_memory_size_ptr)
{
    if (allocator_ptr == NULL || area_start_addr % BUDDY_ALLOCATOR_PAGE_SIZE != 0 || area_size == 0 || required_memory_size_ptr == NULL) {
        return;
    }
    allocator_ptr->large_block_size = (1 << BUDDY_ALLOCATOR_MAX_ORDER) * BUDDY_ALLOCATOR_PAGE_SIZE;
    allocator_ptr->small_block_size = BUDDY_ALLOCATOR_PAGE_SIZE;
    if (area_size < allocator_ptr->large_block_size) {
        // The memory area is less than one largest block, I don't want to work with it.
        return;
    }

    allocator_ptr->large_blocks_max_number = area_size / allocator_ptr->large_block_size;
    allocator_ptr->small_blocks_max_number = allocator_ptr->large_blocks_max_number * (1 << BUDDY_ALLOCATOR_MAX_ORDER);
    //allocator_ptr->total_blocks_number = allocator_ptr->large_blocks_number * ((1 << (BUDDY_ALLOCATOR_MAX_ORDER + 1)) - 1);
    allocator_ptr->free_dll_nodes_max_number = allocator_ptr->small_blocks_max_number / 2;

    allocator_ptr->area_start_addr = area_start_addr;
    allocator_ptr->area_size = allocator_ptr->large_blocks_max_number * allocator_ptr->large_block_size;

    // For free for use doubly-linked list nodes
    allocator_ptr->free_dll_nodes_memory_size = allocator_ptr->free_dll_nodes_max_number * sizeof(block_node_t);
    // For free for use doubly-linked list nodes pointers stack
    allocator_ptr->free_dll_nodes_ptrs_stack_memory_size = allocator_ptr->free_dll_nodes_max_number * sizeof(block_node_t*);
    // For allocations orders array
    allocator_ptr->allocations_orders_memory_size = allocator_ptr->small_blocks_max_number * sizeof(uint8_t);

    memset(allocator_ptr->free_blocks_lists, 0, sizeof(allocator_ptr->free_blocks_lists));

    // Calculate required memory
    // [free_dll_nodes_memory free_dll_nodes_ptrs_stack_memory allocations_orders_memory]
    *required_memory_size_ptr = allocator_ptr->free_dll_nodes_memory_size + allocator_ptr->free_dll_nodes_ptrs_stack_memory_size + allocator_ptr->allocations_orders_memory_size;
}

void buddy_allocator_init(buddy_allocator_t* allocator_ptr, void* required_memory_ptr)
{
    if (allocator_ptr == NULL || required_memory_ptr == NULL) {
        return;
    }

    // Setting up required memory
    // required_memory_ptr = [free_dll_nodes_memory free_dll_nodes_ptrs_stack_memory allocations_orders_memory]
    // Free for use doubly-linked list nodes
    allocator_ptr->free_dll_nodes_memory = required_memory_ptr;
    // Free for use doubly-linked list nodes pointers stack
    allocator_ptr->free_dll_nodes_ptrs_stack_memory = (block_node_t**)((uintptr_t)allocator_ptr->free_dll_nodes_memory + allocator_ptr->free_dll_nodes_memory_size);
    // Allocations orders array
    allocator_ptr->allocations_orders_memory = (uint8_t*)((uintptr_t)allocator_ptr->free_dll_nodes_ptrs_stack_memory + allocator_ptr->free_dll_nodes_ptrs_stack_memory_size);
    memset(allocator_ptr->free_dll_nodes_memory, 0, allocator_ptr->free_dll_nodes_memory_size);
    memset(allocator_ptr->free_dll_nodes_ptrs_stack_memory, 0, allocator_ptr->free_dll_nodes_ptrs_stack_memory_size);
    memset(allocator_ptr->allocations_orders_memory, 0, allocator_ptr->allocations_orders_memory_size);
    // Setting up free nodes ptrs stack
    stack_init(&allocator_ptr->free_dll_nodes_ptrs_stack, allocator_ptr->free_dll_nodes_ptrs_stack_memory, allocator_ptr->free_dll_nodes_ptrs_stack_memory_size);
    // Add to the stack all addresses of nodes available for use
    for (uint32_t i = 0; i < allocator_ptr->free_dll_nodes_max_number; ++i) {
        uintptr_t free_dll_node_addr = (uintptr_t)allocator_ptr->free_dll_nodes_memory + (i * sizeof(block_node_t));
        stack_push(&allocator_ptr->free_dll_nodes_ptrs_stack, &free_dll_node_addr, sizeof(uintptr_t));
    }

    // Init free blocks lists
    for (uint32_t i = 0; i < allocator_ptr->large_blocks_max_number; ++i) {
        // Right now all of our large free blocks are free, let's add them to the free list.
        // Get free for use node
        block_node_t* block_node_ptr = NULL;
        stack_pop(&allocator_ptr->free_dll_nodes_ptrs_stack, &block_node_ptr, sizeof(uintptr_t));
        memset(block_node_ptr, 0, sizeof(block_node_t));
        dll_insert_node_to_tail(&allocator_ptr->free_blocks_lists[BUDDY_ALLOCATOR_MAX_ORDER], (dll_node_t*)block_node_ptr);
    }
}
