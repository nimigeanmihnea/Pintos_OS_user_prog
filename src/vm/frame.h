#ifdef VM_FRAME_H

#define VM_FRAME_H

#include <hash.h>
#include "vm/page.h"
#include "threads/palloc.h"

struct hash frame_table;

struct frame{
	struct hash_elem hash_elem; //Folosit pentru salvarea frame-ului in frame table
	int32_t frame_adr; //Adresa frame-ului;
	struct page *page; //Pagina ce va fi mapata
	tid_t tid; //Id-ul thread-ului care detine frame-ul
	int32_t not_evicted; //De cate ori frame-ul nu a fost folosit
};

void init_frame_table(void);
void *frame_alloc_page(struct page *page, enum palloc_flags flags, bool writable);
void frame_free_page(void *kernel_vaddr, bool locked);

#endif
