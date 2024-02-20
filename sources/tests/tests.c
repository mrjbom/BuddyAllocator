#include "tests.h"
#include "../buddy_allocator/buddy_allocator.h"
#ifndef _DEBUG
#undef NDEBUG
#endif // !_DEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
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
    const uint32_t required_memory_size_control[] = { total_blocks_number_control[0] * sizeof(memory_block_node_t) + 0 + small_blocks_number_control[0] * sizeof(uint8_t),
                                                      total_blocks_number_control[1] * sizeof(memory_block_node_t) + 0 + small_blocks_number_control[1] * sizeof(uint8_t),
                                                      total_blocks_number_control[2] * sizeof(memory_block_node_t) + (max_order + 1) * sizeof(doubly_linked_list_t) + small_blocks_number_control[2] * sizeof(uint8_t),
                                                      total_blocks_number_control[3] * sizeof(memory_block_node_t) + (max_order + 1) * sizeof(doubly_linked_list_t) + small_blocks_number_control[3] * sizeof(uint8_t),
                                                      total_blocks_number_control[4] * sizeof(memory_block_node_t) + (max_order + 1) * sizeof(doubly_linked_list_t) + small_blocks_number_control[4] * sizeof(uint8_t),
                                                      total_blocks_number_control[5] * sizeof(memory_block_node_t) + (max_order + 1) * sizeof(doubly_linked_list_t) + small_blocks_number_control[5] * sizeof(uint8_t) };
    
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

