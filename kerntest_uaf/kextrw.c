#include "kextrw.h"
#include "offsets.h"
#include <stdint.h>
#include <mach/mach.h> 
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <IOKit/IOKitLib.h>
#include <mach-o/loader.h>

uint64_t gKernelBase = 0, gKernelSlide = 0;
io_connect_t gClient = MACH_PORT_NULL;

#ifndef MIN
#    define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

kern_return_t kextrw_init(void) {
    io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("KextRW"));
    if(!MACH_PORT_VALID(service))   return KERN_FAILURE;
    kern_return_t ret = IOServiceOpen(service, mach_task_self(), 0, &gClient);
    IOObjectRelease(service);
    if(ret != KERN_SUCCESS) return KERN_FAILURE;

	MACH_PORT_VALID(gClient) ? KERN_SUCCESS : KERN_FAILURE;
}

void kextrw_deinit(void) {
    IOServiceClose(gClient);
}

static inline kern_return_t kextrw_get_reset_vector(io_connect_t client, uint64_t *out)
{
    uint32_t outCnt = 1; 
    return IOConnectCallScalarMethod(client, 4, NULL, 0, out, &outCnt);
}

int get_kernel_base(void)
{
    uint64_t kernelPage = 0;
    kextrw_get_reset_vector(gClient, &kernelPage);
    printf("kernelPage: 0x%llx\n", kernelPage);
    if (!kernelPage) return 0;

    uint64_t kernelBase = 0;
    while (!kernelBase) {
        if (kread32(kernelPage) == MH_MAGIC_64
            && kread32(kernelPage + 0xC) == MH_EXECUTE) {
            kernelBase = kernelPage;
            break;
        }
        kernelPage -= 0x1000;
    }

    gKernelSlide = kernelBase - VM_KERNEL_LINK_ADDR;
    gKernelBase = kernelBase;

    return 0;
}

kern_return_t
kreadbuf(uint64_t kaddr, void *buf, size_t sz) {
    uint64_t in[] = { kaddr, (uint64_t)buf, sz };
    return IOConnectCallScalarMethod(gClient, 0, in, 3, NULL, NULL);
}

kern_return_t
kwritebuf(uint64_t kaddr, const void *buf, size_t sz) {
    uint64_t in[] = { kaddr, (uint64_t)buf, sz };
    return IOConnectCallScalarMethod(gClient, 1, in, 3, NULL, NULL);
}

uint8_t kread8(uint64_t where) {
    uint8_t out;
    kreadbuf(where, &out, sizeof(uint8_t));
    return out;
}

uint16_t kread16(uint64_t where) {
    uint16_t out;
    kreadbuf(where, &out, sizeof(uint16_t));
    return out;
}

uint32_t kread32(uint64_t where) {
    uint32_t out;
    kreadbuf(where, &out, sizeof(uint32_t));
    return out;
}

uint64_t kread64(uint64_t where) {
    uint64_t out;
    kreadbuf(where, &out, sizeof(uint64_t));
    return out;
}

uint64_t xpaci(uint64_t ptr)
{
	asm("xpaci %[value]\n" : [value] "+r"(ptr));
	return ptr;
}

uint64_t kreadptr(uint64_t where)
{
    return xpaci(kread64(where));
}

void kwrite8(uint64_t where, uint8_t what) {
    uint8_t _what = what;
    kwritebuf(where, &_what, sizeof(uint8_t));
}

void kwrite16(uint64_t where, uint16_t what) {
    uint16_t _what = what;
    kwritebuf(where, &_what, sizeof(uint16_t));
}

void kwrite32(uint64_t where, uint32_t what) {
    uint32_t _what = what;
    kwritebuf(where, &_what, sizeof(uint32_t));
}

void kwrite64(uint64_t where, uint64_t what) {
    uint64_t _what = what;
    kwritebuf(where, &_what, sizeof(uint64_t));
}

const uint64_t kernel_address_space_base = 0xffff000000000000;
void kmemcpy(uint64_t dest, uint64_t src, uint32_t length) {
    if (dest >= kernel_address_space_base) {
      // copy to kernel:
      kwritebuf(dest, (void*) src, length);
    } else {
      // copy from kernel
      kreadbuf(src, (void*)dest, length);
    }
}

