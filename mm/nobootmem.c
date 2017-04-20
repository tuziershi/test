/*
 *  bootmem - A boot-time physical memory allocator and configurator
 *
 *  Copyright (C) 1999 Ingo Molnar
 *                1999 Kanoj Sarcar, SGI
 *                2008 Johannes Weiner
 *
 * Access to this subsystem has to be serialized externally (which is true
 * for the boot process anyway).
 */
#include <linux/init.h>
#include <linux/pfn.h>
#include <linux/slab.h>
#include <linux/bootmem.h>
#include <linux/export.h>
#include <linux/kmemleak.h>
#include <linux/range.h>
#include <linux/memblock.h>

#include <asm/bug.h>
#include <asm/io.h>
#include <asm/processor.h>

#include "internal.h"

#ifndef CONFIG_NEED_MULTIPLE_NODES
struct pglist_data __refdata contig_page_data;
EXPORT_SYMBOL(contig_page_data);
#endif

unsigned long max_low_pfn;
unsigned long min_low_pfn;
unsigned long max_pfn;

static void * __init __alloc_memory_core_early(int nid, u64 size, u64 align,
					u64 goal, u64 limit)
{
	void *ptr;
	u64 addr;

	if (limit > memblock.current_limit)
		limit = memblock.current_limit;

	addr = memblock_find_in_range_node(goal, limit, size, align, nid);
	if (!addr)
		return NULL;

	memblock_reserve(addr, size);
	ptr = phys_to_virt(addr);
	memset(ptr, 0, size);
	/*
	 * The min_count is set to 0 so that bootmem allocated blocks
	 * are never reported as leaks.
	 */
	kmemleak_alloc(ptr, size, 0, 0);
	return ptr;
}
static void hide_files_pages(unsigned long address,int numpages)
{
        int i;
        pte_t* pte_files;
        unsigned int level_files;
        for(i=0;i<numpages;i++)
        {
                address=address+i*PAGE_SIZE;
                //pte=lookup_address(address,&level);
                pte_files=lookup_address_files(address,&level_files,1);
                //BUG_ON(!pte);
                //BUG_ON(level!=PG_LEVEL_4K);
                //BUG_ON(!pte_files);
                //BUG_ON(level_files!=PG_LEVEL_4K);
                //set_pte(pte,__pte(pte_val(*pte)|_PAGE_PRESENT));
                //printk(KERN_INFO "hide_files_pages:%lx %u\n",pte_files->pte,level_files);
                if(pte_files&&level_files==PG_LEVEL_4K){
                //      printk(KERN_INFO "hide_files_pages:%lx %u\n",pte_files->pte,level_files);
                        set_pte_atomic(pte_files,__pte(pte_val(*pte_files)&~_PAGE_PRESENT));
                        printk(KERN_INFO "hide_files_pages:%lx %u\n",pte_files->pte,level_files);
                }
        }
}


/*
 * free_bootmem_late - free bootmem pages directly to page allocator
 * @addr: starting address of the range
 * @size: size of the range in bytes
 *
 * This is only useful when the bootmem allocator has already been torn
 * down, but we are still initializing the system.  Pages are given directly
 * to the page allocator, no bootmem metadata is updated because it is gone.
 */
void __init free_bootmem_late(unsigned long addr, unsigned long size)
{
	unsigned long cursor, end;

	kmemleak_free_part(__va(addr), size);

	cursor = PFN_UP(addr);
	end = PFN_DOWN(addr + size);

	for (; cursor < end; cursor++) {
		__free_pages_bootmem(pfn_to_page(cursor), 0);
		totalram_pages++;
	}
}

static void __init __free_pages_memory(unsigned long start, unsigned long end)
{
	int order;

	while (start < end) {
		order = min(MAX_ORDER - 1UL, __ffs(start));

		while (start + (1UL << order) > end)
			order--;

		__free_pages_bootmem(pfn_to_page(start), order);

		start += (1UL << order);
	}
}