void tests_small_sizes_predetermined()
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
    const uint32_t required_memory_size_control = total_blocks_number_control * sizeof(memory_block_node_t) + (max_order + 1) * sizeof(doubly_linked_list_t) + small_blocks_number_control * sizeof(uint8_t);

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
    void* allocated_addr = NULL;

    // 2 |     0     |     1     |     2     | 16 bytes per blocks
    // 1 |  3  |  4  |  5  |  6  |  7  |  8  | 8 bytes per blocks
    // 0 |9 |10|11|12|13|14|15|16|17|18|19|20| 4 bytes per blocks

    // Block 9
    allocated_addr = buddy_allocator_alloc(&allocator, 4);
    assert((uintptr_t)allocated_addr == 0 * 4 + fake_area_start_addr);

    // Block 4
    allocated_addr = buddy_allocator_alloc(&allocator, 8);
    assert((uintptr_t)allocated_addr == 1 * 8 + fake_area_start_addr);

    // Block 1
    allocated_addr = buddy_allocator_alloc(&allocator, 16);
    assert((uintptr_t)allocated_addr == 1 * 16 + fake_area_start_addr);

    // Block 10
    allocated_addr = buddy_allocator_alloc(&allocator, 4);
    assert((uintptr_t)allocated_addr == 1 * 4 + fake_area_start_addr);

    // Block 17
    allocated_addr = buddy_allocator_alloc(&allocator, 4);
    assert((uintptr_t)allocated_addr == 8 * 4 + fake_area_start_addr);

    // Block 18
    allocated_addr = buddy_allocator_alloc(&allocator, 4);
    assert((uintptr_t)allocated_addr == 9 * 4 + fake_area_start_addr);

    // Order 2 blocks not exist
    allocated_addr = buddy_allocator_alloc(&allocator, 16);
    assert((uintptr_t)allocated_addr == 0);

    // Block 8
    allocated_addr = buddy_allocator_alloc(&allocator, 8);
    assert((uintptr_t)allocated_addr == 5 * 8 + fake_area_start_addr);

    // Order 0 blocks not exist
    allocated_addr = buddy_allocator_alloc(&allocator, 4);
    assert((uintptr_t)allocated_addr == 0);

    // Order 1 blocks not exist
    allocated_addr = buddy_allocator_alloc(&allocator, 8);
    assert((uintptr_t)allocated_addr == 0);

    // 2 |      0 S     |       1 A     |       2 S     | 16 bytes per blocks
    // 1 |  3 S |   4 A |   5   |   6   |   7 S |   8 A | 8 bytes per blocks
    // 0 |9A|10A|11 |12 |13 |14 |15 |16 |17A|18A|19 |20 | 4 bytes per blocks
    // free_blocks_list[order].count
    // 0 0
    // 0 0
    // 0 0

    // Free all the blocks in random order
    // Block 1
    buddy_allocator_free(&allocator, (void*)(1 * 16 + fake_area_start_addr));
    // 2 |      0 S     |       1       |       2 S     | 16 bytes per blocks
    // 1 |  3 S |   4 A |   5   |   6   |   7 S |   8 A | 8 bytes per blocks
    // 0 |9A|10A|11 |12 |13 |14 |15 |16 |17A|18A|19 |20 | 4 bytes per blocks
    // free_blocks_list[order].count
    // 0 1
    // 0 0
    // 0 0
    
    // Block 18
    buddy_allocator_free(&allocator, (void*)(9 * 4 + fake_area_start_addr));
    // 2 |      0 S     |       1       |       2 S     | 16 bytes per blocks
    // 1 |  3 S |   4 A |   5   |   6   |   7 S |   8 A | 8 bytes per blocks
    // 0 |9A|10A|11 |12 |13 |14 |15 |16 |17A|18 |19 |20 | 4 bytes per blocks
    // free_blocks_list[order].count
    // 0 1
    // 0 0
    // 0 1
    
    // Block 8
    buddy_allocator_free(&allocator, (void*)(5 * 8 + fake_area_start_addr));
    // 2 |      0 S     |       1       |       2 S     | 16 bytes per blocks
    // 1 |  3 S |   4 A |   5   |   6   |   7 S |   8   | 8 bytes per blocks
    // 0 |9A|10A|11 |12 |13 |14 |15 |16 |17A|18 |19 |20 | 4 bytes per blocks
    // free_blocks_list[order].count
    // 0 1
    // 0 1
    // 0 1
    
    // Block 4
    buddy_allocator_free(&allocator, (void*)(1 * 8 + fake_area_start_addr));
    // 2 |      0 S     |       1       |       2 S     | 16 bytes per blocks
    // 1 |  3 S |   4   |   5   |   6   |   7 S |   8   | 8 bytes per blocks
    // 0 |9A|10A|11 |12 |13 |14 |15 |16 |17A|18 |19 |20 | 4 bytes per blocks
    // free_blocks_list[order].count
    // 0 1
    // 0 2
    // 0 1
    
    // Block 10
    buddy_allocator_free(&allocator, (void*)(1 * 4 + fake_area_start_addr));
    // 2 |      0 S     |       1       |       2 S     | 16 bytes per blocks
    // 1 |  3 S |   4   |   5   |   6   |   7 S |   8   | 8 bytes per blocks
    // 0 |9A|10 |11 |12 |13 |14 |15 |16 |17A|18 |19 |20 | 4 bytes per blocks
    // free_blocks_list[order].count
    // 0 1
    // 0 2
    // 0 2
    
    // Block 9
    buddy_allocator_free(&allocator, (void*)(0 * 4 + fake_area_start_addr));
    // 2 |      0       |       1       |       2 S     | 16 bytes per blocks
    // 1 |  3   |   4   |   5   |   6   |   7 S |   8   | 8 bytes per blocks
    // 0 |9 |10 |11 |12 |13 |14 |15 |16 |17A|18 |19 |20 | 4 bytes per blocks
    // free_blocks_list[order].count
    // 0 2
    // 0 1
    // 0 1
    
    // Block 17
    buddy_allocator_free(&allocator, (void*)(8 * 4 + fake_area_start_addr));

    for (uint32_t i = 0; i < max_order; ++i) {
        assert(allocator.free_blocks_lists[i].count == 0);
    }
    assert(allocator.free_blocks_lists[allocator.max_order].count == 3);

    // Allocate blocks again in random order
    // The order of large nodes in the list is different from the previous one, which means that the order of node allocations will be different (addresses changed)
    // 2<->0<->1
    // Allocator data
    // 2 |     2     |     0     |     1     |
    // 1 |  7  |  8  |  3  |  4  |  5  |  6  |
    // 0 |17|18|19|20|9 |10|11|12|13|14|15|16|
    // Memory
    // 2 |     0     |     1     |     2     | 16 bytes per blocks
    // 1 |  3  |  4  |  5  |  6  |  7  |  8  | 8 bytes per blocks
    // 0 |9 |10|11|12|13|14|15|16|17|18|19|20| 4 bytes per blocks

    // Sizes
    // 4 16 8 4 4 16 8 15 8 4 4
    // Memory
    // 9 1 4 10 17 o2(fail) 8 o2(fail) o1(fail) 18 o0(fail)
    // Allocator data
    // 17 0 8 18 13 o2(fail) 6 o2(fail) o1(fail) 14 o0(fail)

    // Block 17
    allocated_addr = buddy_allocator_alloc(&allocator, 4);
    assert((uintptr_t)allocated_addr == 8 * 4 + fake_area_start_addr);
    // Block 0
    allocated_addr = buddy_allocator_alloc(&allocator, 16);
    assert((uintptr_t)allocated_addr == 0 * 16 + fake_area_start_addr);
    // Block 8
    allocated_addr = buddy_allocator_alloc(&allocator, 8);
    assert((uintptr_t)allocated_addr == 5 * 8 + fake_area_start_addr);
    // Block 18
    allocated_addr = buddy_allocator_alloc(&allocator, 4);
    assert((uintptr_t)allocated_addr == 9 * 4 + fake_area_start_addr);
    // Block 13
    allocated_addr = buddy_allocator_alloc(&allocator, 4);
    assert((uintptr_t)allocated_addr == 4 * 4 + fake_area_start_addr);
    // Order 2 fail
    allocated_addr = buddy_allocator_alloc(&allocator, 16);
    assert((uintptr_t)allocated_addr == 0);
    // Block 6
    allocated_addr = buddy_allocator_alloc(&allocator, 8);
    assert((uintptr_t)allocated_addr == 3 * 8 + fake_area_start_addr);
    // Order 2 fail
    allocated_addr = buddy_allocator_alloc(&allocator, 16);
    assert((uintptr_t)allocated_addr == 0);
    // Order 1 fail
    allocated_addr = buddy_allocator_alloc(&allocator, 8);
    assert((uintptr_t)allocated_addr == 0);
    // Block 14
    allocated_addr = buddy_allocator_alloc(&allocator, 4);
    assert((uintptr_t)allocated_addr == 5 * 4 + fake_area_start_addr);
    // Order 0 fail
    allocated_addr = buddy_allocator_alloc(&allocator, 4);
    assert((uintptr_t)allocated_addr == 0);

    
    // Free all blocks in random order
    // 18 13 8 14 17 0 6
    // Block 18
    buddy_allocator_free(&allocator, (void*)(9 * 4 + fake_area_start_addr));
    // Block 13
    buddy_allocator_free(&allocator, (void*)(4 * 4 + fake_area_start_addr));
    // Block 8
    buddy_allocator_free(&allocator, (void*)(5 * 8 + fake_area_start_addr));
    // Block 14
    buddy_allocator_free(&allocator, (void*)(5 * 4 + fake_area_start_addr));
    // Block 17
    buddy_allocator_free(&allocator, (void*)(8 * 4 + fake_area_start_addr));
    // Block 0
    buddy_allocator_free(&allocator, (void*)(0 * 16 + fake_area_start_addr));
    // Block 6
    buddy_allocator_free(&allocator, (void*)(3 * 8 + fake_area_start_addr));

    for (uint32_t i = 0; i < max_order; ++i) {
        assert(allocator.free_blocks_lists[i].count == 0);
    }
    assert(allocator.free_blocks_lists[allocator.max_order].count == 3);

    free(required_memory);
}

