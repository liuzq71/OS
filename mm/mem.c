#include <list.h>
#define	NULL	((void *)0)

#define _MEM_END	0x33c00000
#define _MEM_START	0x30100000


#define PAGE_SHIFT	(12)
#define PAGE_SIZE	(1<<PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))

#define KERNEL_MEM_END	(_MEM_END)
/*the bigin and end of the kernel mem which is needed to be paged.*/
#define KERNEL_PAGING_START	((_MEM_START+(~PAGE_MASK))&((PAGE_MASK)))
#define	KERNEL_PAGING_END	(((KERNEL_MEM_END-KERNEL_PAGING_START)/(PAGE_SIZE+sizeof(struct page)))*(PAGE_SIZE)+KERNEL_PAGING_START)

/*page number in need */
#define KERNEL_PAGE_NUM	((KERNEL_PAGING_END-KERNEL_PAGING_START)/PAGE_SIZE)
/*the start and end of the page structure should be storaged in.*/
#define KERNEL_PAGE_END	_MEM_END
#define KERNEL_PAGE_START	(KERNEL_PAGE_END-KERNEL_PAGE_NUM*sizeof(struct page))


/*page flags*/
#define PAGE_AVAILABLE		0x00
#define PAGE_DIRTY			0x01
#define PAGE_PROTECT		0x02
#define PAGE_BUDDY_BUSY		0x04
#define PAGE_IN_CACHE		0x08


struct kmem_cache{
	unsigned int obj_size;
	unsigned int obj_nr;
	unsigned int page_order;
	unsigned int flags;
	struct page *head_page;
	struct page *end_page;
	void *nf_block;
};

struct page {
	unsigned int vaddr;
	unsigned int flags;
	int order;
	struct kmem_cache *cachep;
	struct list_head list;//to string the buddy member
};



#define MAX_BUDDY_PAGE_NUM	(9)	//finally I desided to choose a fixed value,which means users could alloc 1M space at most,those who need more than 1M should change the macro to reserve enough space for private use.

#define AVERAGE_PAGE_NUM_PER_BUDDY	(KERNEL_PAGE_NUM/MAX_BUDDY_PAGE_NUM)
#define PAGE_NUM_FOR_EACH_BUDDY(j) ((AVERAGE_PAGE_NUM_PER_BUDDY>>(j))*(1<<(j)))
#define PAGE_NUM_FOR_MAX_BUDDY	((1<<MAX_BUDDY_PAGE_NUM)-1)//=0xff=255,其实名称有误，最大应该加1

struct list_head page_buddy[MAX_BUDDY_PAGE_NUM];


struct page *virt_to_page(unsigned int addr){
	unsigned int i;
	i=((addr)-KERNEL_PAGING_START)>>PAGE_SHIFT;
	if(i>KERNEL_PAGE_NUM)
		return NULL;
	return (struct page *)KERNEL_PAGE_START+i;
}

void init_page_buddy(void){
	int i;
	for(i=0;i<MAX_BUDDY_PAGE_NUM;i++){
		INIT_LIST_HEAD(&page_buddy[i]);
	}
}


void init_page_map(void){
	int i;
	struct page *pg=(struct page *)KERNEL_PAGE_START;
	init_page_buddy();
	for(i=0;i<(KERNEL_PAGE_NUM);pg++,i++){
/*fill struct page first*/
		pg->vaddr=KERNEL_PAGING_START+i*PAGE_SIZE;	
		pg->flags=PAGE_AVAILABLE;
		INIT_LIST_HEAD(&(pg->list));


/*make the memory max buddy as possible*/
		if(i<(KERNEL_PAGE_NUM&(~PAGE_NUM_FOR_MAX_BUDDY))){	//如果够分配1M，放到page_buddy[MAX_BUDDY_PAGE_NUM-1]
/*the following code should be dealt carefully,we would change the order field of a head struct page to the corresponding order,and change others to -1*/
			if((i&PAGE_NUM_FOR_MAX_BUDDY)==0){//如果凑满256个页，即1M
				pg->order=MAX_BUDDY_PAGE_NUM-1;
			}else{
				pg->order=-1;//表示这个 page 结构体不是 buddy 的上边界
			}
			list_add_tail(&(pg->list),&page_buddy[MAX_BUDDY_PAGE_NUM-1]);
/*the remainder not enough to merge into a max buddy is done as min buddy*/
		}else{
			pg->order=0;
			list_add_tail(&(pg->list),&page_buddy[0]);
		}

	}

}

/*we can do these all because the page structure that represents one page aera is continuous*/
#define BUDDY_END(x,order)	((x)+(1<<(order))-1)
#define NEXT_BUDDY_START(x,order)	((x)+(1<<(order)))
#define PREV_BUDDY_START(x,order)	((x)-(1<<(order)))


