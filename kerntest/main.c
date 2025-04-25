#include "kextrw.h"

#include <stdio.h>
#include "offsets.h"
#include "kfunc.h"
#include "kutils.h"
#include "kextrw.h"

int main(void) {
    offsets_init();

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

    uint64_t p_ptr = kfunc_kvtophys(gKernelBase);
    printf("ptr: 0x%llx\n", p_ptr);
    printf("ptr 2: 0x%llx\n", kvtophys(gKernelBase));

    uint64_t v_ptr = kfunc_phystokv(p_ptr);
    printf("vptr: 0x%llx\n", v_ptr);


#endif
    // khexdump(ksym(KSYMBOL_RET_1024), 0x50);

    uint64_t kaslr_slide = gKernelBase - VM_KERNEL_LINK_ADDR - 0x8000;
    uint64_t csr_status_data_off = 0xFFFFFE0007CFC378 + kaslr_slide;
    uint64_t csr_check_func_off = 0xFFFFFE0008CCE278 + kaslr_slide;

    uint32_t csr_status_data = kread32(csr_status_data_off);
    printf("[*] csr_status_data: 0x%x\n", csr_status_data);
    // khexdump(csr_check_func_off, 0x100);

    uint64_t p_csr_status_data_off = kvtophys(csr_status_data_off);
    printf("[*] p_csr_status_data_off: 0x%llx\n", p_csr_status_data_off);

    csr_status_data = physread32(p_csr_status_data_off);
    printf("[*] csr_status_data 2: 0x%x\n", csr_status_data);

    // csr_status_data = physread32(kfunc_kvtophys(csr_status_data_off));
    // printf("[*] csr_status_data 2: 0x%x\n", csr_status_data);


    printf("[*] done\n");

//     while(1) {};
 
    kextrw_deinit();

    return 0;
}