void tests_small_sizes_predetermined2(void)
{
    buddy_allocator_t allocator;
    uint32_t fake_area_start_addr = 0x1000;
    uint8_t max_order = 2;
    uint32_t required_memory_size = 0;
    memset(&allocator, 0, sizeof(buddy_allocator_t));
    buddy_allocator_preinit(&allocator, fake_area_start_addr, 96, max_order, 8, &required_memory_size);
    void* required_memory = malloc(required_memory_size);
    assert(required_memory != NULL);
    buddy_allocator_init(&allocator, required_memory);

    // 2 |     0     |     1     |     2     | 32 bytes per blocks
    // 1 |  3  |  4  |  5  |  6  |  7  |  8  | 16 bytes per blocks
    // 0 |9 |10|11|12|13|14|15|16|17|18|19|20| 8 bytes per blocks

    for (uint32_t i = 0; i < 3; ++i) {
        // Try to allocate 6x 16 bytes blocks
        void* allocated_addr = NULL;
        // Block 3
        allocated_addr = buddy_allocator_alloc(&allocator, 16);
        assert((uintptr_t)allocated_addr == 0 * 16 + fake_area_start_addr);
        // Block 4
        allocated_addr = buddy_allocator_alloc(&allocator, 16);
        assert((uintptr_t)allocated_addr == 1 * 16 + fake_area_start_addr);
        // Block 5
        allocated_addr = buddy_allocator_alloc(&allocator, 16);
        assert((uintptr_t)allocated_addr == 2 * 16 + fake_area_start_addr);
        // Block 6
        allocated_addr = buddy_allocator_alloc(&allocator, 16);
        assert((uintptr_t)allocated_addr == 3 * 16 + fake_area_start_addr);
        // Block 7
        allocated_addr = buddy_allocator_alloc(&allocator, 16);
        assert((uintptr_t)allocated_addr == 4 * 16 + fake_area_start_addr);
        // Block 8
        allocated_addr = buddy_allocator_alloc(&allocator, 16);
        assert((uintptr_t)allocated_addr == 5 * 16 + fake_area_start_addr);
        // NULL
        allocated_addr = buddy_allocator_alloc(&allocator, 16);
        assert(allocated_addr == NULL);
        // NULL
        allocated_addr = buddy_allocator_alloc(&allocator, 16);
        assert(allocated_addr == NULL);
        // NULL
        allocated_addr = buddy_allocator_alloc(&allocator, 8);
        assert(allocated_addr == NULL);
        // NULL
        allocated_addr = buddy_allocator_alloc(&allocator, 32);
        assert(allocated_addr == NULL);

        assert(allocator.free_blocks_lists[2].count == 0);
        assert(allocator.free_blocks_lists[1].count == 0);
        assert(allocator.free_blocks_lists[0].count == 0);

        // Free all alocated blocks
        // Block 8
        buddy_allocator_free(&allocator, (void*)(5 * 16 + fake_area_start_addr));
        // Block 7
        buddy_allocator_free(&allocator, (void*)(4 * 16 + fake_area_start_addr));
        // Block 6
        buddy_allocator_free(&allocator, (void*)(3 * 16 + fake_area_start_addr));
        // Block 5
        buddy_allocator_free(&allocator, (void*)(2 * 16 + fake_area_start_addr));
        // Block 4
        buddy_allocator_free(&allocator, (void*)(1 * 16 + fake_area_start_addr));
        // Block 3
        buddy_allocator_free(&allocator, (void*)(0 * 16 + fake_area_start_addr));

        assert(allocator.free_blocks_lists[2].count == 3);
        assert(allocator.free_blocks_lists[1].count == 0);
        assert(allocator.free_blocks_lists[0].count == 0);
    }

    for (uint32_t i = 0; i < 3; ++i) {
        // Try to allocate 3x 32 bytes blocks
        void* allocated_addr = NULL;
        // Block 0
        allocated_addr = buddy_allocator_alloc(&allocator, 32);
        assert((uintptr_t)allocated_addr == 0 * 32 + fake_area_start_addr);
        // Block 1
        allocated_addr = buddy_allocator_alloc(&allocator, 32);
        assert((uintptr_t)allocated_addr == 1 * 32 + fake_area_start_addr);
        // Block 2
        allocated_addr = buddy_allocator_alloc(&allocator, 32);
        assert((uintptr_t)allocated_addr == 2 * 32 + fake_area_start_addr);
        // NULL
        allocated_addr = buddy_allocator_alloc(&allocator, 32);
        assert(allocated_addr == NULL);
        // NULL
        allocated_addr = buddy_allocator_alloc(&allocator, 16);
        assert(allocated_addr == NULL);
        // NULL
        allocated_addr = buddy_allocator_alloc(&allocator, 8);
        assert(allocated_addr == NULL);
        // NULL
        allocated_addr = buddy_allocator_alloc(&allocator, 32);
        assert(allocated_addr == NULL);

        assert(allocator.free_blocks_lists[2].count == 0);
        assert(allocator.free_blocks_lists[1].count == 0);
        assert(allocator.free_blocks_lists[0].count == 0);

        // Free all alocated blocks
        // Wrong, out of memory area, block 3 not exist
        // Block 3
        buddy_allocator_free(&allocator, (void*)(3 * 32 + fake_area_start_addr));
        // Block 2
        buddy_allocator_free(&allocator, (void*)(2 * 32 + fake_area_start_addr));
        // Block 1
        buddy_allocator_free(&allocator, (void*)(1 * 32 + fake_area_start_addr));
        // Wrong, double allocation
        // Block 1
        buddy_allocator_free(&allocator, (void*)(1 * 32 + fake_area_start_addr));
        // Block 0
        buddy_allocator_free(&allocator, (void*)(0 * 32 + fake_area_start_addr));
        // Wornd, out of memory area
        // Block -1
        buddy_allocator_free(&allocator, (void*)(-1 * 32 + fake_area_start_addr));

        assert(allocator.free_blocks_lists[2].count == 3);
        assert(allocator.free_blocks_lists[1].count == 0);
        assert(allocator.free_blocks_lists[0].count == 0);
    }

    for (uint32_t i = 0; i < 3; ++i) {
        // Try to allocate 12x 8 bytes blocks
        // Blocks 9 - 20
        for (uint32_t j = 0; j < 12; ++j) {
            void* allocated_addr = buddy_allocator_alloc(&allocator, 8);
            assert((uintptr_t)allocated_addr == j * 8 + fake_area_start_addr);
        }

        // Free all allocated blocks 20 - 9
        for (int32_t j = 11; j >= 0; --j) {
            buddy_allocator_free(&allocator, (void*)(j * 8 + fake_area_start_addr));
        }
    }

    free(required_memory);
}

