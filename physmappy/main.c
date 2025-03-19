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

    uint64_t kr = 0;

#define TEST__KCALL_10 0
#if TEST__KCALL_10
    kr = kcall10(ksym(KSYMBOL_RET_1024), (uint64_t []){ }, 0);
    printf("[*] kcall KSYMBOL_RET_1024 kr = %lld\n", kr);
#endif

#define TEST__KVTOPHYS_PHYSTOKV 0
#if TEST__KVTOPHYS_PHYSTOKV
    uint64_t kr2 = kfunc_kvtophys(gKernelBase);
    printf("[*] kr2 kfunc_kvtophys(gKernelBase) = 0x%llx\n", kr2);
    printf("[*] kvtophys(gKernelBase) = 0x%llx\n", kvtophys(gKernelBase));

    uint64_t kr3 = kfunc_phystokv(kr2);
    printf("[*] kr3 kfunc_kvtophys(kr2) = 0x%llx\n", kr3);

    kr3 = phystokv(kr2);
    printf("[*] kr3 phystokv(kr2) = 0x%llx\n", kr3);
#endif

    kr = handoffPPLPrimitives(getpid());
    printf("[*] handoffPPLPrimitives kr = %lld\n", kr);
    // if(kr != 0) {
    //     kextrw_deinit();
    //     exit(1);
    // }

    printf("[*] kread64_via_prw(gKernelBase) = 0x%llx\n", kread64_via_prw(gKernelBase));

    printf("[*] done\n");

    while(1) {};
 
    kextrw_deinit();

    return 0;
}