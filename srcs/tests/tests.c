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
    const uint32_t area_sizes[] = { 0, 1048576, 4194304, 8388608, 12582912, 1073741824 };
    // MAX_ORDER = 10
    // PAGE_SIZE = 4096
    // area_size - large_blocks_number small_blocks_number
    // 0 MB - 0 0
    // 1 MB - 0 0
    // 4 MB - 1 1024
    // 8 MB - 2 2048
    // 12 MB - 3 3072
    // 1 GB - 256 262144
    const uint32_t large_blocks_max_number_control[] = { 0, 0, 1, 2, 3, 256 };
    const uint32_t small_blocks_max_number_control[] = { 0, 0, 1024, 2048, 3072, 262144 };
    const uint32_t free_dll_nodes_max_number_control[] = { small_blocks_max_number_control[0] / 2, small_blocks_max_number_control[1] / 2, small_blocks_max_number_control[2] / 2,
                                                           small_blocks_max_number_control[3] / 2, small_blocks_max_number_control[4] / 2, small_blocks_max_number_control[5] / 2 };
    // free_dll_nodes + free_dll_nodes_ptrs_stack + allocations_orders
    const uint32_t required_memory_size_control[] = { free_dll_nodes_max_number_control[0] * sizeof(block_node_t) + free_dll_nodes_max_number_control[0] * sizeof(block_node_t*) + small_blocks_max_number_control[0] * sizeof(uint8_t),
                                                      free_dll_nodes_max_number_control[1] * sizeof(block_node_t) + free_dll_nodes_max_number_control[1] * sizeof(block_node_t*) + small_blocks_max_number_control[1] * sizeof(uint8_t),
                                                      free_dll_nodes_max_number_control[2] * sizeof(block_node_t) + free_dll_nodes_max_number_control[2] * sizeof(block_node_t*) + small_blocks_max_number_control[2] * sizeof(uint8_t),
                                                      free_dll_nodes_max_number_control[3] * sizeof(block_node_t) + free_dll_nodes_max_number_control[3] * sizeof(block_node_t*) + small_blocks_max_number_control[3] * sizeof(uint8_t),
                                                      free_dll_nodes_max_number_control[4] * sizeof(block_node_t) + free_dll_nodes_max_number_control[4] * sizeof(block_node_t*) + small_blocks_max_number_control[4] * sizeof(uint8_t),
                                                      free_dll_nodes_max_number_control[5] * sizeof(block_node_t) + free_dll_nodes_max_number_control[5] * sizeof(block_node_t*) + small_blocks_max_number_control[5] * sizeof(uint8_t) };
    
    for (uint32_t i = 0; i < sizeof(area_sizes) / sizeof(uint32_t); ++i) {
        buddy_allocator_t allocator;
        uint32_t required_memory_size = 0;
        memset(&allocator, 0, sizeof(buddy_allocator_t));
        buddy_allocator_preinit(&allocator, 0, area_sizes[i], &required_memory_size);
        assert(allocator.large_blocks_max_number == large_blocks_max_number_control[i]);
        assert(allocator.small_blocks_max_number == small_blocks_max_number_control[i]);
        assert(allocator.free_dll_nodes_max_number == free_dll_nodes_max_number_control[i]);
        assert(required_memory_size == required_memory_size_control[i]);
    }
}

void tests_alloc_free_4MB()
{
    // 4 MB - 4194304
    // MAX_ORDER = 10
    // PAGE_SIZE = 4096
    // 1 large block
    // 1024 small block
    buddy_allocator_t allocator;
    uint32_t required_memory_size = 0;
    memset(&allocator, 0, sizeof(buddy_allocator_t));
    buddy_allocator_preinit(&allocator, 0, 4194304, &required_memory_size);
    void* required_memory = malloc(required_memory_size);
    assert(required_memory != NULL);
    buddy_allocator_init(&allocator, required_memory);
}
