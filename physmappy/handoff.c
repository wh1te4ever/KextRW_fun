#include "handoff.h"
#include "kextrw.h"
#include "kutils.h"
#include "offsets.h"
#include "translation.h"
#include "physrw.h"
#include "kfunc.h"
#include "pvh.h"
#include "pte.h"
#include <errno.h>

int physrw_handoff(pid_t pid)
{
	if (!pid) return -1;

	uint64_t proc = proc_of_pid(pid);
	if (!proc) return -2;

	int ret = 0;
	do {
        uint64_t proc = proc_of_pid(pid);
        uint64_t p_proc_ro = kread64(proc + off_p_proc_ro);
        uint64_t pr_task = kread64(p_proc_ro + off_p_ro_pr_task);
        printf("[*] pr_task = 0x%llx\n", pr_task);
        
		if (!pr_task) { ret = -3; break; };

		uint64_t vmMap = kreadptr(pr_task + off_task_map);
        printf("[*] vmMap = 0x%llx\n", vmMap);
		if (!vmMap) { ret = -4; break; };

		uint64_t pmap = kreadptr(vmMap + off_vm_map_pmap);
        printf("[*] pmap = 0x%llx\n", pmap);
		if (!pmap) { ret = -5; break; };

        uint64_t gPhysBase = kread64(ksym(KSYMBOL_gPhysBase));
        uint64_t gPhysSize = kread64(ksym(KSYMBOL_gPhysSize));
        printf("[*] PPLRW_USER_MAPPING_OFFSET = 0x%llx, gPhysBase = 0x%llx, gPhysSize = 0x%llx\n", PPLRW_USER_MAPPING_OFFSET, gPhysBase, gPhysSize);

		// Map the entire kernel physical address space into the userland process, starting at PPLRW_USER_MAPPING_OFFSET
		int mapInRet = pmap_map_in(pmap, gPhysBase+PPLRW_USER_MAPPING_OFFSET, gPhysBase, gPhysSize);
		if (mapInRet != 0) ret = -10 + mapInRet;
	} while (0);

	return ret;
}

