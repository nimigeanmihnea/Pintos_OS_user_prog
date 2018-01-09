#ifdef VM_SWAP_H

#define VM_SWAP_H

#include <stdbool.h>
#include "devices/block.h"
#include "vm/page.h"

struct swap_entry{
	block_sector_t block;
	bool in_use;
};

void swap_init(void);
void swap_destroy(void);
struct swap_entry *swap_alloc(void);
void swap_save(struct swap_entry *swap_location, void *paddr);
void *swap_load(struct swap_entry *swap_location, struct page *page, void *kernel_vaddr);

#endif
