#include <stdint.h>
#include <mach/mach.h>

uint64_t gKernelBase; 
uint64_t gKernelSlide;

kern_return_t kextrw_init(void);
void kextrw_deinit(void);

int get_kernel_base(void);

kern_return_t
kreadbuf(uint64_t kaddr, void *buf, size_t sz);
kern_return_t
kwritebuf(uint64_t kaddr, const void *buf, size_t sz);

uint16_t kread16(uint64_t where);
uint32_t kread32(uint64_t where);
uint64_t kread64(uint64_t where);
uint64_t kreadptr(uint64_t where);
void kwrite16(uint64_t where, uint16_t what);
void kwrite32(uint64_t where, uint32_t what);
void kwrite64(uint64_t where, uint64_t what);

void kmemcpy(uint64_t dest, uint64_t src, uint32_t length);
void khexdump(uint64_t addr, size_t size);

uint64_t kcall10(uint64_t fn, uint64_t *args, uint32_t argsCnt);