#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int physrw_handoff(pid_t pid);

#define L1_BLOCK_SIZE 0x1000000000
#define L1_BLOCK_MASK (L1_BLOCK_SIZE-1)
#define L1_BLOCK_COUNT 0x8

#define L2_BLOCK_SIZE 0x2000000
#define L2_BLOCK_MASK (L2_BLOCK_SIZE-1)
#define L2_BLOCK_COUNT 0x800

//https://github.com/apple-oss-distributions/xnu/blob/xnu-8019.41.5/osfmk/arm/pmap/pmap_pt_geometry.h#L169C1-L169C62
#define PMAP_TT_L0_LEVEL    0x0
#define PMAP_TT_L1_LEVEL    0x1
#define PMAP_TT_L2_LEVEL    0x2
#define PMAP_TT_L3_LEVEL    0x3

#define PPLRW_USER_MAPPING_OFFSET   ((L1_BLOCK_SIZE * L1_BLOCK_COUNT) - 0x1000000000)
//L1_BLOCK_SIZE = get_l1_block_size(): 16k device = 0x1000000000, 4k device = 0x40000000;
//L1_BLOCK_COUNT = get_l1_block_count: 16k = 8, 4k = 256;
//if 16k: hex(0x1000000000 * 8 - 0x1000000000) = 0x7000000000;
//if 4k:  hex(0x40000000 * 256 - 0x1000000000) = 0x3000000000;

#define FAKE_PHYSPAGE_TO_MAP 0x13370000

#define PERM_KRW_URW 0x7 // R/W for kernel and user

int pmap_map_in(uint64_t pmap, uint64_t uaStart, uint64_t paStart, uint64_t size);
int pmap_expand_range(uint64_t pmap, uint64_t vaStart, uint64_t size);
void pmap_remove(uint64_t pmap, uint64_t start, uint64_t end);

uint64_t pa_index(uint64_t pa);
uint64_t pai_to_pvh(uint64_t pai);
uint64_t pvh_ptd(uint64_t pvh);