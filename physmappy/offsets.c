#include "offsets.h"
#include "kextrw.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <sys/sysctl.h>

uint64_t *gadgets = NULL;
uint64_t *symbols = NULL;
uint64_t kaslr_slide = 0;

uint64_t ksymbols_avm1_24d81[] = {
    0xFFFFFE00077F4EE0, // KSYMBOL_KERNPROC
    0xFFFFFE0007D5C648, // KSYMBOL_RET_1024
    0xFFFFFE0007ECC81C, // KSYMBOL_phystokv
    0xFFFFFE0007EB8174, // KSYMBOL_kvtophys
    0xFFFFFE00076E1E38, // KSYMBOL_ptov_table
    0xFFFFFE00077A15D0, // KSYMBOL_gPhysBase
    0xFFFFFE00077A15D8, // KSYMBOL_gPhysSize
    0xFFFFFE000779DB28, // KSYMBOL_gVirtBase
    0xFFFFFE000776C010, // KSYMBOL_cpu_ttep
    0xFFFFFE0007EB5944, // KSYMBOL_pmap_enter_options_addr
    0xFFFFFE0007EBBAEC, // KSYMBOL_pmap_remove_options
    // 0xFFFFFE0008FA7300, //KSYMBOL_pmap_remove_options
    0xFFFFFE000776C158, // KSYMBOL_vm_first_phys
    0xFFFFFE00076E1910, // KSYMBOL_pv_head_table
    0xFFFFFE0007EBEE44, // KSYMBOL_pmap_find_pa
};

uint64_t ksym(enum ksymbol sym)
{
    kaslr_slide = gKernelBase - VM_KERNEL_LINK_ADDR - 0x8000;

    return symbols[sym] + kaslr_slide;
}

uint32_t off_p_pid = 0;
uint32_t off_p_list_le_prev = 0;
uint32_t off_p_proc_ro = 0;
uint32_t off_p_ro_pr_proc = 0;
uint32_t off_p_ro_pr_task = 0;
uint32_t off_task_map = 0;
uint32_t off_vm_map_pmap = 0;
uint32_t off_pmap_ttep = 0;
uint32_t off_pmap_type = 0;
uint32_t off_pt_desc_pmap = 0;
uint32_t off_pt_desc_va = 0;
uint32_t off_pt_desc_ptd_info = 0;

void offsets_init(void) {
    char kern_version[512] = {};
    size_t size = sizeof(kern_version);
    sysctlbyname("kern.version", &kern_version, &size, NULL, 0);

    if (strcmp(kern_version, AVM1_24D81_KVERSION) != 0) {
        printf("[-] Only supported kernel for %s\n", AVM1_24D81_KVERSION);
        exit(EXIT_FAILURE);
    }

    if (strcmp(kern_version, AVM1_24D81_KVERSION) == 0) {
        off_p_pid = 0x60;
        off_p_list_le_prev = 0x8;
        off_p_proc_ro = 0x18;

        off_p_ro_pr_proc = 0;
        off_p_ro_pr_task = 0x8;

        off_task_map = 0x28;

        off_vm_map_pmap = 0x40;

        off_pmap_ttep = 0x8;
        off_pmap_type = 0xAA;

        off_pt_desc_pmap = 0x10;
        off_pt_desc_va = 0x18;
        off_pt_desc_ptd_info = off_pt_desc_pmap + (/*kconstant(PT_INDEX_MAX)*/ 4 * sizeof(uint64_t)); 

        symbols = ksymbols_avm1_24d81;
    }
}