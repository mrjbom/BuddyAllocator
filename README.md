# Buddy Allocator
This is an implementation of the buddy allocator designed for use in the kernel of a 32-bit operating system.  
That's why it doesn't use hosted functions like malloc() or pow(), it is also not intended for use in the 64-bit address space, so it probably won't work correctly in it.  
It also uses stack and doubly-linked list implementations I wrote, you can find them among my repositories.  
