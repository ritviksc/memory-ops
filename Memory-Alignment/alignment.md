Ritvik Sharma  
1/8/2026  
University of Otago

# Memory Alignment  


Memory alignment is a term that can stump low level C/C++ engineers and cause baffling bugs while writing code, and make debugging someone's worst nightmare. When I encountered my first alignment bug, it did really stump me, but after some hard work I had a breakthrough. After reading this piece, you should have a good idea about memory operations that take place inside a computer and how respecting the specific operations of memory manipulations can help you write efficient code in C/C++ and avoid nasty bugs along the way.

The CPU can fetch data from memory more efficiently when the address of the memory cell is a multiple of the data size it needs to fetch. That being said, fetching one byte of data is easy and we do not need to worry about alignment of data, but if we need to fetch more data, ensuring alignment will help avoid extra cpu cycles and boost code performance. This is because CPUs move memory in fixed-size chunks, not one byte at a time to reduce CPU workload.

 What is really interesting is the case, where data is misaligned. When I first learnt that reading misaligned data can take more than 1 CPU cycle, I was very puzzled. I reasoned that reading data from any memory address is the same whether or not the address of the data is a multiple of the data size. The answer lies in the architecture of the CPU, nothing more and nothing less. 

Misaligned loads are slow not because memory is byte-addressable, but because registers are word-oriented and the datapath is optimized for fixed alignment. Different architectures have different rules regarding memory alignment, but understanding the core concepts will help you produce safe and portable code.

When we reference a memory address, it seems like the CPU fetches exactly the data we want, but in reality it fetches a larger block containing it. In reality, while you can address any byte, the CPU fetches a whole block that contains that byte. When we say block, we refer to the CPU cache line size which is 64 bytes on x86-64, so we fetch 64 bytes of memory at a time. A cache line is the amount of data moved between cache levels and memory. A cache on the other hand, is fast memory, built into or near the processor, that stores copies of frequently used data and instructions. The cache is essentially split into “blocks” which are the cache lines! It is also  important to know that DRAM(Main Memory) is split into rows (8 KB a row on DDR-4/DDR-5) and the CPU will receive data from the cache line that can fetch the requested data from the row it belongs to into the cache. To make it clear, take a look at the following diagram down below.



##### Rough idea of memory operations in a CPU




Simply what the CPU does is it checks its cache(s) for requested data, and if none of the cache levels have it, it requests the memory row in which the data resides and loads the cache with this data. An important thing to keep in mind is that whenever we reference memory, the CPU will check its cache storage first to check if it can find the data. The reason why CPUs are designed this way is that memory transfers between the CPU and RAM take time since they are not local to each other, but caches are so data transfer is quicker between cache and CPU, thus increasing throughput.

Note: Not all ISA allow misaligned access, some will generate traps and may crash the system. For example, the x86-64 ISA is very forgiving and allows unaligned access( with a performance penalty), but ISAs like MIPS,SPARC do not.

With this in mind, let's go back to the elephant in the room - why does memory alignment matter still? Let’s look at a concrete example to demystify this concept.

### EXAMPLE:

#### Aligned:  
Address:         0x1000  
64-bit word:    [0x1000 ... 0x1007]  
Cache line:     [0x1000 ... 0x103F]  
In this case, the entire word is inside one cache line, so the CPU can load it in 1 cycle.  
Efficient handling of memory when the data is aligned.  

#### Unaligned:  
Address:         0x103F  
64-bit word:    [0x103F ... 0x1046]  
Cache lines:  
 line 1:             [0x1000 ... 0x103F]  
 line 2:             [0x1040 ... 0x107F]  


In this case, the first byte is in line 1, and the remaining bytes are in line 2. This will take more cpu cycles plus the work from extracting bytes from each line and merging them internally. You can see the CPU has to do more work in this case.

With our basics solidified, let's see how alignment plays a crucial role in code, especially in languages that allow users to handle memory operations without garbage collectors.
I will use C for everything, and you should as well. C++ is also fine, so with that out the way, let's dive into some code!

First of all, how do we check if data is aligned? For this case we follow the alignment rule which states data is aligned if its base address is fully divisible by the size of the data (i.e. the modulo is zero). Look at the C snippet down below:
```
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

bool aligned(void *ptr, size_t alignment)
{
    return ((uintptr_t)ptr % (alignment )) == 0; // Condition
}
int main() {

    long var = 100;
    if (aligned(&var, 8))
    {
        printf("Data aligned!");
    } else
    {
        printf("Data misaligned!");
    }
    return 0;
}
```

The output you would expect is:
```
Data aligned!
```
How can we make this misaligned? If we offset the address of the variable by an odd number it will become misaligned. Try it out yourself, and note down the result.   

Now let's dive into a very important memory operation in C - memory copy which you may be familiar with as memcpy() provided in the <string.h> library. The goal is clear, we want to transfer n bytes from the source to the destination. A naive approach would be to transfer byte to byte, which works but we can surely optimize this approach and squeeze some extra performance out of our routine?
A natural optimization is to copy more than one byte at a time—using 8-byte words where possible, then falling back to 4-byte, 2-byte, and finally single-byte transfers for any remaining data.
However, this immediately raises a critical question: are both source and destination pointers suitably aligned for such accesses? 

If the pointers are properly aligned, wider loads and stores can be used safely and efficiently. Otherwise, the implementation must first advance the pointers until the required alignment is reached, after which bulk copying can proceed. Any remaining unaligned tail bytes are then handled separately.
This alignment-aware approach avoids undefined behavior, minimizes unaligned memory accesses, and enables significantly higher throughput by allowing the CPU to operate on wider data paths rather than being forced into slow byte-wise copies.
Once the pointers are aligned, copying data a machine word at a time already offers a noticeable speedup. But modern CPUs can go much further.

Instead of operating on 8 bytes per instruction, CPUs expose SIMD (Single Instruction, Multiple Data) registers that can move 16, 32, or even 64 bytes in a single operation. At this point, the problem stops being how do we copy safely and becomes how do we copy as much as possible per instruction.
With aligned source and destination pointers, the bulk of the copy can be handed off to a tight SIMD loop, leaving only a small head and tail to be handled separately.   


To take advantage of SIMD instructions, we should have an idea of what they are and the capabilities of such instructions. The same operation on multiple pieces of data is executed at once, reducing the number of instructions required. These registers are great for parallel programming, and used widely in multimedia processing. SSE introduced 128-bit XMM registers for SIMD, and AVX extended them to 256-bit YMM and 512-bit ZMM registers for wider vector operations. The naming does not matter, it is just a notation for different sizes of these special register types. A brief introduction will suffice, but if you want to read more on this, check out the x86-64 assembly manual.

You can explore the memcpy implementation in the GitHub repo, along with its associated benchmarks, to see firsthand how critical memory alignment is for performance. By combining careful alignment with wider, vectorized transfers, we can achieve much faster memory operations while avoiding the pitfalls of unaligned access. This case study illustrates how a deep understanding of both software and hardware can guide seemingly simple operations like memory copying, turning them into highly optimized routines.
Memory alignment may seem like a low-level detail, but it has a huge impact on both correctness and performance. 

Properly aligned data allows CPUs to access memory efficiently, reduces the risk of undefined behavior, and enables wider vectorized operations. By paying attention to alignment in even simple routines like memory copying, we can write code that runs faster, scales better, and leverages the full capabilities of modern hardware.


Happy Coding!

