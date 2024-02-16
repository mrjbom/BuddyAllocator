#include "buddy_allocator.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Get the memory block index by node pointer
 */
static uint32_t get_index_by_node(buddy_allocator_t* allocator_ptr, block_node_t* block_node_ptr)
{
    return (((uintptr_t)block_node_ptr - (uintptr_t)allocator_ptr->blocks_nodes)) / sizeof(block_node_t);
}

/*
 * Get the node pointer by memory block index
 */
static block_node_t* get_node_by_index(buddy_allocator_t* allocator_ptr, uint32_t blocks_index)
{
    return (block_node_t*)(blocks_index * sizeof(block_node_t) + (uintptr_t)allocator_ptr->blocks_nodes);
}

/*
 * Get order of block by index
 * Example:
 * 2 |     0     |     1     | 0 1
 * 1 |  2  |  3  |  4  |  5  | 2 5
 * 0 |6 |7 |8 |9 |10|11|12|13| 6 13
 * 1 - 2, 4 - 1, 8 - 0
 */
static uint8_t get_order_by_index(buddy_allocator_t* allocator_ptr, uint32_t block_index)
{
    uint8_t current_order = allocator_ptr->max_order;
    uint32_t current_lower_index = 0;
    uint32_t current_higher_index = allocator_ptr->large_blocks_number - 1;
    uint32_t current_index_step = allocator_ptr->large_blocks_number;
    while (block_index < current_lower_index || block_index > current_higher_index) {
        current_order--;
        current_lower_index = current_higher_index + 1;
        current_higher_index += current_index_step * 2;
        current_index_step *= 2;
    }
    return current_order;
}

/*
 * Get block size by order
 */
static uint32_t get_size_by_order(buddy_allocator_t* allocator_ptr, uint8_t order)
{
    return (1 << order) * allocator_ptr->page_size;
}

/*
 * Get blocks number by order
 */
static uint32_t get_blocks_number_by_order(buddy_allocator_t* allocator_ptr, uint8_t order)
{
    // Number of blocks changes exponentially, let's find member of geometric progression
    return allocator_ptr->large_blocks_number * (1 << (allocator_ptr->max_order - order));
}

/*
 * Get blocks first (left) child
 * 2 |     0     |
 * 1 |  1  |  2  |
 * 0 |3 |4 |5 |6 |
 * 0 - 1, 1 - 3, 2 - 5
 * 2 |     0     |     1     |
 * 1 |  2  |  3  |  4  |  5  |
 * 0 |6 |7 |8 |9 |10|11|12|13|
 * 0 - 2, 4 - 10, 5 - 12
 * 2 |     0     |     1     |     2     |
 * 1 |  3  |  4  |  5  |  6  |  7  |  8  |
 * 0 |9 |10|11|12|13|14|15|16|17|18|19|20|
 * 0 - 3, 3 - 9, 2 - 7, 8 - 19
 */
static uint32_t get_first_child_by_index(buddy_allocator_t* allocator_ptr, uint32_t block_index)
{
    return (block_index * 2) + allocator_ptr->large_blocks_number;
}

/*
 * Get blocks second (right) child
 * 2 |     0     |
 * 1 |  1  |  2  |
 * 0 |3 |4 |5 |6 |
 * 0 - 2, 1 - 4, 2 - 6
 * 2 |     0     |     1     |
 * 1 |  2  |  3  |  4  |  5  |
 * 0 |6 |7 |8 |9 |10|11|12|13|
 * 0 - 3, 4 - 11, 5 - 13
 * 2 |     0     |     1     |     2     |
 * 1 |  3  |  4  |  5  |  6  |  7  |  8  |
 * 0 |9 |10|11|12|13|14|15|16|17|18|19|20|
 * 0 - 4, 3 - 10, 2 - 8, 8 - 20
 */
static uint32_t get_second_child_by_index(buddy_allocator_t* allocator_ptr, uint32_t block_index)
{
    return ((block_index * 2) + allocator_ptr->large_blocks_number) + 1;
}

/*
 * Get index in order by global index
 */
