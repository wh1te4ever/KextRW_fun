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

uint8_t kread8(uint64_t where);
uint16_t kread16(uint64_t where);
uint32_t kread32(uint64_t where);
uint64_t kread64(uint64_t where);
uint64_t kreadptr(uint64_t where);
void kwrite8(uint64_t where, uint8_t what);
void kwrite16(uint64_t where, uint16_t what);
void kwrite32(uint64_t where, uint32_t what);
void kwrite64(uint64_t where, uint64_t what);

void kmemcpy(uint64_t dest, uint64_t src, uint32_t length);
void khexdump(uint64_t addr, size_t size);

uint64_t kcall10(uint64_t fn, uint64_t *args, uint32_t argsCnt);


uint8_t physread8(uint64_t addr);
uint16_t physread16(uint64_t addr);
uint32_t physread32(uint64_t addr);
uint64_t physread64(uint64_t addr);
int physreadbuf(uint64_t addr, void *buf, size_t len);

void physwrite8(uint64_t addr, uint8_t val);
void physwrite16(uint64_t addr, uint16_t val);
void physwrite32(uint64_t addr, uint32_t val);
void physwrite64(uint64_t addr, uint64_t val);
int physwritebuf(uint64_t addr, void *buf, size_t len);

uint64_t kvtophys(uint64_t va);
uint64_t phystokv(uint64_t pa);

uint64_t kalloc(uint64_t size);
void kfree(uint64_t addr, uint64_t size);

uint64_t xpaci(uint64_t pointer);
uint64_t kreadptr(uint64_t addr);