// TESTS_RANDOM STAFF
enum ACTION {
    ALLOCATE_RANDOM_BLOCKS,
    FREE_RANDOM_BLOCKS,
    CHECK_RANDOM_ALLOCATED_BLOCKS
};

uintptr_t g_area_start_addr = 0;

const size_t g_area_size_max = 16384;
const size_t g_area_size_min = 8192;
// Random
size_t g_area_size = 0;

const uint8_t g_max_order_max = 9;
const uint8_t g_max_order_min = 2;
// Random
uint8_t g_max_order = 0;

uint32_t g_page_size = 8;

// number of elements = max_order_max + 1
size_t g_block_sizes[] = { 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };

typedef struct {
    dll_node_t dll_node;
    void* block_ptr;
    uint32_t block_size;
} block_info_t;

doubly_linked_list_t g_allocated_blocks_list;

buddy_allocator_t g_allocator;

static enum ACTION get_random_action()
{
    if (g_allocated_blocks_list.count == 0) {
        return ALLOCATE_RANDOM_BLOCKS;
    }
    // 0-1 - allocate or free
    // 2 - check
    uint8_t af_or_c = rand() % 3;
    if (af_or_c == 0 || af_or_c == 1) {
        // Allocate or free
        if (g_allocated_blocks_list.count == 0) {
            return ALLOCATE_RANDOM_BLOCKS;
        }
        // Let's analyze the free blocks
        // Allocator has free blocks?
        bool allocator_has_free_blocks = false;
        for (uint8_t i = 0; i <= g_max_order; ++i) {
            if (g_allocator.free_blocks_lists[i].count > 0) {
                allocator_has_free_blocks = true;
                break;
            }
        }
        if (allocator_has_free_blocks == false) {
            return FREE_RANDOM_BLOCKS;
        }
        
        // Allocate or free
        // 0 - allocate
        // 1 - free
        uint8_t al_or_f = rand() % 2;
        if (al_or_f == 0) {
            return ALLOCATE_RANDOM_BLOCKS;
        }
        else {
            return FREE_RANDOM_BLOCKS;
        }
    }
    else {
        return CHECK_RANDOM_ALLOCATED_BLOCKS;
    }
}

