#include "kfunc.h"
#include "kextrw.h"
#include "offsets.h"
#include <stdio.h>

uint64_t kfunc_kvtophys(uint64_t va) {
    uint64_t kr = kcall10(ksym(KSYMBOL_kvtophys), (uint64_t []){ va }, 1);
    return kr;
}

uint64_t kfunc_phystokv(uint64_t pa) {
    uint64_t kr = kcall10(ksym(KSYMBOL_phystokv), (uint64_t []){ pa }, 1);
    return kr;
}

kern_return_t kfunc_pmap_enter_options_addr(uint64_t pmap, uint64_t pa, uint64_t va)
{
	while (1) {
        // uint64_t kret = kcall8(ksym(KSYMBOL_pmap_enter_options_addr), pmap, va, pa, VM_PROT_READ | VM_PROT_WRITE, 0, 0, 1, 1);
        uint64_t kr = kcall10(ksym(KSYMBOL_pmap_enter_options_addr), (uint64_t []){ pmap, va, pa, VM_PROT_READ | VM_PROT_WRITE, 0, 0, 1, 1, 0 }, 9);
		if (kr != KERN_RESOURCE_SHORTAGE) {
			return kr;
		} 
	}
}

uint64_t kfunc_pmap_remove_options(uint64_t pmap, uint64_t start, uint64_t end)
{ 
    uint64_t kr = kcall10(ksym(KSYMBOL_pmap_remove_options), (uint64_t []){ pmap, start, end, 0x100 }, 4);
    // uint64_t val = 0x4141414141414141;
    // uint64_t kr = kcall10(ksym(KSYMBOL_pmap_remove_options), (uint64_t []){ val, val+1, val+2, val+3, val+4, val+5, val+6, val+7, val+8, val+9 }, 10);
	return kr;
}

uint64_t kfunc_pmap_find_pa(uint64_t pmap, uint64_t va)
{ 
    uint64_t kr = kcall10(ksym(KSYMBOL_pmap_find_pa), (uint64_t []){ pmap, va }, 2);
	return kr;
}