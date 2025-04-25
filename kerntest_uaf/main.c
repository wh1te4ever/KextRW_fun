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
    // khexdump(gKernelBase, 0x100);

    uint64_t kr = 0;

#define TEST__KCALL_10 1
#if TEST__KCALL_10
    kr = kcall10(ksym(KSYMBOL_RET_1024), (uint64_t []){ }, 0);
    printf("[*] kcall KSYMBOL_RET_1024 kr = %lld\n", kr);
#endif


    uint64_t kptr = kfunc_kalloc_external(0x40);
    // kptr = xpaci(kptr);
    printf("kalloc(0x100)ed kptr: 0x%llx\n", kptr);


    uint64_t val = 0x4142434445464749;
    kwrite64(kptr, val);
    kfunc_kwrite64(kptr, val);
    khexdump(kptr, 0x40);
    khexdump(xpaci(kptr), 0x40);

    kfunc_kfree_external(kptr, 0x40);

    kfunc_kwrite64(kptr, val);
    khexdump(kptr, 8);

    printf("[*] done\n");

//     while(1) {};
 
    kextrw_deinit();

    return 0;
}