void do_action_allocate();
void do_action_free();
void do_action_check();

static void do_action()
{
    enum ACTION action = get_random_action();
    switch (action)
    {
    case ALLOCATE_RANDOM_BLOCKS:
        do_action_allocate();
        break;
    case FREE_RANDOM_BLOCKS:
        do_action_free();
        break;
    case CHECK_RANDOM_ALLOCATED_BLOCKS:
        do_action_check();
        break;
    default:
        break;
    }
}

void do_action_allocate()
{
    //[1 - 3]
    uint8_t allocations_number = (1 + (rand() % 3));
    //allocations_number = 1;
    uint8_t successful_allocated_blocks_number = 0;

    // Get max free order
    int8_t max_free_order = -1;
    for (int8_t j = g_max_order; j >= 0; --j) {
        if (g_allocator.free_blocks_lists[j].count > 0) {
            max_free_order = (uint8_t)j;
            break;
        }
    }
    assert(max_free_order >= 0);

    // Try to allocate blocks
    for (uint8_t i = 0; i < allocations_number; ++i) {
        // Try to allocate random block
        uint8_t random_free_order = rand() % (max_free_order + 1);
        size_t random_allocation_size = g_block_sizes[random_free_order];
        //printf("TRY ALLOCATE %u bytes: ", random_allocation_size);
        void* allocated_block_ptr = buddy_allocator_alloc(&g_allocator, random_allocation_size);
        if (allocated_block_ptr == NULL) {
            // Failed to allocate blocks, try again with new size
            break;
        }
        successful_allocated_blocks_number++;
        // Fill block by addresses of block
        for (uint32_t j = 0; j < random_allocation_size / sizeof(void*); j++) {
            void** allocated_block_ptr_array = allocated_block_ptr;
            allocated_block_ptr_array[j] = allocated_block_ptr;
        }
        // Add block to allocated blocks list
        block_info_t* block_info_ptr = malloc(sizeof(block_info_t));
        //printf("Alloc bi: %p\n", block_info_ptr);
        assert(block_info_ptr != NULL);
        block_info_ptr->block_ptr = allocated_block_ptr;
        block_info_ptr->block_size = random_allocation_size;
        //printf("ALLOCATED 0x%p (0x%p)\n", block_info_ptr->block_ptr, (void*)((uintptr_t)(block_info_ptr->block_ptr) - g_area_start_addr));
        //printf("insert %p", (dll_node_t*)block_info_ptr);
        dll_insert_node_to_tail(&g_allocated_blocks_list, (dll_node_t*)block_info_ptr);
    }
    assert(successful_allocated_blocks_number > 0);
    //printf("ALLOCATE %u %u\n", allocations_number, successful_allocated_blocks_number);
}

