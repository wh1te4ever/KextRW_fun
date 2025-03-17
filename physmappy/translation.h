#include <stdint.h>
#include <mach/mach.h>

#define vm_real_kernel_page_size 0x4000

uint64_t phystokv(uint64_t pa);
uint64_t vtophys_lvl(uint64_t tte_ttep, uint64_t va, uint64_t *leaf_level, uint64_t *leaf_tte_ttep);
uint64_t vtophys(uint64_t tte_ttep, uint64_t va);
uint64_t kvtophys(uint64_t va);
void translation_init(void);