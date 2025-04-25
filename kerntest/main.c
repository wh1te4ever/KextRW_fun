#include "kextrw.h"

#include <stdio.h>
#include "offsets.h"
#include "kfunc.h"
#include "kutils.h"
#include "translation.h"
#include "handoff.h"
#include "physrw.h"

int main(void) {
    offsets_init();
    translation_init();

    if (kextrw_init() == -1) {
        printf("[-] Failed to initialize KextRW\n");
        return 1;
    }

    if(get_kernel_base() != 0) {
        printf("[-] Failed get_kernel_base\n");
        return 1;
    }
    printf("[*] gKernelBase = 0x%llx\n", gKernelBase);
    printf("[*] gKernelSlide = 0x%llx\n", gKernelSlide);
    // khexdump(gKernelSlide + 0xFFFFFE0008793E54, 0x100);

    uint64_t kr = 0;

#define TEST__KCALL_10 1
#if TEST__KCALL_10
    // kr = kcall10(ksym(KSYMBOL_RET_1024), (uint64_t []){ }, 0);
    // printf("[*] kcall KSYMBOL_RET_1024 kr = %lld\n", kr);

    uint64_t ptr = kfunc_kvtophys(gKernelBase);
    printf("ptr: 0x%llx\n", ptr);

    uint64_t vptr = phystokv(ptr);
    printf("vptr: 0x%llx\n", vptr);
#endif
    // khexdump(ksym(KSYMBOL_RET_1024), 0x50);

    // uint64_t kaslr_slide = gKernelBase - VM_KERNEL_LINK_ADDR - 0x8000;
    // uint64_t csr_status_data_off = 0xFFFFFE0007CFC378 + kaslr_slide;
    // uint64_t csr_check_func_off = 0xFFFFFE0008CCE278 + kaslr_slide;

    // uint32_t csr_status_data = kread32(csr_status_data_off);
    // printf("[*] csr_status_data: 0x%x\n", csr_status_data);
    // khexdump(csr_check_func_off, 0x100);


    printf("[*] done\n");

//     while(1) {};
 
    kextrw_deinit();

    return 0;
}