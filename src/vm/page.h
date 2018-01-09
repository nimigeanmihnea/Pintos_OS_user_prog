#ifdef VM_PAGE_H

#define VM_PAGE_H

#include <hash.h>
#include "threads/thread.h"
#include "lib/user/syscall.h"
#include "vm/swap.h"
#include "devices/block.h"

enum page_status{
	PAGE_UNDIFINED = 0,
	PAGE_FILESYS = 1,
	PAGE_SWAP = 2,
	PAGE_MEM_MAPPED = 3,
	PAGE_IN_MEM = 4,
	PAGE_ZERO = 5,
	PAGE_NOT_IN_MEM = 6,
};

struct page{
	struct hash_elem hash_elem;
	void *vaddr;
	//info
	struct file *file;
	size_t offset;
	size_t lenght;
	//
	//mmaped
	mapid_t mapid;
	size_t moffeset;
	size_t mlenght;
	//
	//swap_entry
	block_sector_t block;
	bool in_use;
	//
	enum page_status page_status;
	bool writable;
};

struct page* supplemental_create_filesys_info(void *vaddr, struct file *file, size_t offset, size_t lenght, bool writable);
struct page* supplemental_create_zero_info(void *vaddr);
struct page* supplemental_in_mem_info(void *vaddr, bool writable);
struct page* supplemental_mmap_info(void *vaddr, mapid_t mapid, size_t moffset, size_t mlenght);
struct page* supplemental_create_swap_info(void *vaddr, block_sector_t block, bool in_use);

void supplemental_insert_page(struct hash *supplemental_page_table, struct page *page);
void mark_page_in_mem(struct hash *supplemental_page_table, void *uaddr);
void mark_page_not_in_mem(struct hash *supplemental_page_table, void *uaddr);
bool supplemental_exists(struct hash *supplemental_page_table, void *uaddr, struct page **entry);
bool supplemental_is_writable(struct hash *supplemental_page_table, void *uaddr);
void supplemental_remove_page(struct hash *supplemental_page_table, void *uaddr);

void supplemental_page_table_destroy(struct hash_elem *elem, void *aux);

#endif
