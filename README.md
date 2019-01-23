Pin Tools: WSS, memstats
------------------------

### Build
```
$ PIN_ROOT=~/src/pin-3.7-97619-g0d0c92f4f-gcc-linux/ make
```

### wss
```
$ pin -t ./obj-intel64/wss.so -- ~/src/stream
...

486521425 instructions
208269585 access
69696046 reads
writes 138573539 writes

WSS 131660kB
```


### memstats
```
$ time pin -t ./obj-intel64/memstats.so -- ~/src/stream | c++filt -t
...

   mem_reads  mem_writes       calls      function
    67108895          35           1      main
     2097158     4194309           1      copy(unsigned char*, unsigned char*, unsigned long)
       14225          17           1      _dl_addr
        2944        1280         256      btowc
        2569        1368           1      .text
        3180         270          30      std::locale::_Impl::_M_install_facet(std::locale::id const*, std::locale::facet const*)
        1152         768         128      wctob
         931         539          49      __dynamic_cast
           5         810           1      std::ctype<wchar_t>::_M_initialize_ctype()
         440         360          40      __cxxabiv1::__si_class_type_info::__do_dyncast(long, __cxxabiv1::__class_type_info::__sub_kind, __cxxabiv1::__class_type_info const*, void const*, __cxxabiv1::__class_type_info const*, void const*, __cxxabiv1::__class_type_info::__dyncast_result&) const
          80         423           1      std::locale::_Impl::_Impl(unsigned long)
         322          92         115      std::locale::id::_M_id() const
         225         162           9      __cxxabiv1::__vmi_class_type_info::__do_dyncast(long, __cxxabiv1::__class_type_info::__sub_kind, __cxxabiv1::__class_type_info const*, void const*, __cxxabiv1::__class_type_info const*, void const*, __cxxabiv1::__class_type_info::__dyncast_result&) const
         384           0         128      _dl_mcount_wrapper_check
         198         176          22      std::locale::locale()
         158         177          10      wctype_l
         216         108          12      _IO_fflush
          26         278           1      std::ctype<char>::_M_widen_init() const
         184         105           8      fwrite
         172          86          86      .plt
         256           1         256      btowc@plt
         104         146           0      .text
         120         121          36      malloc
...
```