void do_action_free()
{
    uint8_t max = 3;
    if (g_allocated_blocks_list.count < max) {
        max = (uint8_t)g_allocated_blocks_list.count;
    }
    //[1 - max]
    uint8_t freeing_number = (1 + (rand() % max));
    //freeing_number = 1;
    uint8_t successful_freed_blocks_number = 0;
    for (uint8_t i = 0; i < freeing_number; ++i) {
        // Random block
        //[0 - g_allocated_blocks_list.count - 1]
        uint32_t random_block_index = rand() % (g_allocated_blocks_list.count);
        block_info_t* block_info_ptr = (block_info_t*)dll_get_nth_node(&g_allocated_blocks_list, random_block_index);
        assert(block_info_ptr != NULL);
        //printf("TRY FREE 0x%p (0x%p): ", block_info_ptr->block_ptr, (void*)((uintptr_t)(block_info_ptr->block_ptr) - g_area_start_addr));

        // Check block memory
        for (uint32_t j = 0; j < block_info_ptr->block_size / sizeof(void*); j++) {
            void** block_mem_ptr = block_info_ptr->block_ptr;
            block_mem_ptr[i] = block_info_ptr->block_ptr;
        }

        // Free block
        buddy_allocator_free(&g_allocator, block_info_ptr->block_ptr);
        //printf("remove %p", (dll_node_t*)block_info_ptr);
        dll_remove_node(&g_allocated_blocks_list, (dll_node_t*)block_info_ptr);
        //printf("Free bi: %p\n", block_info_ptr);
        free(block_info_ptr);
        //printf("FREED\n");
        successful_freed_blocks_number++;
    }
    assert(successful_freed_blocks_number > 0);
    //printf("FREE %u %u\n", freeing_number, successful_freed_blocks_number);
}

void do_action_check()
{
    uint8_t max = 3;
    if (g_allocated_blocks_list.count < max) {
        max = (uint8_t)g_allocated_blocks_list.count;
    }
    //[1 - max]
    uint8_t checking_number = (1 + (rand() % max));
    for (uint8_t i = 0; i < checking_number; ++i) {
        // Random block
        //[0 - g_allocated_blocks_list.count - 1]
        uint32_t random_block_index = rand() % (g_allocated_blocks_list.count);
        block_info_t* block_info_ptr = (block_info_t*)dll_get_nth_node(&g_allocated_blocks_list, random_block_index);
        assert(block_info_ptr != NULL);

        // Check block memory
        for (uint32_t j = 0; j < block_info_ptr->block_size / sizeof(void*); j++) {
            void** block_mem_ptr = block_info_ptr->block_ptr;
            block_mem_ptr[i] = block_info_ptr->block_ptr;
        }
    }
}


