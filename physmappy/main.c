#include "kextrw.h"

#include <stdio.h>
#include "offsets.h"
#include "kfunc.h"
#include "kutils.h"
#include "translation.h"
#include "handoff.h"

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

    uint64_t kr = kcall10(ksym(KSYMBOL_RET_1024), (uint64_t []){ }, 0);
    printf("[*] kcall KSYMBOL_RET_1024 kr = %lld\n", kr);

#if 0
    uint64_t kr2 = kfunc_kvtophys(gKernelBase);
    printf("[*] kr2 kfunc_kvtophys(gKernelBase) = 0x%llx\n", kr2);
    printf("[*] kvtophys(gKernelBase) = 0x%llx\n", kvtophys(gKernelBase));

    uint64_t kr3 = kfunc_phystokv(kr2);
    printf("[*] kr3 kfunc_kvtophys(kr2) = 0x%llx\n", kr3);

    kr3 = phystokv(kr2);
    printf("[*] kr3 phystokv(kr2) = 0x%llx\n", kr3);
#endif

    kr = physrw_handoff(getpid());
    printf("[*] physrw_handoff kr = %lld\n", kr);
    printf("[*] done\n");

    while(1) {};
 
    kextrw_deinit();

    return 0;
}