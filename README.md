# Buddy Allocator
This is an implementation of the buddy allocator designed for use in the kernel of a 32-bit operating system.  
That's why it doesn't use hosted functions like malloc() or pow(), it is also not intended for use in the 64-bit address space, so it probably won't work correctly in it.  
It uses a doubly-linked list implementation I wrote, you can find it here or among my repositories.  

It's tested by a random test that does random actions in random amounts, so it's pretty reliable.

## How to use:
```
#define MEMORY_AREA_START 0x1000
#define MEMORY_AREA_SIZE 16384
#define MAX_ORDER 2
#define PAGE_SIZE 4096
...
buddy_allocator_t allocator;
uint32_t required_memory_size = 0;
memset(&allocator, 0, sizeof(buddy_allocator_t));
buddy_allocator_preinit(&allocator, MEMORY_AREA_START, MEMORY_AREA_SIZE, MAX_ORDER, PAGE_SIZE, false, &required_memory_size);
if (required_memory_size == 0) {
    // Initialization failed, incorrect parameters
    return -1;
}
// Allocation of memory required by the allocator, with a size of at least required_memory_size
void* required_memory_ptr = malloc(required_memory_size);
buddy_allocator_init(&allocator, required_memory_ptr);

// Allocate 1 page (smallest block)
void* allocated_memory_ptr = buddy_allocator_alloc(&allocator, 4096);
buddy_allocator_free(&allocator, allocated_memory_ptr);

free(required_memory_ptr);
```