#define USE_KCALL 1
int pmap_map_in(uint64_t pmap, uint64_t uaStart, uint64_t paStart, uint64_t size)
{
	uint64_t ttep = kread64(pmap + off_pmap_ttep);
    printf("[*] ttep = 0x%llx\n", ttep);

	uint64_t paEnd = paStart + size;
	uint64_t uaEnd = uaStart + size;

	uint64_t uaL2Start = uaStart & ~L2_BLOCK_MASK;
	uint64_t uaL2End   = ((uaStart + size - 1) + L2_BLOCK_SIZE) & ~L2_BLOCK_MASK;

	uint64_t paL2Start = paStart & ~L2_BLOCK_MASK;
	uint64_t l2Count = (((uaL2End - uaL2Start) - 1) / L2_BLOCK_SIZE) + 1;

	// Sanity check: Ensure the entire area to be mapped in is not mapped to anything yet
	for(uint64_t ua = uaStart; ua < uaEnd; ua += vm_real_kernel_page_size) {
		uint64_t leafLevel = PMAP_TT_L3_LEVEL;
		if (vtophys_lvl(ttep, ua, &leafLevel, NULL) != 0) {
			return -1;
		}
		else {
			// Performance improvement
			// If there is no L1 / L2 mapping we can skip a whole bunch of addresses
			if (leafLevel == PMAP_TT_L1_LEVEL) {
				ua = (((ua + L1_BLOCK_SIZE) & ~L1_BLOCK_MASK) - vm_real_kernel_page_size);
			}
			else if (leafLevel == PMAP_TT_L2_LEVEL) {
				ua = (((ua + L2_BLOCK_SIZE) & ~L2_BLOCK_MASK) - vm_real_kernel_page_size);
			}
		}

		if (vtophys(ttep, ua)) return -1;
		// TODO: If all mappings match 1:1, maybe return 0 instead of -1?
	}
	// Allocate all page tables that need to be allocated
	if (pmap_expand_range(pmap, uaStart, size) != 0) return -1;
	
	// Insert entries into L3 pages
	for (uint64_t i = 0; i < l2Count; i++) {
		uint64_t uaL2Cur = uaL2Start + (i * L2_BLOCK_SIZE);
		uint64_t paL2Cur = paL2Start + (i * L2_BLOCK_SIZE);

		// Create full table for this mapping
		uint64_t tableToWrite[L2_BLOCK_COUNT];
		for (int k = 0; k < L2_BLOCK_COUNT; k++) {
			uint64_t curMappingPage = paL2Cur + (k * vm_real_kernel_page_size);
			if (curMappingPage >= paStart && curMappingPage < paEnd) {
				tableToWrite[k] = curMappingPage | PERM_TO_PTE(PERM_KRW_URW) | PTE_NON_GLOBAL | PTE_OUTER_SHAREABLE | PTE_LEVEL3_ENTRY;
			}
			else {
				tableToWrite[k] = 0;
			}
		}

		// Replace table with the entries we generated
		uint64_t leafLevel = PMAP_TT_L2_LEVEL;
		// printf("leafLevel: 0x%llx, uaL2Cur: 0x%llx, ttep: 0x%llx, errno = %d\n", leafLevel, uaL2Cur, ttep, errno);
		uint64_t level2Table = vtophys_lvl(ttep, uaL2Cur, &leafLevel, NULL);
        // printf("level2Table: 0x%llx, uaL2Cur: 0x%llx, ttep: 0x%llx, errno = %d\n", level2Table, uaL2Cur, ttep, errno);
		// sleep(1);
		// return -3;
		if (!level2Table) return -2;
		physwritebuf_via_krw(level2Table, tableToWrite, vm_real_kernel_page_size);

        // Reference count of new page table must be 0!
	    // original ref count is 1 because the table holds one PTE
	    // Our new PTEs are not part of the pmap layer though so refcount needs to be 0
#if USE_KCALL
        uint64_t pvh = pai_to_pvh(pa_index(level2Table));
		uint64_t ptdp = pvh_ptd(pvh);
        printf("ptdp = 0x%llx, off_pt_desc_ptd_info = 0x%x\n", ptdp, off_pt_desc_ptd_info);

		uint16_t pinfo_refCount = kread16(ptdp + off_pt_desc_ptd_info);
        printf("pinfo_refCount = 0x%hx\n", pinfo_refCount);

        kwrite16(ptdp + off_pt_desc_ptd_info, 0);  
#endif
	}

	return 0;
}

