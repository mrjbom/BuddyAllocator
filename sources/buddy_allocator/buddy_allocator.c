#include "buddy_allocator.h"
#include <string.h>
#include <stdio.h>

void buddy_allocator_preinit(buddy_allocator_t* allocator_ptr, uintptr_t area_start_addr, size_t area_size, size_t* required_memory_size_ptr)
{
    if (allocator_ptr == NULL || area_start_addr == 0 || area_start_addr % BUDDY_ALLOCATOR_PAGE_SIZE != 0 || area_size == 0 || required_memory_size_ptr == NULL) {
        return;
    }
    allocator_ptr->large_block_size = (1 << BUDDY_ALLOCATOR_MAX_ORDER) * BUDDY_ALLOCATOR_PAGE_SIZE;
    allocator_ptr->small_block_size = BUDDY_ALLOCATOR_PAGE_SIZE;
    if (area_size < allocator_ptr->large_block_size) {
        // The memory area is less than one largest block, I don't want to work with it.
        return;
    }

    allocator_ptr->large_blocks_number = area_size / allocator_ptr->large_block_size;
    allocator_ptr->small_blocks_number = allocator_ptr->large_blocks_number * (1 << BUDDY_ALLOCATOR_MAX_ORDER);
    allocator_ptr->total_blocks_number = allocator_ptr->large_blocks_number * ((1 << (BUDDY_ALLOCATOR_MAX_ORDER + 1)) - 1);

    allocator_ptr->area_start_addr = area_start_addr;
    allocator_ptr->area_size = allocator_ptr->large_blocks_number * allocator_ptr->large_block_size;

    // For blocks nodes
    allocator_ptr->blocks_nodes_memory_size = allocator_ptr->total_blocks_number * sizeof(block_node_t);
    // For allocations orders array
    allocator_ptr->allocations_orders_memory_size = allocator_ptr->small_blocks_number * sizeof(uint8_t);

    memset(allocator_ptr->free_blocks_lists, 0, sizeof(allocator_ptr->free_blocks_lists));

    // Calculate required memory
    // [blocks_nodes allocations_orders]
    *required_memory_size_ptr = allocator_ptr->blocks_nodes_memory_size + allocator_ptr->allocations_orders_memory_size;
}

void buddy_allocator_init(buddy_allocator_t* allocator_ptr, void* required_memory_ptr)
{
    if (allocator_ptr == NULL || required_memory_ptr == NULL) {
        return;
    }

    // Setting up required memory
    // required_memory_ptr = [blocks_nodes + allocations_orders]
    // Blocks nodes
    allocator_ptr->blocks_nodes = required_memory_ptr;
    // Allocations orders array
    allocator_ptr->allocations_orders = (uint8_t*)((uintptr_t)allocator_ptr->blocks_nodes + allocator_ptr->blocks_nodes_memory_size);
    memset(allocator_ptr->blocks_nodes, 0, allocator_ptr->blocks_nodes_memory_size);
    memset(allocator_ptr->allocations_orders, 0, allocator_ptr->allocations_orders_memory_size);
}

void* buddy_allocator_alloc(buddy_allocator_t* allocator_ptr, size_t size)
{
    if (allocator_ptr == NULL || size == 0 || size > allocator_ptr->large_block_size) {
        return NULL;
    }

    // We cannot allocate memory less than 1 page (smallest block)
    uint32_t current_size = BUDDY_ALLOCATOR_PAGE_SIZE;
    uint32_t current_order = 0;
    // We trying to allocate memory larger than a small block
    if (size > BUDDY_ALLOCATOR_PAGE_SIZE) {
        while (current_size < size) {
            current_size *= 2;
            current_order++;
        }
    }

    // Trying to find a free block of required size
    if (allocator_ptr->free_blocks_lists[current_order].count > 0) {
        // We found a free block of the requested size, we take it and remove it from the free list.
    }
    else {

    }

    return (void*)allocator_ptr->area_start_addr;
}