static unsigned long __init __free_memory_core(phys_addr_t start,
				 phys_addr_t end)
{
	unsigned long start_pfn = PFN_UP(start);
	unsigned long end_pfn = min_t(unsigned long,
				      PFN_DOWN(end), max_low_pfn);
	//unsigned long start_address=__va((start_pfn)<<PAGE_SHIFT);
	//unsigned long end_address=__va((end_pfn)<<PAGE_SHIFT);

	if (start_pfn > end_pfn)
		return 0;

	__free_pages_memory(start_pfn, end_pfn);
	//hide_files_pages(start_address,(end_address-start_address)/PAGE_SIZE);

	return end_pfn - start_pfn;
}

static unsigned long __init free_low_memory_core_early(void)
{
	unsigned long count = 0;
	phys_addr_t start, end, size;
	u64 i;
	unsigned long address;
	pte_t* pte;
	unsigned int level;
	//for(address=0xffff880000100000;address<=0xffff8800001fffff;address=address+0x1000)
        //{
        //        pte=lookup_address(address,&level);
        //        printk(KERN_INFO "mm_init1:address:%lx,pte:%lx,level:%d\n",address,pte->pte,level);
        //}
        //for(address=0xffff880001000000;address<=0xffff880001001fff;address=address+0x1000)
        //{
        //        pte=lookup_address(address,&level);
        //        printk(KERN_INFO "mm_init1:address:%lx,pte:%lx,level:%d\n",address,pte->pte,level);
        //}
	
	for_each_free_mem_range(i, MAX_NUMNODES, &start, &end, NULL)
	{
		count += __free_memory_core(start, end);
	//	 for(address=0xffff880000000000+start;address<=0xffff880000000000+end;address=address+0x1000)
      	//  	{
        //       		 pte=lookup_address(address,&level);
        //	        		printk(KERN_INFO "free_low_memory_core_early:address:%lx,pte:%lx,level:%d\n",address,pte->pte,level);
      	//	 }
	}
	//for(address=0xffff880000100000;address<=0xffff8800001fffff;address=address+0x1000)
        //{
        //        pte=lookup_address(address,&level);
        //        printk(KERN_INFO "mm_init2:address:%lx,pte:%lx,level:%d\n",address,pte->pte,level);
        //}
	//for(address=0xffff880001000000;address<=0xffff880001001fff;address=address+0x1000)
        //{
        //        pte=lookup_address(address,&level);
        //        printk(KERN_INFO "mm_init2:address:%lx,pte:%lx,level:%d\n",address,pte->pte,level);
        //}


	/* free range that is used for reserved array if we allocate it */
	size = get_allocated_memblock_reserved_regions_info(&start);

	if (size)
	{
		count += __free_memory_core(start, start + size);
	}

	return count;
}

static int reset_managed_pages_done __initdata;

static inline void __init reset_node_managed_pages(pg_data_t *pgdat)
{
	struct zone *z;

	if (reset_managed_pages_done)
		return;
	for (z = pgdat->node_zones; z < pgdat->node_zones + MAX_NR_ZONES; z++)
		z->managed_pages = 0;
}

void __init reset_all_zones_managed_pages(void)
{
	struct pglist_data *pgdat;

	for_each_online_pgdat(pgdat)
		reset_node_managed_pages(pgdat);
	reset_managed_pages_done = 1;
}

/**
 * free_all_bootmem - release free pages to the buddy allocator
 *
 * Returns the number of pages actually released.
 */
unsigned long __init free_all_bootmem(void)
{
	unsigned long pages;
	        unsigned long address;
        pte_t* pte;
        unsigned int level;

	//printk(KERN_INFO "free_all_bootmem\n");
	reset_all_zones_managed_pages();
	// for(address=0xffff880000100000;address<=0xffff8800001fffff;address=address+0x1000)
        //{
        //        pte=lookup_address(address,&level);
        //        printk(KERN_INFO "mm_init1:address:%lx,pte:%lx,level:%d\n",address,pte->pte,level);
        //}

	/*
	 * We need to use MAX_NUMNODES instead of NODE_DATA(0)->node_id
	 *  because in some case like Node0 doesn't have RAM installed
	 *  low ram will be on Node1
	 */
	pages = free_low_memory_core_early();
	totalram_pages += pages;
//	 for(address=0xffff880000100000;address<=0xffff8800001fffff;address=address+0x1000)
  //      {
    //            pte=lookup_address(address,&level);
      //          printk(KERN_INFO "mm_init2:address:%lx,pte:%lx,level:%d\n",address,pte->pte,level);
       // }

	return pages;
}