/*
 * The test randomly allocates, checks and releases(and checks before release) the allocated memory blocks.
 * In each allocated memory block information about it is written, and when the block is released, and also at a random moment, the information in the blocks is checked.
 * This test is intended to check the allocator for correctness of the allocated memory,
 * so that no data corruptions in blocks occur.
 */
void tests_random()
{
    /*
    srand((unsigned int)time(0));
    memset(&g_allocated_blocks_list, 0, sizeof(doubly_linked_list_t));
    memset(&g_allocator, 0, sizeof(buddy_allocator_t));
    g_check_buffer_ptr = malloc(g_check_buffer_size);
    assert(g_check_buffer_ptr != NULL);
    g_area_start_addr = (uintptr_t)malloc(g_area_size);
    printf("g_area_start_addr: 0x%p\n\n", (void*)g_area_start_addr);
    assert(g_area_start_addr != 0);
    size_t required_memory_size = 0;
    buddy_allocator_preinit(&g_allocator, g_area_start_addr, g_area_size, g_max_order, g_page_size, &required_memory_size);
    assert(required_memory_size != 0);
    void* required_memory_ptr = malloc(required_memory_size);
    assert(required_memory_ptr != 0);
    buddy_allocator_init(&g_allocator, required_memory_ptr);
    */
    srand((unsigned int)time(0));

    for (uint32_t k = 1; k <= 8; ++k) {
        // Random allocator data
        // Random test iterations number
        uint32_t random_test_iterations_number_min = 4096;
        uint32_t random_test_iterations_number_max = 8192;
        //[min max]
        uint32_t random_test_iterations_number = random_test_iterations_number_min + (rand() % ((random_test_iterations_number_max + 1) - random_test_iterations_number_min));
        printf("Random iterations number: %u\n", random_test_iterations_number);

        // Generate random area size
        g_area_size = g_area_size_min + (rand() % ((g_area_size_max + 1) - g_area_size_min));
        printf("Random area size number: %u\n", g_area_size);

        // Generate random max order
        g_max_order = g_max_order_min + (rand() % ((g_max_order_max + 1) - g_max_order_min));
        printf("Random max order: %u\n", g_max_order);

        memset(&g_allocated_blocks_list, 0, sizeof(doubly_linked_list_t));
        memset(&g_allocator, 0, sizeof(buddy_allocator_t));
        g_area_start_addr = (uintptr_t)malloc(g_area_size);
        //printf("g_area_start_addr: 0x%p\n\n", (void*)g_area_start_addr);
        assert(g_area_start_addr != 0);
        size_t required_memory_size = 0;
        buddy_allocator_preinit(&g_allocator, g_area_start_addr, g_area_size, g_max_order, g_page_size, &required_memory_size);
        assert(required_memory_size != 0);
        void* required_memory_ptr = malloc(required_memory_size);
        assert(required_memory_ptr != 0);
        buddy_allocator_init(&g_allocator, required_memory_ptr);
        printf("%u in progress...\n", k);

        for (uint32_t j = 0; j < random_test_iterations_number; ++j) {
            //printf("-start %u-\n", j);
            memset(&g_allocated_blocks_list, 0, sizeof(doubly_linked_list_t));
            memset(&g_allocator, 0, sizeof(buddy_allocator_t));
            buddy_allocator_preinit(&g_allocator, g_area_start_addr, g_area_size, g_max_order, g_page_size, &required_memory_size);
            buddy_allocator_init(&g_allocator, required_memory_ptr);
            uint32_t rand_actions_number = rand() % 256;
            for (uint32_t i = 0; i < rand_actions_number; ++i) {
                do_action();
            }
            //printf("-end-free-\n");
            uint32_t allocated_blocks_count = g_allocated_blocks_list.count;
            for (uint32_t i = 0; i < allocated_blocks_count; ++i) {
                uint32_t rand_index = rand() % g_allocated_blocks_list.count;
                //rand_index = i;
                block_info_t* memory_block_ptr = (block_info_t*)dll_get_nth_node(&g_allocated_blocks_list, rand_index);
                dll_remove_node(&g_allocated_blocks_list, (dll_node_t*)memory_block_ptr);
                free(memory_block_ptr);
            }
            //printf("-end-\n");
        }

        free((void*)g_area_start_addr);
        free(required_memory_ptr);
    }
}
