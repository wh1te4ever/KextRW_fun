#include <stdint.h>
#include <mach/mach.h>

uint64_t kfunc_kalloc_external(uint64_t kalloc_sz);
uint64_t kfunc_kalloc_data_external(uint64_t kalloc_sz, uint64_t flags);
uint64_t kfunc_kfree_external(uint64_t kptr, uint64_t kalloc_sz);
uint64_t kfunc_copyout(uint64_t kaddr, uint64_t udaddr, size_t len);
uint64_t kfunc_kwrite64(uint64_t kaddr, uint64_t val);
uint64_t kfunc_kvtophys(uint64_t va);
uint64_t kfunc_phystokv(uint64_t pa);
kern_return_t kfunc_pmap_enter_options_addr(uint64_t pmap, uint64_t pa, uint64_t va);
uint64_t kfunc_pmap_remove(uint64_t pmap, uint64_t start, uint64_t end);
uint64_t kfunc_pmap_find_pa(uint64_t pmap, uint64_t va);