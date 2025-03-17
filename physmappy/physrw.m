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