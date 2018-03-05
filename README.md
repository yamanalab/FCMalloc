# FCMalloc: A Fast Memory Allocator for Fully Homomorphic Encryption
FCMalloc library enables to control local heaps to speed up
memory allocation and deallocation by adopting pseudo free technique.


## how to build
CMake, GNU make, GCC are required to build FCMalloc library.

```
$ cd fcmalloc
$ cmake .
$ make
```


## how to run
In order to use FCMalloc library, set environment variable `LD_PRELOAD`
to the path to `libfcmalloc.so`. For example, zsh uses FCMalloc library;
```
LD_PRELOAD=./libfcmalloc.so zsh
```

### options
* FCM_SIZE_LIST_FILE
    * memory size list file name (format must be csv)
* FCM_FORCE_EXTEND_MEM_FLAG
    * whether memory extension is forced (default: 1)
* FCM_MAIN_MEM_MAX
    * maximum memory size (GB) for main thread (default: 32)
* FCM_SUB_MEM_MAX
    * maximum memory size (GB) for each thread (default: 4 * #cores)
* FCM_LOG_OUTPUT
    * log file name for main thread (`stdout`, `stderr`, or `/dev/null` are also acceptable)
    * currently no log is output to `FCM_LOG_OUTPUT`
* FCM_LOG_PREFIX
    * log file name prefix, which is used for log file name for each core
         * log file name is `PREFIX_xxx.log` when FCM_LOG_PREFIX is `PREFIX`
* FCM_MEM_LOG_INTVL
    * memory log interval(default: 10000)
         * When the number of malloc/free reaches `FCM_MEM_LOG_INTVL`,
           #malloc and #free are output to the log file.
* FCM_POOL_BUFFER_SIZE
    * the number of memory pool buffer per core (default: 4)


## NOTE
* The following functions are unsupported.
    * pvalloc
    * mallopt
    * posix_memalign
* Memory allocated within libfcmalloc.so is not released
  unless the process is terminated.


## References
* TCMalloc
    * Google Inc., "gperftools: Fast, multi-threaded malloc() and nifty performance analysis tools ", http://code.google.com/p/gperftools/ accessed on 2017-03-15.
* JEmalloc
    * Evans, J., "A Scalable Concurrent malloc(3) Implementation for FreeBSD", Proc. of BSDCan, 2006.
* Supermalloc
    * Kuszmaul, B. C., "Supermalloc: A Super Fast Multithreaded Malloc for 64-bit Machines", Proc. of ISMM, pp. 41-55, 2015.
* FHE
    * Gentry, C., "A Fully Homomorphic Encryption Scheme", Ph.D. Thesis, Stanford University, 2009.
* HElib
    * http://shaih.github.io/HElib/
