#include <cstdint>
#include <cstring>
#include <algorithm>

static const std::size_t Elements = 64 * 1024 * 1024;

static unsigned char Src[Elements];
static unsigned char Dst[Elements];

int main()
{
	std::memset(Src, 2, Elements);
	std::memset(Dst, 1, Elements);

	asm volatile("" : : "r,m"(Dst) : "memory");
	std::memcpy(Dst, Src, Elements);
	asm volatile("" : : "r,m"(Dst) : "memory");

	if (std::any_of(Dst, Dst + Elements, [](unsigned char c) { return c != 2; }))
		throw 42;

	return 0;
}

