#include <memory.h>

static void *pf = NULL;

/*
void* new_page(size_t nr_page) {
    // return low address
    void *old_pf = pf;
    Log("before alloc: %p", old_pf);
    //assert(pf >= (void *)0x83000000);
    pf += nr_page * PGSIZE;
    assert(pf <= heap.end);
    Log("after alloc, ustack.start/stack_bottom: %p", pf);
    return old_pf;
}
*/
void* new_page(size_t nr_page) {
  pf += nr_page * PGSIZE;
  Log("ustack.end: %p", pf);
  return pf;
}

#ifdef HAS_VME
static void* pg_alloc(int n) {
    void *page = new_page(n / PGSIZE);
    assert(page != NULL);
    memset(page, '\0', n);
    return page;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("pf, heap.start: %p", pf);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
