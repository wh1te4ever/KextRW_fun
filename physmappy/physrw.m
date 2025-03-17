#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mach/mach.h>
#include <stdbool.h>
#include <errno.h>
#include "offsets.h"
#include "physrw.h"
#include "kextrw.h"
#include "physrw.h"
#include "translation.h"
#include "handoff.h"

void enumerate_pages(uint64_t start, size_t size, uint64_t pageSize, bool (^block)(uint64_t curStart, size_t curSize))
{
	uint64_t curStart = start;
	size_t sizeLeft = size;
	bool c = true;
	while (sizeLeft > 0 && c) {
		uint64_t pageOffset = curStart & (pageSize - 1);
		uint64_t readSize = min(sizeLeft, pageSize - pageOffset);
		c = block(curStart, readSize);
		curStart += readSize;
		sizeLeft -= readSize;
	}
}

int physreadbuf_via_krw(uint64_t pa, void* output, size_t size)
{
	memset(output, 0, size);

	__block int pr = 0;
	enumerate_pages(pa, size, vm_real_kernel_page_size, ^bool(uint64_t curPhys, size_t curSize){
		uint64_t curKaddr = phystokv(curPhys);
		if (curKaddr == 0 && errno != 0) {
			pr = errno;
			return false;
		}
		pr = kreadbuf(curKaddr, &output[curPhys - pa], curSize);
		if (pr != 0) {
			return false;
		}
		return true;
	});
	return pr;
}

uint64_t physread64_via_krw(uint64_t pa)
{
	uint64_t v;
	physreadbuf_via_krw(pa, &v, sizeof(v));
	return v;
}

uint16_t physread16_via_krw(uint64_t pa)
{
	uint16_t v;
	physreadbuf_via_krw(pa, &v, sizeof(v));
	return v;
}

uint32_t physread32_via_krw(uint64_t pa)
{
	uint32_t v;
	physreadbuf_via_krw(pa, &v, sizeof(v));
	return v;
}

int physwritebuf_via_krw(uint64_t pa, const void* input, size_t size)
{
	__block int pr = 0;
	enumerate_pages(pa, size, vm_real_kernel_page_size, ^bool(uint64_t curPhys, size_t curSize){
		uint64_t curKaddr = phystokv(curPhys);
		if (curKaddr == 0 && errno != 0) {
			pr = errno;
			return false;
		}
		pr = kwritebuf(curKaddr, &input[curPhys - pa], curSize);
		if (pr != 0) {
			return false;
		}
		return true;
	});
	return pr;
}

int physwrite8_via_krw(uint64_t pa, uint8_t v)
{
	return physwritebuf_via_krw(pa, &v, sizeof(v));
}

int physwrite16_via_krw(uint64_t pa, uint16_t v)
{
	return physwritebuf_via_krw(pa, &v, sizeof(v));
}

int physwrite32_via_krw(uint64_t pa, uint32_t v)
{
	return physwritebuf_via_krw(pa, &v, sizeof(v));
}

int physwrite64_via_krw(uint64_t pa, uint64_t v)
{
	return physwritebuf_via_krw(pa, &v, sizeof(v));
}

//phys r/w using PPLRW_USER_MAPPING_OFFSET
void *phystouaddr(uint64_t pa)
{
	errno = 0;

	uint64_t physBase = kread64(ksym(KSYMBOL_gPhysBase)), physSize = kread64(ksym(KSYMBOL_gPhysSize));
	bool doBoundaryCheck = (physBase != 0 && physSize != 0);
	if (doBoundaryCheck) {
		if (pa < physBase || pa >= (physBase + physSize)) {
			errno = 1030;
			return 0;
		}
	}

	return (void *)(pa + PPLRW_USER_MAPPING_OFFSET);
}

int physreadbuf(uint64_t pa, void* output, size_t size)
{
	void *uaddr = phystouaddr(pa);
	if (!uaddr && errno != 0) {
		memset(output, 0x0, size);
		return errno;
	}

	asm volatile("dmb sy");
	memcpy(output, uaddr, size);
	return 0;
}

int physwritebuf(uint64_t pa, const void* input, size_t size)
{
	void *uaddr = phystouaddr(pa);
	if (!uaddr && errno != 0) {
		return errno;
	}

	memcpy(uaddr, input, size);
	asm volatile("dmb sy");
	return 0;
}

int kreadbuf_via_prw(uint64_t kaddr, void* output, size_t size)
{
	memset(output, 0, size);

	__block int pr = 0;
	enumerate_pages(kaddr, size, vm_real_kernel_page_size, ^bool(uint64_t curKaddr, size_t curSize){
		uint64_t curPhys = kvtophys(curKaddr);
		if (curPhys == 0 && errno != 0) {
			pr = errno;
			return false;
		}
		pr = physreadbuf(curPhys, &output[curKaddr - kaddr], curSize);
		if (pr != 0) {
			return false;
		}
		return true;
	});
	return pr;
}

int kwritebuf_via_prw(uint64_t kaddr, const void* input, size_t size)
{
	__block int pr = 0;
	enumerate_pages(kaddr, size, vm_real_kernel_page_size, ^bool(uint64_t curKaddr, size_t curSize){
		uint64_t curPhys = kvtophys(curKaddr);
		if (curPhys == 0 && errno != 0) {
			pr = errno;
			return false;
		}
		pr = physwritebuf(curPhys, &input[curKaddr - kaddr], curSize);
		if (pr != 0) {
			return false;
		}
		return true;
	});
	return pr;
}

uint32_t kread32_via_prw(uint64_t where) {
    uint32_t out;
    kreadbuf_via_prw(where, &out, sizeof(uint32_t));
    return out;
}

uint64_t kread64_via_prw(uint64_t where) {
    uint64_t out;
    kreadbuf_via_prw(where, &out, sizeof(uint64_t));
    return out;
}

void kwrite32_via_prw(uint64_t where, uint32_t what) {
    uint32_t _what = what;
    kwritebuf_via_prw(where, &_what, sizeof(uint32_t));
}

void kwrite64_via_prw(uint64_t where, uint64_t what) {
    uint64_t _what = what;
    kwritebuf_via_prw(where, &_what, sizeof(uint64_t));
}