/*the logic of this function seems good,no bug reported yet*/
struct page *get_pages_from_list(int order){
	unsigned int vaddr;
	int neworder=order;
	struct page *pg,*ret;
	struct list_head *tlst,*tlst1;
	for(;neworder<MAX_BUDDY_PAGE_NUM;neworder++){//从order遍历page_buddy
		if(list_empty(&page_buddy[neworder])){
			continue;
		}else{
			pg=list_entry(page_buddy[neworder].next,struct page,list);
			tlst=&(BUDDY_END(pg,neworder)->list);
			tlst->next->prev=&page_buddy[neworder];
			page_buddy[neworder].next=tlst->next;
			goto OUT_OK;
		}
	}
	return NULL;
	
OUT_OK:
	for(neworder--;neworder>=order;neworder--){
		
		tlst1=&(BUDDY_END(pg,neworder)->list);
		tlst=&(pg->list);

		pg=NEXT_BUDDY_START(pg,neworder);
		list_entry(tlst,struct page,list)->order=neworder;

		list_add_chain_tail(tlst,tlst1,&page_buddy[neworder]);
	}
	pg->flags|=PAGE_BUDDY_BUSY;
	pg->order=order;
	pg->cachep=NULL;
	return pg;
}



void put_pages_to_list(struct page *pg,int order){
	struct page *tprev,*tnext;
	if(!(pg->flags&PAGE_BUDDY_BUSY)){
		printf("something must be wrong when you see this message,that probably means you are forcing to release a page that was not alloc at all\n");
		return;
	}
	pg->flags&=~(PAGE_BUDDY_BUSY);

	for(;order<MAX_BUDDY_PAGE_NUM;order++){
		tnext=NEXT_BUDDY_START(pg,order);
		tprev=PREV_BUDDY_START(pg,order);
		if((!(tnext->flags&PAGE_BUDDY_BUSY))&&(tnext->order==order)){
			pg->order++;
			tnext->order=-1;
			list_remove_chain(&(tnext->list),&(BUDDY_END(tnext,order)->list));
			BUDDY_END(pg,order)->list.next=&(tnext->list);
			tnext->list.prev=&(BUDDY_END(pg,order)->list);
			continue;
		}else if((!(tprev->flags&PAGE_BUDDY_BUSY))&&(tprev->order==order)){
			pg->order=-1;
			
			list_remove_chain(&(pg->list),&(BUDDY_END(pg,order)->list));
			BUDDY_END(tprev,order)->list.next=&(pg->list);
			pg->list.prev=&(BUDDY_END(tprev,order)->list);
			
			pg=tprev;
			pg->order++;
			continue;
		}else{
			break;
		}
	}
	
	list_add_chain(&(pg->list),&((tnext-1)->list),&page_buddy[order]);
}


void *page_address(struct page *pg){
	return (void *)(pg->vaddr);
}

struct page *alloc_pages(unsigned int flag,int order){
	struct page *pg;
	int i;
	pg=get_pages_from_list(order);
	if(pg==NULL)
		return NULL;
	for(i=0;i<(1<<order);i++){
		(pg+i)->flags|=PAGE_DIRTY;
	}
	return pg;
}

void free_pages(struct page *pg,int order){
	int i;
	for(i=0;i<(1<<order);i++){
		(pg+i)->flags&=~PAGE_DIRTY;
	}
	put_pages_to_list(pg,order);
}

void *get_free_pages(unsigned int flag,int order){
	struct page * page;
	page = alloc_pages(flag, order);
	if (!page)
		return NULL;
	return	page_address(page);
}

void put_free_pages(void *addr,int order){
	free_pages(virt_to_page((unsigned int)addr),order);
}



#define KMEM_CACHE_DEFAULT_ORDER	(0)
#define KMEM_CACHE_MAX_ORDER		(5)			//cache can deal with the memory no less than 32*PAGE_SIZE
#define KMEM_CACHE_SAVE_RATE		(0x5a)
#define KMEM_CACHE_PERCENT			(0x64)
#define KMEM_CACHE_MAX_WAST			(PAGE_SIZE-KMEM_CACHE_SAVE_RATE*PAGE_SIZE/KMEM_CACHE_PERCENT)


int find_right_order(unsigned int size){
	int order;
	for(order=0;order<=KMEM_CACHE_MAX_ORDER;order++){
		if(size<=(KMEM_CACHE_MAX_WAST)*(1<<order)){
			return order;
		}
	}
	if(size>(1<<order))
		return -1;
	return order;
}


int kmem_cache_line_object(void *head,unsigned int size,int order){
	void **pl;
	char *p;
	pl=(void **)head;
	p=(char *)head+size;
	int i,s=PAGE_SIZE*(1<<order);
	for(i=0;s>size;i++,s-=size){
		*pl=(void *)p;//保存下个内存块首地址于当前内存块
		pl=(void **)p;
		p=p+size;
	}
	if(s==size)
		i++;
	return i;
}