int pmap_expand_range(uint64_t pmap, uint64_t vaStart, uint64_t size)
{
	uint64_t ttep = kread64(pmap + off_pmap_ttep);

#if USE_KCALL	
	uint64_t unmappedStart = 0, unmappedSize = 0;

	uint64_t l2Start = vaStart & ~L2_BLOCK_MASK;
	uint64_t l2End = (vaStart + (size - 1)) & ~L2_BLOCK_MASK;
	uint64_t l2Count = ((l2End - l2Start) / L2_BLOCK_SIZE) + 1;

	for (uint64_t i = 0; i <= l2Count; i++) {
		uint64_t curL2 = l2Start + (i * L2_BLOCK_SIZE);

		uint64_t leafLevel = PMAP_TT_L3_LEVEL;
		uint64_t pt3 = 0;
		vtophys_lvl(ttep, curL2, &leafLevel, &pt3);
		if (leafLevel == PMAP_TT_L3_LEVEL || i == l2Count) {
			// i == l2Count: one extra cycle that this for loop takes
			// We hit this block either if there was a mapping or at the end
			// Alloc page tables for the current area (unmappedStart, unmappedSize) by running pmap_enter_options on every page
			// And then running pmap_remove on the entire area while nested is true
			for (uint64_t l2Off = 0; l2Off < unmappedSize; l2Off += L2_BLOCK_SIZE) {
				kern_return_t kr = kfunc_pmap_enter_options_addr(pmap, FAKE_PHYSPAGE_TO_MAP, unmappedStart + l2Off);
                printf("kfunc_pmap_enter_options_addr kr: 0x%llx\n", kr);
				if (kr != KERN_SUCCESS) {
					return -7;
				}
			}

			// Set type to nested
            printf("kread8 pmap + off_pmap_type: 0x%x\n", kread8(pmap + off_pmap_type));
			physwrite8_via_krw(kvtophys(pmap + off_pmap_type), 3);

			printf("pmap_remove...?\n");
			// sleep(1);

			// Remove mapping (table will stay cause nested is set)
            printf("pmap: 0x%llx, unmappedStart: 0x%llx, unmappedSize:0x%llx\n", pmap, unmappedStart, unmappedSize);
			pmap_remove(pmap, unmappedStart, unmappedStart + unmappedSize);

			printf("pmap_remove... done\n"); 
			// sleep(1);

			// Change type back
			physwrite8_via_krw(kvtophys(pmap + off_pmap_type), 0);

			unmappedStart = 0;
			unmappedSize = 0;
			continue;
		}
		else {
			if (unmappedStart == 0) { 
				unmappedStart = curL2;
			}
			unmappedSize += L2_BLOCK_SIZE;
		}
	}
#else
    uint64_t vaEnd = vaStart + size;
	for (uint64_t va = vaStart; va < vaEnd; va += vm_real_kernel_page_size) {
		uint64_t leafLevel;
		do {
			leafLevel = PMAP_TT_L3_LEVEL;
			uint64_t pt = 0;
			vtophys_lvl(ttep, va, &leafLevel, &pt);
			if (leafLevel != PMAP_TT_L3_LEVEL) {
				uint64_t pt_va = 0;
				switch (leafLevel) {
					case PMAP_TT_L1_LEVEL: {
						pt_va = va & ~L1_BLOCK_MASK;
						break;
					}
					case PMAP_TT_L2_LEVEL: {
						pt_va = va & ~L2_BLOCK_MASK;
						break;
					}
				}
				uint64_t newTable = pmap_alloc_page_table(pmap, pt_va);
                printf("pt: 0x%llx, leafLevel: 0x%llx, newTable: 0x%llx\n", pt, leafLevel, newTable);
				if (newTable) {
                    if(leafLevel == PMAP_TT_L1_LEVEL)
                        physwrite64_via_krw(pt, newTable | ARM_TTE_VALID | ARM_TTE_TYPE_TABLE);
                        // physwrite64_via_krw(pt, newTable | ARM_TTE_VALID | ARM_TTE_TYPE_TABLE | ARM_TTE_TABLE_AP(ARM_TTE_TABLE_AP_USER_NA));
                    else if(leafLevel == PMAP_TT_L2_LEVEL)
                        physwrite64_via_krw(pt, newTable | ARM_TTE_VALID | ARM_TTE_TYPE_TABLE);
				}
				else {
					return -2;
				}
			}
		} while (leafLevel < PMAP_TT_L3_LEVEL);
	}
#endif
	return 0;
}

uint64_t pmap_alloc_page_table(uint64_t pmap, uint64_t va)
{
	if (!pmap) {
		pmap = pmap_self();
	}

	uint64_t tt_p = alloc_page_table_unassigned();
	if (!tt_p) return 0;

	uint64_t pvh = pai_to_pvh(pa_index(tt_p));
	uint64_t ptdp = pvh_ptd(pvh);
    // printf("pmap_alloc_page_table ptdp: 0x%llx\n", ptdp);

	uint64_t ptdp_pa = kvtophys(ptdp);
    // printf("pmap_alloc_page_table ptdp_pa: 0x%llx\n", ptdp_pa);

	// At this point the allocated page table is associated
	// to the pmap of this process alongside the address it was allocated on
	// We now need to replace the association with the context in which it will be used
	physwrite64_via_krw(ptdp_pa + off_pt_desc_pmap, pmap);

	// On A14+ PT_INDEX_MAX is 4, for whatever reason
	// However in practice, only the first slot is used...
	for (uint64_t po = 0; po < vm_page_size; po += vm_real_kernel_page_size) {
		physwrite64_via_krw(ptdp_pa + off_pt_desc_va + (po / vm_page_size), va + po);
	}

	return tt_p;
}

