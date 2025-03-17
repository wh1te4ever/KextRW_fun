#include <stdint.h>

#define AVM1_24D81_KVERSION "Darwin Kernel Version 24.3.0: Thu Jan  2 20:16:37 PST 2025; root:xnu-11215.81.4~3/RELEASE_ARM64_VMAPPLE"
#define VM_KERNEL_LINK_ADDR 0xFFFFFE0007004000ULL

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
  KSYMBOL_pv_head_table
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
extern uint32_t off_pt_desc_ptd_info;

uint64_t ksym(enum ksymbol sym);
void offsets_init(void);