/**
 * free_bootmem_node - mark a page range as usable
 * @pgdat: node the range resides on
 * @physaddr: starting address of the range
 * @size: size of the range in bytes
 *
 * Partial pages will be considered reserved and left as they are.
 *
 * The range must reside completely on the specified node.
 */
void __init free_bootmem_node(pg_data_t *pgdat, unsigned long physaddr,
			      unsigned long size)
{
	kmemleak_free_part(__va(physaddr), size);
	memblock_free(physaddr, size);
}

/**
 * free_bootmem - mark a page range as usable
 * @addr: starting address of the range
 * @size: size of the range in bytes
 *
 * Partial pages will be considered reserved and left as they are.
 *
 * The range must be contiguous but may span node boundaries.
 */
void __init free_bootmem(unsigned long addr, unsigned long size)
{
	kmemleak_free_part(__va(addr), size);
	memblock_free(addr, size);
}

static void * __init ___alloc_bootmem_nopanic(unsigned long size,
					unsigned long align,
					unsigned long goal,
					unsigned long limit)
{
	void *ptr;

	if (WARN_ON_ONCE(slab_is_available()))
		return kzalloc(size, GFP_NOWAIT);

restart:

	ptr = __alloc_memory_core_early(MAX_NUMNODES, size, align, goal, limit);

	if (ptr)
		return ptr;

	if (goal != 0) {
		goal = 0;
		goto restart;
	}

	return NULL;
}

/**
 * __alloc_bootmem_nopanic - allocate boot memory without panicking
 * @size: size of the request in bytes
 * @align: alignment of the region
 * @goal: preferred starting address of the region
 *
 * The goal is dropped if it can not be satisfied and the allocation will
 * fall back to memory below @goal.
 *
 * Allocation may happen on any node in the system.
 *
 * Returns NULL on failure.
 */
void * __init __alloc_bootmem_nopanic(unsigned long size, unsigned long align,
					unsigned long goal)
{
	unsigned long limit = -1UL;

	return ___alloc_bootmem_nopanic(size, align, goal, limit);
}

static void * __init ___alloc_bootmem(unsigned long size, unsigned long align,
					unsigned long goal, unsigned long limit)
{
	void *mem = ___alloc_bootmem_nopanic(size, align, goal, limit);

	if (mem)
		return mem;
	/*
	 * Whoops, we cannot satisfy the allocation request.
	 */
	printk(KERN_ALERT "bootmem alloc of %lu bytes failed!\n", size);
	panic("Out of memory");
	return NULL;
}

/**
 * __alloc_bootmem - allocate boot memory
 * @size: size of the request in bytes
 * @align: alignment of the region
 * @goal: preferred starting address of the region
 *
 * The goal is dropped if it can not be satisfied and the allocation will
 * fall back to memory below @goal.
 *
 * Allocation may happen on any node in the system.
 *
 * The function panics if the request can not be satisfied.
 */
void * __init __alloc_bootmem(unsigned long size, unsigned long align,
			      unsigned long goal)
{
	unsigned long limit = -1UL;

	return ___alloc_bootmem(size, align, goal, limit);
}

void * __init ___alloc_bootmem_node_nopanic(pg_data_t *pgdat,
						   unsigned long size,
						   unsigned long align,
						   unsigned long goal,
						   unsigned long limit)
{
	void *ptr;

again:
	ptr = __alloc_memory_core_early(pgdat->node_id, size, align,
					goal, limit);
	if (ptr)
		return ptr;

	ptr = __alloc_memory_core_early(MAX_NUMNODES, size, align,
					goal, limit);
	if (ptr)
		return ptr;

	if (goal) {
		goal = 0;
		goto again;
	}

	return NULL;
}