struct kmem_cache *kmem_cache_create(struct kmem_cache *cache,unsigned int size,unsigned int flags){
	void **nf_block=&(cache->nf_block);

	int order=find_right_order(size);
	if(order==-1)
		return NULL;
	if((cache->head_page=alloc_pages(0,order))==NULL)
		return NULL;
	*nf_block=page_address(cache->head_page);

	cache->obj_nr=kmem_cache_line_object(*nf_block,size,order);
	cache->obj_size=size;
	cache->page_order=order;
	cache->flags=flags;
	cache->end_page=BUDDY_END(cache->head_page,order);
	cache->end_page->list.next=NULL;

	return cache;
}

/*FIXME:I dont understand it now*/
void kmem_cache_destroy(struct kmem_cache *cache){
	int order=cache->page_order;
	struct page *pg=cache->head_page;
	struct list_head *list;
	while(1){
		list=BUDDY_END(pg,order)->list.next;
		free_pages(pg,order);
		if(list){
			pg=list_entry(list,struct page,list);
		}else{
			return;
		}
	}
}

void kmem_cache_free(struct kmem_cache *cache,void *objp){
	*(void **)objp=cache->nf_block;//保存下个内存块首地址于当前内存块
	cache->nf_block=objp;
	cache->obj_nr++;
}


void *kmem_cache_alloc(struct kmem_cache *cache,unsigned int flag){
	void *p;
	struct page *pg;
	if(cache==NULL)
		return NULL;
	void **nf_block=&(cache->nf_block);
	unsigned int *nr=&(cache->obj_nr);
	int order=cache->page_order;

	if(!*nr){//剩余个数为0
		//再申请slab
		if((pg=alloc_pages(0,order))==NULL)
			return NULL;
		*nf_block=page_address(pg);
		cache->end_page->list.next=&pg->list;
		cache->end_page=BUDDY_END(pg,order);
		cache->end_page->list.next=NULL;
		*nr+=kmem_cache_line_object(*nf_block,cache->obj_size,order);
	}

	(*nr)--;
	//获取对应分配内存块地址
	p=*nf_block;
	//使kmem_cache 结构体的 nf_block 成员指向下一个空闲子内存块
	*nf_block=*(void **)p;
	//获取对应分配内存块所在page结构体
	pg=virt_to_page((unsigned int)p);
	//确保每个 struct page 结构只属于一个 kmem_cache
	pg->cachep=cache;
	return p;
}



#define KMALLOC_BIAS_SHIFT			(5)				//32byte minimal
#define KMALLOC_MAX_SIZE			(4096)
#define KMALLOC_MINIMAL_SIZE_BIAS	(1<<(KMALLOC_BIAS_SHIFT))
#define KMALLOC_CACHE_SIZE			(KMALLOC_MAX_SIZE/KMALLOC_MINIMAL_SIZE_BIAS)
struct kmem_cache kmalloc_cache[KMALLOC_CACHE_SIZE]={{0,0,0,0,NULL,NULL,NULL},};
#define kmalloc_cache_size_to_index(size)	((((size))>>(KMALLOC_BIAS_SHIFT)))


int kmalloc_init(void){
	int i=0;

	for(i=0;i<KMALLOC_CACHE_SIZE;i++){
		if(kmem_cache_create(&kmalloc_cache[i],(i+1)*KMALLOC_MINIMAL_SIZE_BIAS,0)==NULL)
			return -1;
	}
	return 0;
}

void *_kmalloc(unsigned int size){
	int index = kmalloc_cache_size_to_index(size);
	if(index >= KMALLOC_CACHE_SIZE)
		return NULL;
	return kmem_cache_alloc(&kmalloc_cache[index],0);
}

void _kfree(void *addr){
	struct page *pg;
	pg=virt_to_page((unsigned int)addr);
	kmem_cache_free(pg->cachep,addr);
}

int position_of_the_highest_bit_set(unsigned int addr) {
	unsigned int n = addr;
	unsigned int r = 0;
	while (n >>= 1) {
		r++;
	}
	if((1 << r) >= addr) {
		return r;
	} else {
		return r+1;
	}
}

void *kmalloc(unsigned int size)
{
	void *this;
	if(size < 4096)
		this = _kmalloc(size);
	else{
		int pos=position_of_the_highest_bit_set(size >> 12);
		this = get_free_pages(0,pos);
	}
	return this;
}

void kfree(void *addr)
{
	struct page *pg;
	if(addr == NULL)
		return;
	pg=virt_to_page((unsigned int)addr);
	if(pg->cachep != NULL){
		_kfree(addr);
	}else{
		free_pages(pg,pg->order);
	}
}
