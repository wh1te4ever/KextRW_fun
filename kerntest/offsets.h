#include <stdint.h>

#define T8122_24E263_KVERSION "Darwin Kernel Version 24.4.0: Fri Apr 11 18:34:14 PDT 2025; root:xnu-11417.101.15~117/RELEASE_ARM64_T8122"
#define AVM1_24D81_KVERSION "Darwin Kernel Version 24.3.0: Thu Jan  2 20:16:37 PST 2025; root:xnu-11215.81.4~3/RELEASE_ARM64_VMAPPLE"
#define AVM1_22D68_KVERSION "Darwin Kernel Version 22.3.0: Mon Jan 30 20:37:48 PST 2023; root:xnu-8792.81.3~2/RELEASE_ARM64_VMAPPLE"
#define AVM1_22C65_KVERSION "Darwin Kernel Version 22.2.0: Fri Nov 11 02:07:56 PST 2022; root:xnu-8792.61.2~4/RELEASE_ARM64_VMAPPLE"
#define VM_KERNEL_LINK_ADDR 0xFFFFFE0007004000ULL

enum kmajor {
  KVERSION_22_2,
  KVERSION_22_3,
  KVERSION_24,
};

enum ksymbol {
  KSYMBOL_KERNPROC,
  KSYMBOL_RET_1024,
  KSYMBOL_phystokv,
  KSYMBOL_kvtophys,
  KSYMBOL_ptov_table,
  KSYMBOL_gPhysBase,
  KSYMBOL_gPhysSize,
  KSYMBOL_gVirtBase,
  KSYMBOL_cpu_ttep,
  KSYMBOL_pmap_enter_options_addr,
  KSYMBOL_pmap_remove_options,
  KSYMBOL_vm_first_phys,
  KSYMBOL_pv_head_table,
  KSYMBOL_pmap_find_pa
};

extern uint32_t off_p_pid;
extern uint32_t off_p_list_le_prev;
extern uint32_t off_p_proc_ro;
extern uint32_t off_p_ro_pr_proc;
extern uint32_t off_p_ro_pr_task;
extern uint32_t off_task_map;
extern uint32_t off_vm_map_pmap;
extern uint32_t off_pmap_ttep;
extern uint32_t off_pmap_type;
extern uint32_t off_pt_desc_pmap;
extern uint32_t off_pt_desc_va;
extern uint32_t off_pt_desc_ptd_info;

uint64_t ksym(enum ksymbol sym);
void offsets_init(void);