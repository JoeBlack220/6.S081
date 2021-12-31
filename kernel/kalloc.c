// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem {
  struct spinlock lock;
  struct run *freelist;
};

struct kmem kmemlist[NCPU];

void
kinit()
{
	for(int i = 0; i < NCPU; i++) {
		initlock(&kmemlist[i].lock, "kmem");
	}
	freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
	
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);
	
	push_off();
	int idx = cpuid();

  r = (struct run*)pa;

  acquire(&kmemlist[idx].lock);
  r->next = kmemlist[idx].freelist;
  kmemlist[idx].freelist = r;
  release(&kmemlist[idx].lock);
	pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
	
	push_off();
	int idx = cpuid();
  acquire(&kmemlist[idx].lock);
  r = kmemlist[idx].freelist;
  release(&kmemlist[idx].lock);
	if(r) {
    kmemlist[idx].freelist = r->next;
	} else {	
			for(int i = 0; i < NCPU; ++i) {
				acquire(&kmemlist[i].lock);
				r = kmemlist[i].freelist;
				if(r) {
					kmemlist[i].freelist = r->next;
					release(&kmemlist[i].lock);
					break;
				} else {
					release(&kmemlist[i].lock);
				}
			}
	}

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
	pop_off();
  return (void*)r;
}