void pmap_remove(uint64_t pmap, uint64_t start, uint64_t end)
{
	kfunc_pmap_remove_options(pmap, start, end);
    // uint64_t remove_count = 0;
    // if (!pmap) {
    //     return;
    // }
    // uint64_t va = start;
    // while (va < end) {
    //     uint64_t l;
    //     l = ((va + L2_BLOCK_SIZE) & ~L2_BLOCK_MASK);
    //     if (l > end) {
    //         l = end;
    //     }
    //     remove_count = kfunc_pmap_remove_options(pmap, va, l);
    //     // printf("remove_count: 0x%llx, pmap: 0x%llx, va: 0x%llx, l: 0x%llx\n", remove_count, pmap, va, l);
    //     va = remove_count;
    // }
}

uint64_t alloc_page_table_unassigned(void)
{
	uint64_t pmap = pmap_self();
	uint64_t ttep = kread64(pmap + off_pmap_ttep);

	// printf("alloc_page_table_unassigned pmap: 0x%llx, ttep: 0x%llx\n", pmap, ttep);

	// sleep(1);

	void *free_lvl2 = NULL;
	uint64_t tte_lvl2 = 0;
	uint64_t allocatedPT = 0;
	uint64_t pinfo_pa = 0;
	while (1) {
		// When we allocate the entire address range of an L2 block, we can assume ownership of the backing table
		if (posix_memalign(&free_lvl2, L2_BLOCK_SIZE, L2_BLOCK_SIZE) != 0) {
			printf("WARNING: Failed to allocate L2 page table address range\n");
			return 0;
		}
		// Now, fault in one page to make the kernel allocate the page table for it
		*(volatile uint64_t *)free_lvl2;

		// Find the newly allocated page table
		uint64_t lvl = PMAP_TT_L2_LEVEL;
		// printf("HUH, free_lvl2 = 0x%llx\n", (uint64_t)free_lvl2);
		// sleep(1);
		allocatedPT = vtophys_lvl(ttep, (uint64_t)free_lvl2, &lvl, &tte_lvl2);
		// printf("allocatedPT: 0x%llx\n", allocatedPT);
		// sleep(1);

		uint64_t pvh = pai_to_pvh(pa_index(allocatedPT));
		// printf("pvh: 0x%llx\n", pvh);
        // khexdump(pvh, 0x100); //0x80effdf1e7ab1a83
		// sleep(1);
		uint64_t ptdp = pvh_ptd(pvh);
		// printf("ptdp: 0x%llx\n", ptdp);
		// sleep(1);
        // khexdump(ptdp, 0x100);
        // printf("off_pt_desc_ptd_info = 0x%x\n", off_pt_desc_ptd_info);
		uint64_t pinfo = kread64(ptdp + off_pt_desc_ptd_info);
        
		// printf("alloc_page_table_unassigned pinfo: 0x%llx\n", pinfo);
		// sleep(1);
		pinfo_pa = kvtophys(pinfo);
        // printf("pinfo_pa: 0x%llx\n", pinfo_pa);
        // if(pinfo_pa == 0) exit(1);
        // sleep(1);

		uint16_t refCount = physread16_via_krw(pinfo_pa);
        // printf("refCount: 0x%llx\n", refCount);
		if (refCount != 1) {
			// Something is off, retry
			free(free_lvl2);
			continue;
		}
		break;
	}

	// Handle case where all entries in the level 2 table are 0 after we leak ours
	// In that case, leak an allocation in the span of it to keep it alive
	/*uint64_t lvl2Table = tte_lvl2 & ~PAGE_MASK;
	uint64_t lvl2TableEntries[PAGE_SIZE / sizeof(uint64_t)];
	physreadbuf(lvl2Table, lvl2TableEntries, PAGE_SIZE);
	int freeIdx = -1;
	for (int i = 0; i < (PAGE_SIZE / sizeof(uint64_t)); i++) {
		uint64_t curPtr = lvl2Table + (sizeof(uint64_t) * i);
		if (curPtr != tte_lvl2) {
			if (lvl2TableEntries[i]) {
				freeIdx = -1;
				break;
			}
			else {
				freeIdx = i;
			}
		}
	}
	if (freeIdx != -1) {
		vm_address_t freeUserspace = ((uint64_t)free_lvl2 & ~L1_BLOCK_MASK) + (freeIdx * L2_BLOCK_SIZE);
		if (vm_allocate(mach_task_self(), &freeUserspace, 0x4000, VM_FLAGS_FIXED) == 0) {
			*(volatile uint8_t *)freeUserspace;
		}
	}*/

	// Bump reference count of our allocated page table
	physwrite16_via_krw(pinfo_pa, 0x1337);

	// Deallocate address range (our allocated page table will stay because we bumped it's reference count)
	free(free_lvl2);

	// Remove our allocated page table from it's original location (leak it)
	physwrite64_via_krw(tte_lvl2, 0);

	// Ensure there is at least one entry in page table
	// Attempts to prevent "pte is empty" panic
	// Sometimes weird prefetches happen so this has to be a valid physical page to ensure those don't panic
	// Disabled for now cause it causes super weird issues
	// physwrite64_via_krw(allocatedPT, kread64(KSYMBOL_gPhysBase) | PERM_TO_PTE(PERM_KRW_URW) | PTE_NON_GLOBAL | PTE_OUTER_SHAREABLE | PTE_LEVEL3_ENTRY);

	// Reference count of new page table must be 0!
	// original ref count is 1 because the table holds one PTE
	// Our new PTEs are not part of the pmap layer though so refcount needs to be 0
	physwrite16_via_krw(pinfo_pa, 0);

	// After we leaked the page table, the ledger still thinks it belongs to our process
	// We need to remove it from there aswell so that the process doesn't get jetsam killed
	// (This ended up more complicated than I thought, so I just disabled jetsam in launchd)
	//uint64_t ledger = kread_ptr(pmap + koffsetof(pmap, ledger));
	//uint64_t ledger_pa = kvtophys(ledger);
	//int page_table_ledger = physread32(ledger_pa + koffsetof(_task_ledger_indices, page_table));
	//physwrite32(ledger_pa + koffsetof(_task_ledger_indices, page_table), page_table_ledger - 1);

	return allocatedPT;
}

