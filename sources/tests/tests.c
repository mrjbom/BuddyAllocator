#include "tests.h"
#include "../buddy_allocator/buddy_allocator.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../dllist/dllist.h"

void tests_preinit()
{
    // 0, 1 MB, 4 MB, 8 MB, 12 MB, 1 GB
    uint32_t fake_area_start_addr = 0x1000;
    const uint32_t area_sizes[] = { 0, 1048576, 4194304, 8388608, 12582912, 1073741824 };
    const uint8_t max_order = 10;
    // MAX_ORDER = 10
    // PAGE_SIZE = 4096
    // area_size - large_blocks_number small_blocks_number total_blocks_number
    // 0 MB - 0 0 0
    // 1 MB - 0 0 0
    // 4 MB - 1 1024 2047
    // 8 MB - 2 2048 4094
    // 12 MB - 3 3072 6141
    // 1 GB - 256 262144 524032
    const uint32_t large_blocks_number_control[] = { 0, 0, 1, 2, 3, 256 };
    const uint32_t small_blocks_number_control[] = { 0, 0, 1024, 2048, 3072, 262144 };
    const uint32_t total_blocks_number_control[] = { 0, 0, 2047, 4094, 6141, 524032 };

    // [blocks_nodes free_blocks_lists allocations_orders]
    const uint32_t required_memory_size_control[] = { total_blocks_number_control[0] * sizeof(block_node_t) + 0 + small_blocks_number_control[0] * sizeof(uint8_t),
                                                      total_blocks_number_control[1] * sizeof(block_node_t) + 0 + small_blocks_number_control[1] * sizeof(uint8_t),
                                                      total_blocks_number_control[2] * sizeof(block_node_t) + (max_order + 1) * sizeof(doubly_linked_list_t) + small_blocks_number_control[2] * sizeof(uint8_t),
                                                      total_blocks_number_control[3] * sizeof(block_node_t) + (max_order + 1) * sizeof(doubly_linked_list_t) + small_blocks_number_control[3] * sizeof(uint8_t),
                                                      total_blocks_number_control[4] * sizeof(block_node_t) + (max_order + 1) * sizeof(doubly_linked_list_t) + small_blocks_number_control[4] * sizeof(uint8_t),
                                                      total_blocks_number_control[5] * sizeof(block_node_t) + (max_order + 1) * sizeof(doubly_linked_list_t) + small_blocks_number_control[5] * sizeof(uint8_t) };
    
    for (uint32_t i = 0; i < sizeof(area_sizes) / sizeof(uint32_t); ++i) {
        buddy_allocator_t allocator;
        uint32_t required_memory_size = 0;
        memset(&allocator, 0, sizeof(buddy_allocator_t));
        buddy_allocator_preinit(&allocator, fake_area_start_addr, area_sizes[i], max_order, 4096, &required_memory_size);
        assert(allocator.large_blocks_number == large_blocks_number_control[i]);
        assert(allocator.small_blocks_number == small_blocks_number_control[i]);
        assert(allocator.total_blocks_number == total_blocks_number_control[i]);
        assert(required_memory_size == required_memory_size_control[i]);
    }
}

void tests_small_sizes()
{
    buddy_allocator_t allocator;
    uint32_t fake_area_start_addr = 0x1000;
    uint8_t max_order = 2;

    // 48 bytes
    // MAX_ORDER = 2
    // PAGE_SIZE = 4
    // 3 large blocks
    // 12 small blocks
    // 21 total blocks
    // 2 |     0     |     1     |     2     | 16 bytes per blocks
    // 1 |  3  |  4  |  5  |  6  |  7  |  8  | 8 bytes per blocks
    // 0 |9 |10|11|12|13|14|15|16|17|18|19|20| 4 bytes per blocks

    // Test init

    const uint32_t large_blocks_number_control = 3;
    const uint32_t small_blocks_number_control = 12;
    const uint32_t total_blocks_number_control = 21;

    // [blocks_nodes free_blocks_lists allocations_orders]
    const uint32_t required_memory_size_control = total_blocks_number_control * sizeof(block_node_t) + (max_order + 1) * sizeof(doubly_linked_list_t) + small_blocks_number_control * sizeof(uint8_t);

    uint32_t required_memory_size = 0;
    memset(&allocator, 0, sizeof(buddy_allocator_t));
    buddy_allocator_preinit(&allocator, fake_area_start_addr, 48, max_order, 4, &required_memory_size);

    assert(allocator.large_blocks_number == large_blocks_number_control);
    assert(allocator.small_blocks_number == small_blocks_number_control);
    assert(allocator.total_blocks_number == total_blocks_number_control);
    assert(required_memory_size == required_memory_size_control);

    void* required_memory = malloc(required_memory_size);
    assert(required_memory != NULL);
    buddy_allocator_init(&allocator, required_memory);

    // Test allocation
    void* allocated_addr = buddy_allocator_alloc(&allocator, 16); // block 0
    assert((uintptr_t)allocated_addr == 0 * 16 + fake_area_start_addr);

    allocated_addr = buddy_allocator_alloc(&allocator, 8);
    assert((uintptr_t)allocated_addr == 2 * 8 + fake_area_start_addr); // block 5

    allocated_addr = buddy_allocator_alloc(&allocator, 4);
    assert((uintptr_t)allocated_addr == 6 * 4 + fake_area_start_addr); // block 15

    allocated_addr = buddy_allocator_alloc(&allocator, 4);
    assert((uintptr_t)allocated_addr == 7 * 4 + fake_area_start_addr); // block 16

    allocated_addr = buddy_allocator_alloc(&allocator, 4);
    assert((uintptr_t)allocated_addr == 8 * 4 + fake_area_start_addr); // block 17

    allocated_addr = buddy_allocator_alloc(&allocator, 8);
    assert((uintptr_t)allocated_addr == 5 * 8 + fake_area_start_addr); // block 8

    free(required_memory);
}
