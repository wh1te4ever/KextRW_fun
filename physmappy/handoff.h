#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PPLRW_USER_MAPPING_OFFSET   0x7000000000
#define PPLRW_USER_MAPPING_TTEP_IDX (PPLRW_USER_MAPPING_OFFSET / 0x1000000000)

#define PERM_KRW_URW 0x7 // R/W for kernel and user
#define FAKE_PHYSPAGE_TO_MAP 0x13370000
#define L2_BLOCK_SIZE 0x2000000
#define L2_BLOCK_PAGECOUNT (L2_BLOCK_SIZE / PAGE_SIZE)
#define L2_BLOCK_MASK (L2_BLOCK_SIZE-1)

int handoffPPLPrimitives(pid_t pid);
int pmap_map_in(uint64_t pmap, uint64_t ua, uint64_t pa, uint64_t size);
void pmap_remove(uint64_t pmap, uint64_t start, uint64_t end);
void pmap_set_type(uint64_t pmap_ptr, uint8_t type);
uint64_t pmap_lv2(uint64_t pmap_ptr, uint64_t virt);
uint64_t pmap_get_ttep(uint64_t pmap_ptr);
