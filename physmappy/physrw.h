#include <unistd.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

int physreadbuf_via_krw(uint64_t pa, void* output, size_t size);
uint64_t physread64_via_krw(uint64_t pa);
uint32_t physread32_via_krw(uint64_t pa);
uint16_t physread16_via_krw(uint64_t pa);

int physwritebuf_via_krw(uint64_t pa, const void* input, size_t size);
int physwrite8_via_krw(uint64_t pa, uint8_t v);
int physwrite16_via_krw(uint64_t pa, uint16_t v);
int physwrite32_via_krw(uint64_t pa, uint32_t v);
int physwrite64_via_krw(uint64_t pa, uint64_t v);