uint64_t pmap_self(void)
{
	uint64_t proc = proc_of_pid(getpid());
    uint64_t p_proc_ro = kread64(proc + off_p_proc_ro);
    uint64_t pr_task = kread64(p_proc_ro + off_p_ro_pr_task);
	uint64_t vmMap = kreadptr(pr_task + off_task_map);
    uint64_t pmap = kreadptr(vmMap + off_vm_map_pmap);
	
	return pmap;
}

#define atop(x) ((vm_address_t)(x) >> vm_kernel_page_shift)
uint64_t pa_index(uint64_t pa)
{
    // printf("kread64(ksym(KSYMBOL_vm_first_phys)): 0x%llx\n", kread64(ksym(KSYMBOL_vm_first_phys)));
	return atop(pa - kread64(ksym(KSYMBOL_vm_first_phys)));
}

uint64_t pai_to_pvh(uint64_t pai)
{
    // khexdump(ksym(KSYMBOL_pv_head_table), 0x100);
	return kread64(ksym(KSYMBOL_pv_head_table)) + (pai * 8);
}

uint64_t pvh_ptd(uint64_t pvh)
{
    uint64_t tmp = kreadptr(pvh);
    // printf("tmp: 0x%llx, tmp2: 0x%llx\n", tmp, kread64(pvh));
	return ((kreadptr(pvh) & PVH_LIST_MASK) | PVH_HIGH_FLAGS);
}