static uint32_t get_index_in_order_by_index(buddy_allocator_t* allocator_ptr, uint32_t block_index)
{
    uint8_t order = get_order_by_index(allocator_ptr, block_index);
    uint32_t blocks_number_in_previous_order = allocator_ptr->large_blocks_number * ((1 << (allocator_ptr->max_order - order)) - 1);
    return block_index - blocks_number_in_previous_order;
}

void buddy_allocator_preinit(buddy_allocator_t* allocator_ptr, uintptr_t area_start_addr, size_t area_size, uint8_t max_order, uint32_t page_size, size_t* required_memory_size_ptr)
{
    if (page_size == 0) {
        return;
    }
    if (allocator_ptr == NULL || area_start_addr == 0 || area_start_addr % page_size != 0 || area_size == 0 || required_memory_size_ptr == NULL) {
        return;
    }
    allocator_ptr->large_block_size = (1 << max_order) * page_size;
    allocator_ptr->small_block_size = page_size;
    if (area_size < allocator_ptr->large_block_size) {
        // The memory area is less than one largest block, I don't want to work with it
        return;
    }

    allocator_ptr->large_blocks_number = area_size / allocator_ptr->large_block_size;
    allocator_ptr->small_blocks_number = allocator_ptr->large_blocks_number * (1 << max_order);
    allocator_ptr->total_blocks_number = allocator_ptr->large_blocks_number * ((1 << (max_order + 1)) - 1);

    allocator_ptr->area_start_addr = area_start_addr;
    allocator_ptr->area_size = allocator_ptr->large_blocks_number * allocator_ptr->large_block_size;
    allocator_ptr->max_order = max_order;
    allocator_ptr->page_size = page_size;

    // For blocks nodes
    allocator_ptr->blocks_nodes_memory_size = allocator_ptr->total_blocks_number * sizeof(block_node_t);
    // For free blocks lists
    allocator_ptr->free_blocks_lists_memory_size = (allocator_ptr->max_order + 1) * sizeof(doubly_linked_list_t);
    // For allocations orders array
    allocator_ptr->allocations_orders_memory_size = allocator_ptr->small_blocks_number * sizeof(uint8_t);

    /*
    // Debug
    if (area_size == 4194304) {
        FILE* file = fopen("debug_output.txt", "wb");
        for (uint32_t i = 0; i < allocator_ptr->total_blocks_number; ++i) {
            static uint32_t prev_order = BUDDY_ALLOCATOR_MAX_ORDER;
            uint32_t order = get_order_by_index(allocator_ptr, i);
            if (order != prev_order) {
                prev_order = order;
                fprintf(file, "\n");
            }
            fprintf(file, "%u ", order);
        }
        fclose(file);
    }
    */

    // Calculate required memory
    // [blocks_nodes free_blocks_lists allocations_orders]
    *required_memory_size_ptr = allocator_ptr->blocks_nodes_memory_size + allocator_ptr->free_blocks_lists_memory_size + allocator_ptr->allocations_orders_memory_size;
}

void buddy_allocator_init(buddy_allocator_t* allocator_ptr, void* required_memory_ptr)
{
    if (allocator_ptr == NULL || required_memory_ptr == NULL) {
        return;
    }

    // Setting up required memory
    // required_memory_ptr = [blocks_nodes free_blocks_lists allocations_orders]
    // Blocks nodes
    allocator_ptr->blocks_nodes = required_memory_ptr;
    // Free blocks lists
    allocator_ptr->free_blocks_lists = (doubly_linked_list_t*)((uintptr_t)allocator_ptr->blocks_nodes + allocator_ptr->blocks_nodes_memory_size);
    // Allocations orders array
    allocator_ptr->allocations_orders = (uint8_t*)((uintptr_t)allocator_ptr->free_blocks_lists + allocator_ptr->free_blocks_lists_memory_size);
    
    memset(allocator_ptr->blocks_nodes, 0, allocator_ptr->blocks_nodes_memory_size);
    memset(allocator_ptr->free_blocks_lists, 0, allocator_ptr->free_blocks_lists_memory_size);
    memset(allocator_ptr->allocations_orders, 0, allocator_ptr->allocations_orders_memory_size);

    // Right now all of our large blocks are free, let's put them on the free list
    for (uint32_t index = 0; index < allocator_ptr->large_blocks_number; ++index) {
        block_node_t* block_node = get_node_by_index(allocator_ptr, index);
        dll_insert_node_to_tail(&allocator_ptr->free_blocks_lists[allocator_ptr->max_order], (dll_node_t*)block_node);
    }
}