void khexdump(uint64_t addr, size_t size) {
    void *data = malloc(size);
    kreadbuf(addr, data, size);
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i) {
        if ((i % 16) == 0)
        {
            printf("[0x%016llx+0x%03zx] ", addr, i);
//            printf("[0x%016llx] ", i + addr);
        }
        
        printf("%02X ", ((unsigned char*)data)[i]);
        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char*)data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i+1) % 8 == 0 || i+1 == size) {
            printf(" ");
            if ((i+1) % 16 == 0) {
                printf("|  %s \n", ascii);
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8) {
                    printf(" ");
                }
                for (j = (i+1) % 16; j < 16; ++j) {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
    free(data);
}

uint64_t kcall10(uint64_t fn, uint64_t *args, uint32_t argsCnt)
{
    uint64_t rv = 0;
    if (argsCnt > 10) return KERN_INVALID_ARGUMENT;

    uint64_t argsBuf[11] = { 0 };
    argsBuf[0] = fn;
    for (uint32_t i = 0; i < argsCnt; i++)
    {
        if (args[i]) argsBuf[i + 1] = args[i] ? args[i] : 0;
    }
    uint32_t outCnt = 1;
    IOReturn ret = IOConnectCallScalarMethod(gClient, 7, argsBuf, 11, &rv, &outCnt);

    if (ret != KERN_SUCCESS) printf("WARNING: kcall failed with error %d\n", ret);
    return rv;
}

/* Physical read/write */
static inline kern_return_t kextrw_physread(io_connect_t client, uint64_t from, void *to, uint64_t len, uint8_t align)
{
    uint64_t in[] = { from, (uint64_t)to, len, align };
    return IOConnectCallScalarMethod(client, 2, in, 4, NULL, NULL);
}

static inline kern_return_t kextrw_physwrite(io_connect_t client, void *from, uint64_t to, uint64_t len, uint8_t align)
{
    uint64_t in[] = { (uint64_t)from, to, len, align };
    return IOConnectCallScalarMethod(client, 3, in, 4, NULL, NULL);
}

uint8_t physread8(uint64_t addr)
{
    uint8_t val = 0;
    kextrw_physread(gClient, addr, &val, sizeof(val), 1);
    return val;
}

uint16_t physread16(uint64_t addr)
{
    uint16_t val = 0;
    kextrw_physread(gClient, addr, &val, sizeof(val), 2);
    return val;
}

uint32_t physread32(uint64_t addr)
{
    uint32_t val = 0;
    kextrw_physread(gClient, addr, &val, sizeof(val), 4);
    return val;
}

uint64_t physread64(uint64_t addr)
{
    uint64_t val = 0;
    kextrw_physread(gClient, addr, &val, sizeof(val), 8);
    return val;
}

int physreadbuf(uint64_t addr, void *buf, size_t len)
{
    return kextrw_physread(gClient, addr, buf, len, 1);
}

void physwrite8(uint64_t addr, uint8_t val)
{
    kextrw_physwrite(gClient, &val, addr, sizeof(val), 1);
}

void physwrite16(uint64_t addr, uint16_t val)
{
    kextrw_physwrite(gClient, &val, addr, sizeof(val), 2);
}

void physwrite32(uint64_t addr, uint32_t val)
{
    kextrw_physwrite(gClient, &val, addr, sizeof(val), 4);
}

void physwrite64(uint64_t addr, uint64_t val)
{
    kextrw_physwrite(gClient, &val, addr, sizeof(val), 8);
}

int physwritebuf(uint64_t addr, void *buf, size_t len)
{
    return kextrw_physwrite(gClient, buf, addr, len, 1);
}

/* Translation */
static inline kern_return_t kextrw_kvtophys(io_connect_t client, uint64_t va, uint64_t *out)
{
    uint64_t in = va;
    uint32_t outCnt = 1;
    return IOConnectCallScalarMethod(client, 5, &in, 1, out, &outCnt);
}

static inline kern_return_t kextrw_phystokv(io_connect_t client, uint64_t pa, uint64_t *out)
{
    uint64_t in = pa;
    uint32_t outCnt = 1;
    return IOConnectCallScalarMethod(client, 6, &in, 1, out, &outCnt);
}

uint64_t kvtophys(uint64_t va)
{
    uint64_t pa = 0;
    kextrw_kvtophys(gClient, va, &pa);
    return pa;
}

uint64_t phystokv(uint64_t pa)
{
    uint64_t va = 0;
    kextrw_phystokv(gClient, pa, &va);
    return va;
}

/* Allocations */
static inline kern_return_t kextrw_kalloc(io_connect_t client, uint64_t size, uint64_t *out)
{
    uint64_t in = size;
    uint32_t outCnt = 1;
    return IOConnectCallScalarMethod(client, 8, &in, 1, out, &outCnt);
}

static inline kern_return_t kextrw_kfree(io_connect_t client, uint64_t addr, uint64_t size)
{
    uint64_t in[] = { addr, size };
    return IOConnectCallScalarMethod(client, 9, in, 2, NULL, NULL);
}

uint64_t kalloc(uint64_t size)
{
    uint64_t addr = 0;
    kextrw_kalloc(gClient, size, &addr);
    return addr;
}

void kfree(uint64_t addr, uint64_t size)
{
    kextrw_kfree(gClient, addr, size);
}
