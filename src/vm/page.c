#include "vm/page.h"
#include <debug.h>
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "vm/frame.h"

static struct page *supplemental_get_page_info(struct hash *supplemental_page_table, void *vaddr);
static void free_user_page(void *upage);


static void
free_user_page(void *upage){
	struct thread *t = thread_current();
	void *kpage = pagedir_get_page(t->page_dir, upage);
	frame_free_page(kpage, false);
}

void
supplemental_insert_page(struct hash *supplemental_page_table, struct page *page){
	hash_insert(supplemental_page_table, &page->hash_elem);
}

static struct page *
supplemental_get_page_info(struct hash *supplemental_page_table, void *vaddr){
	struct page p;
	p.vaddr = vaddr;
	struct hash_elem *elem = hash_find(supplemental_page_table, &p.hash_elem);

	if(elem == NULL){
		return NULL;
	}

	return hash_entry(elem, struct page, hash_elem);
}

struct page*
supplemental_create_filesys_info(void *vaddr, struct file *file, size_t offset, size_t lenght, bool writable){
	struct page *page = malloc(sizeof(struct page));
	if(page){
		page->page_status = PAGE_FILESYS;
		page->file = file;
		page->offset = offset;
		page->lenght = lenght;
		page->writable = writable;
		page->vaddr = vaddr;
	}

	return page;
}

struct page*
supplemental_create_zero_info(void *vaddr){
	struct page *page = malloc(sizeof(struct page));
		if(page){
			page->page_status = PAGE_ZERO;
			page->file = NULL;
			page->offset = NULL;
			page->lenght = NULL;
			page->writable = true;
			page->vaddr = vaddr;
		}

		return page;
}
struct page*
supplemental_in_mem_info(void *vaddr, bool writable){
	struct page *page = malloc(sizeof(struct page));
			if(page){
				page->page_status = PAGE_IN_MEM;
				page->file = NULL;
				page->offset = NULL;
				page->lenght = NULL;
				page->writable = writable;
				page->vaddr = vaddr;
			}

			return page;
}
struct page*
supplemental_mmap_info(void *vaddr, mapid_t mapid, size_t moffset, size_t mlenght){
	struct page *page = malloc(sizeof(struct page));
				if(page){
					page->page_status = PAGE_MEM_MAPPED;
					page->mapid = mapid;
					page->moffset = moffset;
					page->mlenght = mlenght;
					page->vaddr = vaddr;
				}

				return page;
}
struct page*
supplemental_create_swap_info(void *vaddr, block_sector_t block, bool in_use){
	struct page *page = malloc(sizeof(struct page));
				if(page){
					page->page_status = PAGE_SWAP;
					page->block = block;
					page->in_use = in_use;
					page->writable = false;
					page->vaddr = vaddr;
				}

				return page;
}

void
mark_page_in_mem(struct hash *supplemental_page_table, void *uaddr){
	struct page *page = supplemental_get_page_info(supplemental_page_table, uaddr);
	if(!page){
		PANIC("Nu se poate marca pagina %p", page);
	}
	page->page_status = PAGE_IN_MEM;
}

void
mark_page_not_in_mem(struct hash *supplemental_page_table, void *uaddr){
	struct page *page = supplemental_get_page_info(supplemental_page_table, uaddr);
		if(!page){
			PANIC("Nu se poate marca pagina %p", page);
		}
		page->page_status = PAGE_NOT_IN_MEM;
}

bool
supplemental_exists(struct hash *supplemental_page_table, void *uaddr, struct page **entry){
	struct page *page = supplemental_get_page_info(supplemental_page_table, uaddr);
	if(!page){
		return false;
	}

	if(entry){
		*entry  = page;
	}

	return true;
}

bool
supplemental_is_writable(struct hash *supplemental_page_table, void *uaddr){
	struct page *page = supplemental_get_page_info(supplemental_page_table, uaddr);
		if(!page){
			PANIC("Nu se poate determina daca pagina %p este writable", page);
		}

		return page->writable;
}

void
supplemental_remove_page(struct hash *supplemental_page_table, void *uaddr){
	struct page page;

	p.vaddr = uaddr;

	struct page *page_aux = NULL;
	if(!supplemental_exits(supplemental_page_table, uaddr, &page_aux)){
		return ;
	}

	hash_delete(supplemental_page_table, &page.hash_elem);
	free(page_aux);
}

void
supplemental_page_table_destroy(struct hash_elem *elem, void *aux){
	struct page *page = hahs_entry(elem, struct page, hash_elem);

	if(page->page_status == PAGE_IN_MEM){
		if(page->vaddr){
			free_user_page(page->vaddr);
		}
	}

	if(page->page_status == PAGE_FILESYS){
		free(page->file);
		page->file = NULL;
		page->offset = NULL;
		page->lenght = NULL;
	}

	if(page->page_status == PAGE_MEM_MAPPED){
		page->mappid = NULL;
		page->moffest = NULL;
		page->mlenght = NULL;
	}

	if(page->page_status == PAGE_SWAP){
		page->block = NULL;
		page->in_use = false;
	}

	free(page);
}