void* buddy_allocator_alloc(buddy_allocator_t* allocator_ptr, size_t size)
{
    if (allocator_ptr == NULL || size == 0 || size > allocator_ptr->large_block_size) {
        return NULL;
    }

    // We cannot allocate memory less than 1 page (smallest block)
    uint32_t current_size = allocator_ptr->page_size;
    uint8_t required_order = 0;
    // We trying to allocate memory larger than a small block
    if (size > allocator_ptr->page_size) {
        while (current_size < size) {
            current_size *= 2;
            required_order++;
        }
    }

    // Trying to find a free block of required size
    find_and_allocate_block:
    if (allocator_ptr->free_blocks_lists[required_order].count > 0) {
        // We found a free block of the requested size, we take it and remove it from the free list
        // Take first free block node
        block_node_t* free_block_node_ptr = (block_node_t*)allocator_ptr->free_blocks_lists[required_order].head;
        uint32_t free_block_index = get_index_by_node(allocator_ptr, free_block_node_ptr);
        uint32_t free_block_size = get_size_by_order(allocator_ptr, required_order);

        // Calculate memory block addr
        uintptr_t memory_block_addr = get_index_in_order_by_index(allocator_ptr, free_block_index) * free_block_size;

        // Remove block from free list
        dll_remove_node(&allocator_ptr->free_blocks_lists[required_order], (dll_node_t*)free_block_node_ptr);
        ((dll_node_t*)free_block_node_ptr)->next = NULL;
        ((dll_node_t*)free_block_node_ptr)->prev = NULL;

        // Save allocation order
        allocator_ptr->allocations_orders[memory_block_addr / allocator_ptr->small_block_size] = (uint8_t)required_order;

        // Return calculated memory block addr
        return (void*)(memory_block_addr + allocator_ptr->area_start_addr);
    }
    else {
        //printf("required %u\n", required_order);
        // Failed to find a free block of the requested size, which means we need to recursively divide larger blocks until a free block of the requested size is created
        // We're trying to allocate the largest block, but there isn't one
        if (required_order == allocator_ptr->max_order) {
            return NULL;
        }
        // Try to find free larger block
        uint8_t current_order = required_order + 1;
        while (allocator_ptr->free_blocks_lists[current_order].count == 0) {
            if (current_order == allocator_ptr->max_order) {
                return NULL;
            }
            current_order++;
        }
        // We found free larger block
        // Split it
        while (current_order > required_order) {
            //printf("split order %u\n", current_order);

            block_node_t* split_block_node_ptr = (block_node_t*)allocator_ptr->free_blocks_lists[current_order].head;
            uint32_t split_block_index = get_index_by_node(allocator_ptr, split_block_node_ptr);
            uint32_t split_block_first_child_index = get_first_child_by_index(allocator_ptr, split_block_index);
            uint32_t split_block_second_child_index = get_second_child_by_index(allocator_ptr, split_block_index);
            //printf("index:%u f_c:%u s_c:%u\n", split_block_index, split_block_first_child_index, split_block_second_child_index);

            // Split current block
            // Put childs to the free list
            block_node_t* split_block_first_child_node_ptr = get_node_by_index(allocator_ptr, split_block_first_child_index);
            block_node_t* split_block_second_child_node_ptr = get_node_by_index(allocator_ptr, split_block_second_child_index);
            dll_insert_node_to_head(&allocator_ptr->free_blocks_lists[current_order - 1], (dll_node_t*)split_block_first_child_node_ptr);
            dll_insert_node_after_node(&allocator_ptr->free_blocks_lists[current_order - 1], (dll_node_t*)split_block_first_child_node_ptr, (dll_node_t*)split_block_second_child_node_ptr);

            // Remove splitted block from the free list
            dll_remove_node(&allocator_ptr->free_blocks_lists[current_order], (dll_node_t*)split_block_node_ptr);
            ((dll_node_t*)split_block_node_ptr)->next = NULL;
            ((dll_node_t*)split_block_node_ptr)->prev = NULL;

            current_order--;
        }

        // Now we have a block of the requested size/order
        goto find_and_allocate_block;
    }
}
