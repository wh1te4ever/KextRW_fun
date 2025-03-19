#include "handoff.h"
#include "kextrw.h"
#include "kutils.h"
#include "offsets.h"
#include "translation.h"
#include "physrw.h"
#include "kfunc.h"
#include "pte.h"
#include <errno.h>


int handoffPPLPrimitives(pid_t pid)
{
	if (!pid) return -1;

	int ret = 0;

	uint64_t proc = proc_of_pid(pid);
	uint64_t p_proc_ro = kread64(proc + off_p_proc_ro);
	if (p_proc_ro) {
		uint64_t pr_task = kread64(p_proc_ro + off_p_ro_pr_task);
		if (pr_task) {
			uint64_t vmMap = kreadptr(pr_task + off_task_map);
			if (vmMap) {
				uint64_t pmap = kreadptr(vmMap + off_vm_map_pmap);
				if (pmap) {
					// printf("pmap: 0x%llx\n", pmap); sleep(1);
					// printf("ptr: 0x%llx\n", phystokv(pmap_get_ttep(pmap))); 
					// exit(1);
					uint64_t existingLevel1Entry = kread64(phystokv(pmap_get_ttep(pmap)) + (8 * PPLRW_USER_MAPPING_TTEP_IDX));
					// If there is an existing level 1 entry, we assume the process already has PPLRW primitives
					// Normally there cannot be mappings above 0x3D6000000, so this assumption should always be true
					// If we would try to handoff PPLRW twice, the second time would cause a panic because the mapping already exists
					// So this check protects the device from kernel panics, by not adding the mapping if the process already has it
					if (existingLevel1Entry == 0)
					{
						// Map the entire kernel physical address space into the userland process, starting at PPLRW_USER_MAPPING_OFFSET
						uint64_t physBase = kread64(ksym(KSYMBOL_gPhysBase));
						uint64_t physSize = kread64(ksym(KSYMBOL_gPhysSize));
						ret = pmap_map_in(pmap, physBase+PPLRW_USER_MAPPING_OFFSET, physBase, physSize);
					}
				}
				else { ret = -5; }
			}
			else { ret = -4; }
		}
		else { ret = -3; }
	}
	else { ret = -2; }

	return ret;
}

int pmap_map_in(uint64_t pmap, uint64_t ua, uint64_t pa, uint64_t size)
{
	uint64_t mappingUaddr = ua & ~L2_BLOCK_MASK;
	uint64_t mappingPA = pa & ~L2_BLOCK_MASK;

	uint64_t endPA = pa + size;
	uint64_t mappingEndPA = endPA & ~L2_BLOCK_MASK;

	uint64_t l2Count = ((mappingEndPA - mappingPA) / L2_BLOCK_SIZE) + 1;

	for (uint64_t i = 0; i < l2Count; i++) {
		uint64_t curMappingUaddr = mappingUaddr + (i * L2_BLOCK_SIZE);
		kern_return_t kr = kfunc_pmap_enter_options_addr(pmap, FAKE_PHYSPAGE_TO_MAP, curMappingUaddr);
		if (kr != KERN_SUCCESS) {
			pmap_remove(pmap, mappingUaddr, curMappingUaddr);
			return -7;
		}
	}

	// Temporarily change pmap type to nested
	pmap_set_type(pmap, 3);
	// Remove mapping (table will not be removed because we changed the pmap type, but IT REMOVED!!! idk why,,,, If you disabled this code, I could get value table2Entry in pmap_lv2 function)
	pmap_remove(pmap, mappingUaddr, mappingUaddr + (l2Count * L2_BLOCK_SIZE));
	// Change type back
	pmap_set_type(pmap, 0);

	// printf("Work???\n");
	// sleep(1);

	for (uint64_t i = 0; i < l2Count; i++) {
		uint64_t curMappingUaddr = mappingUaddr + (i * L2_BLOCK_SIZE);
		uint64_t curMappingPA = mappingPA + (i * L2_BLOCK_SIZE);

		// Create full table for this mapping
		uint64_t tableToWrite[2048];
		for (int k = 0; k < 2048; k++) {
			uint64_t curMappingPage = curMappingPA + (k * 0x4000);
			if (curMappingPage >= pa || curMappingPage < (pa + size)) {
				tableToWrite[k] = curMappingPage | PERM_TO_PTE(PERM_KRW_URW) | PTE_NON_GLOBAL | PTE_OUTER_SHAREABLE | PTE_LEVEL3_ENTRY;
			}
			else {
				tableToWrite[k] = 0;
			}
		}

		// Replace table with the entries we generated
		uint64_t table2Entry = pmap_lv2(pmap, curMappingUaddr);
		if ((table2Entry & 0x3) == 0x3) {
			uint64_t table3 = table2Entry & 0xFFFFFFFFC000ULL;
			physwritebuf_via_krw(table3, tableToWrite, 0x4000);
		}
		else {
			return -6;
		}
	}

	return 0;
}

void pmap_remove(uint64_t pmap, uint64_t start, uint64_t end)
{
	// kfunc_pmap_remove(pmap, start, end);
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
        remove_count = kfunc_pmap_remove(pmap, va, l);
        va = remove_count;
    }
}

void pmap_set_type(uint64_t pmap_ptr, uint8_t type)
{
	kwrite8(pmap_ptr + off_pmap_type, type);
}

uint64_t pmap_lv2(uint64_t pmap_ptr, uint64_t virt)
{
	uint64_t ttep = pmap_get_ttep(pmap_ptr);
	
	uint64_t table1Off   = (virt >> 36ULL) & 0x7ULL;
	uint64_t table1Entry = physread64_via_krw(ttep + (8ULL * table1Off));
	if ((table1Entry & 0x3) != 3) {
		return 0;
	}
	khexdump(phystokv(ttep), 0x1000);
	printf("table1Entry = 0x%llx\n", table1Entry);
	
	uint64_t table2 = table1Entry & 0xFFFFFFFFC000ULL;
	uint64_t table2Off = (virt >> 25ULL) & 0x7FFULL;
	uint64_t table2Entry = physread64_via_krw(table2 + (8ULL * table2Off));
	khexdump(phystokv(table2), 0x1000); 
	printf("table2Entry: 0x%llx\n", table2Entry);
	
	
	return table2Entry;
}

uint64_t pmap_get_ttep(uint64_t pmap_ptr)
{
	return kread64(pmap_ptr + off_pmap_ttep);
}