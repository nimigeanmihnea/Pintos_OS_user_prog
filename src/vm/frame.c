#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"

#include <debug.h>

#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"



struct lock frame_table_lock;
struct lock frame_alloc_lock;

static bool hash_function(const struct hash_elem *a, const struct hash_elem *b, void *aux){
	const struct frame *f1=hash_entry(a, struct frame, hash_elem);
	const struct frame *f2=hash_entry(b, struct frame, hash_elem);
	return f1->frame_adr < f2->frame_adr;

}

static unsigned hash_hash_funct(const struct hash_elem *e, void *aux UNUSED){

	const struct frame *f=hash_entry(e, struct frame, hash_elem);
	return hash_bytes(&f->frame_adr, sizeof(f->frame_adr) );


}
void
init_frame_table(void){

	hash_init(&frame_table, frame_hash, hash_function, NULL);
	lock_init(&frame_table_lock);
	lock_init(&frame_alloc_lock);


}
void
frame_free_page(void *kernel_vaddr, bool locked){

	if(!locked)
		lock_acquire(&frame_alloc_lock);
	palloc_free_page(kernel_vaddr);
	struct frame found_frame;
	found_frame->frame_adr=kernel_vaddr;
	struct hash_elem *e=hash_find(&frame_table, &found_frame->hash_elem);

	struct frame *frame = hash_entry(e, struct frame, hash_elem);
	if(frame){
		frame->page->page_status=PAGE_NOT_IN_MEM;
		tid_t tid=frame->tid;

		struct thread *t=get_thread(tid);
		if(t){
			pagedir_clear_page(t->pagedir, frame->page->vadr);
			frame_unmap(kernel_vaddr);
			free(frame);
		}
	}

	if(!locked){
		lock_release(&frame_alloc_lock);
	}

}

void frame_map(void *frame_addr, struct page *page, bool writable){
	struct frame* new =NULL;
	new = malloc(sizeof(struct frame));

	new->page = page;
	new->frame_adr = frame_addr;
	new->tid = thread_current()->tid;
	new->not_evicted = 0;

	lock_acquire(&frame_table_lock);
	hash_insert(&frame_table, &frame->hash_elem);
	lock_release(&frame_table_lock);

}

void frame_unmap(void *frame_addr){
	struct frame frame;
	frame.frame_adr = frame_addr;
	lock_acquire(&frame_table_lock);
	hash_delete(&frame_table, &new->hash_elem);
	lock_release(&frame_table_lock);
}

static void
frame_save_frame(struct frame* frame){

	tid_t tid = frame->tid;
	struct thread *t = get_thread(tid);
	if(t){
		bool dirty = pagedir_is_dirty(t->pagedir, frame->page->vaddr);
		if((frame->page->page_status == PAGE_MEM_MAPPED) && dirty){
			struct mmap_mapping *map = mmap_get_mapping(&t->mmap_table, frame->page->mapid);
			mmap_save_data(map, frame->frame_adr, frame->page->moffset, frame->page->mlength );
		}
		else
			if(!(frame->page->page_status == PAGE_FILESYS)){
				struct swap_entry *entry = swap_alloc();
				if(!entry){
					PANIC("Nu mai este memorie in swap");
				}
				frame->page->page_status = PAGE_SWAP;
				frame->page->block = entry->block;
				frame->page->in_use = entry->in_use;
				swap_save(entry, (void*)frame->frame_adr);
			}
	}
}

struct frame*
frame_choose_frame(void){
	struct hash_iterator i;
	struct thread* t;
	struct frame* candidate;

	bool acc_cand = true;
	bool dirty_cand = true;
	int32_t least_used = 0;

	lock_acquire(&frame_table_lock);
	hash_first(&i, &frame_table);

	while(has_next(&i)){
		struct frame *frame = hash_entry(hash_cur(&i), struct frame, hash_elem);
		t=get_thread(frame->tid);
		bool dirty = pagedir_is_dirty(t->pagedir, frame->frame_adr);
		bool accessed = pagedir_is_accessed(t->pagedir, frame->frame_adr);

		if(accessed){
			if(!acc_cand){
				frame->not_evicted = 0;
				break;
			}
		}else
			frame->not_evicted ++;
		if(dirty){
			if(!dirty_cand){
				frame->not_evicted = 0;
				break;
			}
		}else
			frame->not_evicted ++;

		if(++frame->not_evicted > least_used && (frame->page->page_status == PAGE_FILESYS)){
			candidate = frame;
			dirty_cand = dirty;
			acc_cand= accessed;
			least_used = frame->not_evicted;
		}
	}

	candidate->not_evicted = 0;
	lock_release(&frame_table_lock);
	return candidate;
}
void
*frame_alloc_page(struct page *page, enum palloc_flags flags, bool writable){
	lock_acquire(&frame_alloc_lock);
	void *uvadr=page->vaddr;
	void *kvadr=palloc_get_page(PAL_USER | flags);
	if(!kvadr){

		struct frame *frame=frame_choose_frame();
		frame_save_frame(frame);
		frame_free_page(frame->frame_adr, true);
		kvadr=palloc_get_page(PAL_USER | flags);

	}
	bool instalation=install_page(uvadr, kvadr,writable);
	if(!instalation){
		PANIC("Nu s-a putut instala pagina");

	}

	frame_map(kvadr, page, writable);
	lock_release(&frame_alloc_lock);
	memset(kvadr, 0, PGSIZE);
	return kvadr;
}