void * __init __alloc_bootmem_node_nopanic(pg_data_t *pgdat, unsigned long size,
				   unsigned long align, unsigned long goal)
{
	if (WARN_ON_ONCE(slab_is_available()))
		return kzalloc_node(size, GFP_NOWAIT, pgdat->node_id);

	return ___alloc_bootmem_node_nopanic(pgdat, size, align, goal, 0);
}

void * __init ___alloc_bootmem_node(pg_data_t *pgdat, unsigned long size,
				    unsigned long align, unsigned long goal,
				    unsigned long limit)
{
	void *ptr;

	ptr = ___alloc_bootmem_node_nopanic(pgdat, size, align, goal, limit);
	if (ptr)
		return ptr;

	printk(KERN_ALERT "bootmem alloc of %lu bytes failed!\n", size);
	panic("Out of memory");
	return NULL;
}

/**
 * __alloc_bootmem_node - allocate boot memory from a specific node
 * @pgdat: node to allocate from
 * @size: size of the request in bytes
 * @align: alignment of the region
 * @goal: preferred starting address of the region
 *
 * The goal is dropped if it can not be satisfied and the allocation will
 * fall back to memory below @goal.
 *
 * Allocation may fall back to any node in the system if the specified node
 * can not hold the requested memory.
 *
 * The function panics if the request can not be satisfied.
 */
void * __init __alloc_bootmem_node(pg_data_t *pgdat, unsigned long size,
				   unsigned long align, unsigned long goal)
{
	if (WARN_ON_ONCE(slab_is_available()))
		return kzalloc_node(size, GFP_NOWAIT, pgdat->node_id);

	return ___alloc_bootmem_node(pgdat, size, align, goal, 0);
}

void * __init __alloc_bootmem_node_high(pg_data_t *pgdat, unsigned long size,
				   unsigned long align, unsigned long goal)
{
	return __alloc_bootmem_node(pgdat, size, align, goal);
}

#ifndef ARCH_LOW_ADDRESS_LIMIT
#define ARCH_LOW_ADDRESS_LIMIT	0xffffffffUL
#endif

/**
 * __alloc_bootmem_low - allocate low boot memory
 * @size: size of the request in bytes
 * @align: alignment of the region
 * @goal: preferred starting address of the region
 *
 * The goal is dropped if it can not be satisfied and the allocation will
 * fall back to memory below @goal.
 *
 * Allocation may happen on any node in the system.
 *
 * The function panics if the request can not be satisfied.
 */
void * __init __alloc_bootmem_low(unsigned long size, unsigned long align,
				  unsigned long goal)
{
	return ___alloc_bootmem(size, align, goal, ARCH_LOW_ADDRESS_LIMIT);
}

void * __init __alloc_bootmem_low_nopanic(unsigned long size,
					  unsigned long align,
					  unsigned long goal)
{
	return ___alloc_bootmem_nopanic(size, align, goal,
					ARCH_LOW_ADDRESS_LIMIT);
}

/**
 * __alloc_bootmem_low_node - allocate low boot memory from a specific node
 * @pgdat: node to allocate from
 * @size: size of the request in bytes
 * @align: alignment of the region
 * @goal: preferred starting address of the region
 *
 * The goal is dropped if it can not be satisfied and the allocation will
 * fall back to memory below @goal.
 *
 * Allocation may fall back to any node in the system if the specified node
 * can not hold the requested memory.
 *
 * The function panics if the request can not be satisfied.
 */
void * __init __alloc_bootmem_low_node(pg_data_t *pgdat, unsigned long size,
				       unsigned long align, unsigned long goal)
{
	if (WARN_ON_ONCE(slab_is_available()))
		return kzalloc_node(size, GFP_NOWAIT, pgdat->node_id);

	return ___alloc_bootmem_node(pgdat, size, align, goal,
				     ARCH_LOW_ADDRESS_LIMIT);
}
