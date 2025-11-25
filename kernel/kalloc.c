// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// pa4: struct for page control
struct page pages[PHYSTOP/PGSIZE];
struct page *page_lru_head;
int num_free_pages;
int num_lru_pages;

struct spinlock lru_lock;
char *swap_bitmap;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&lru_lock, "lru");
  page_lru_head = 0;
  for(int i = 0; i < PHYSTOP/PGSIZE; i++) {
    pages[i].next = 0;
    pages[i].prev = 0;
    pages[i].pagetable = 0;
    pages[i].vaddr = 0;
  }
  freerange(end, (void*)PHYSTOP);
  swap_bitmap = kalloc();
  if(swap_bitmap)
    memset(swap_bitmap, 0, PGSIZE);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
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

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

void
lru_add(struct page *pg)
{
  acquire(&lru_lock);
  if(page_lru_head == 0) {
    page_lru_head = pg;
    pg->next = pg;
    pg->prev = pg;
  } else {
    struct page *tail = page_lru_head->prev;
    tail->next = pg;
    pg->prev = tail;
    pg->next = page_lru_head;
    page_lru_head->prev = pg;
  }
  release(&lru_lock);
}

void
lru_remove(struct page *pg)
{
  acquire(&lru_lock);
  if(pg->next == 0 || pg->prev == 0) {
    release(&lru_lock);
    return;
  }
  if(pg->next == pg) {
    page_lru_head = 0;
  } else {
    if(page_lru_head == pg)
      page_lru_head = pg->next;
    pg->prev->next = pg->next;
    pg->next->prev = pg->prev;
  }
  pg->next = 0;
  pg->prev = 0;
  release(&lru_lock);
}

int
bitmap_alloc(void)
{
  int maxblocks = SWAPMAX / (PGSIZE/BSIZE);
  for(int i = 0; i < maxblocks; i++) {
    int byte = i / 8;
    int bit = i % 8;
    if((swap_bitmap[byte] & (1 << bit)) == 0) {
      swap_bitmap[byte] |= (1 << bit);
      return i;
    }
  }
  return -1;
}

void
bitmap_free(int blkno)
{
  int byte = blkno / 8;
  int bit = blkno % 8;
  swap_bitmap[byte] &= ~(1 << bit);
}

struct page*
select_victim(void)
{
  if(page_lru_head == 0)
    return 0;

  struct page *pg = page_lru_head;
  struct page *start = pg;
  int scanned = 0;

  while(1) {
    pte_t *pte = walk(pg->pagetable, (uint64)pg->vaddr, 0);
    if(pte == 0) {
      page_lru_head = pg->next;
      return pg;
    }

    if(*pte & PTE_A) {
      *pte &= ~PTE_A;
      page_lru_head = pg->next;
      pg = pg->next;
      scanned++;
      if(pg == start) {
        page_lru_head = pg->next;
        return pg;
      }
    } else {
      page_lru_head = pg->next;
      return pg;
    }
  }
}

int
swapout(void)
{
  struct page *victim = select_victim();
  if(victim == 0)
    return 0;

  pte_t *pte = walk(victim->pagetable, (uint64)victim->vaddr, 0);
  if(pte == 0)
    panic("swapout: walk");

  uint64 pa = PTE2PA(*pte);

  int blkno = bitmap_alloc();
  if(blkno < 0)
    return 0;

  printf("[SWAPOUT] va=0x%p blkno=%d\n", victim->vaddr, blkno);
  swapwrite(pa, blkno);

  lru_remove(victim);

  *pte = (blkno << 10) | PTE_FLAGS(*pte);
  *pte &= ~PTE_V;

  kfree((void*)pa);

  return 1;
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
// pa4: kalloc function
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  else {
    release(&kmem.lock);
    if(!swapout())
      return 0;
    acquire(&kmem.lock);
    r = kmem.freelist;
    if(r)
      kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE);
  return (void*)r;
}