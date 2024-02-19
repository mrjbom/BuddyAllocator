# Buddy Allocator
This is an implementation of the buddy allocator designed for use in the kernel of a 32-bit operating system.  
That's why it doesn't use hosted functions like malloc() or pow(), it is also not intended for use in the 64-bit address space, so it probably won't work correctly in it.  
It uses a doubly-linked list implementation I wrote, you can find it here or among my repositories.  

It's tested by a random test that does random actions in random amounts, so it's pretty reliable.
