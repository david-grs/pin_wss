WSS (Working Set Size) estimator
--------------------------------

### Build
```
$ PIN_ROOT=~/src/pin-3.7-97619-g0d0c92f4f-gcc-linux/ make
```

### Usage example
```
$ cat memcpy.cc 

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

$ g++ -g -O3 -o memcpy memcpy.cc

$ pin -t ./obj-intel64/wss.so -- ./memcpy && head -20 wss.out
        reads      WSS (R)       writes      WSS (W)          WSS        calls         insn             function
          256        128 B      8192 Mi      128 MiB      128 MiB            2            2             memset@plt
      4096 Mi       64 MiB          192         64 B       64 MiB            1    335544338             main
       256 Mi       64 MiB       128 Mi       64 MiB      128 MiB            1            1             memcpy@plt
        26 Mi      434 KiB      8829 Ki       85 KiB      493 KiB            1         9367             .text
       889 Ki       68 KiB         9792       2560 B       71 KiB            1        59300             _dl_addr
       290 Ki       19 KiB       126 Ki       14 KiB       26 KiB           36          729             malloc
        99 Ki       9472 B        40 Ki       2048 B      10112 B            8           16             .plt
        58 Ki       9024 B        14 Ki       1728 B       9280 B           20          361             __tunable_get_val
        42 Ki       6208 B        13 Ki       1216 B       6464 B            2            4             .plt
        26 Ki       4608 B        19 Ki       1536 B       4928 B           10          310             _dl_catch_exception
        18 Ki       3200 B         9344       1024 B       3392 B            1           99             _init
         8768       4032 B         9984       1664 B       4864 B            0          369             .text
        11 Ki       1472 B         5888        512 B       1600 B            0          247             .text
        10 Ki       1024 B         4864        640 B       1216 B            4          483             __cxa_finalize
         9600       2112 B         4480       1024 B       2176 B            1            3             .fini
         7168        896 B         6336        576 B        960 B            6          216             __cxa_atexit
         4288       1472 B         7552       1344 B       2048 B            2           14             __default_morecore
         3264        704 B         2432        384 B        896 B            1          153             _dl_allocate_tls_init
         2368        832 B         1664        384 B        896 B            2            8             free
```

