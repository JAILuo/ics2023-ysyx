#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <proc.h>

static void *pf = NULL;

void* new_page(size_t nr_page) {
    // return low address
    void *old_pf = pf;
    //Log("before alloc: %p", old_pf);
    pf += nr_page * PGSIZE;
    assert(pf <= heap.end);
    //Log("after alloc, ustack.start/stack_bottom: %p", pf);
    return old_pf;
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
    //Log("brk: 0x%x  current->max_brk: 0x%x", brk, current->max_brk);
    if (current->max_brk == 0) {
        current->max_brk = (brk % PGSIZE == 0) ?
                            brk : (brk / PGSIZE + 1) * PGSIZE;
    } else if (brk > current->max_brk) {
        uintptr_t gap = brk - current->max_brk;
        size_t nr_page = gap / PGSIZE + ((gap % PGSIZE == 0) ? 0 : 1);
        void *va_base = (void *)(current->max_brk);
        void *pa_base = new_page(nr_page);

        //Log("[gap]: 0x%x  [nr_page]: 0x%x  [va_base]: %p  [pa_base]: %p",
        //    gap, nr_page, va_base, pa_base);
        for (int i = 0; i < nr_page; i++) {
            map(&current->as,
                va_base + (i * PGSIZE),
                pa_base + (i * PGSIZE), 
                PTE_R | PTE_W); // other bits TODO
        }
        current->max_brk += nr_page * PGSIZE;
    }
    //Log("current->max_brk: %p", (void *)current->max_brk);
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
