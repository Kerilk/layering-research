# Memory Leak Reproducer

Requirements are `gcc` and `vagrind`.

 * The reproducer can be built by invoking `./build.sh`.
 * The reproducer can then be executed by invoking `./run.sh`

The expected output on `glibc 2.35` is:

```
==20850== Memcheck, a memory error detector
==20850== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==20850== Using Valgrind-3.18.1 and LibVEX; rerun with -h for copyright info
==20850== Command: ./test
==20850== 
Initing
Opened 0x4aab4f0
Deiniting
Closing 0x4aab4f0
==20850== 
==20850== HEAP SUMMARY:
==20850==     in use at exit: 1,373 bytes in 4 blocks
==20850==   total heap usage: 8 allocs, 4 frees, 4,765 bytes allocated
==20850== 
==20850== LEAK SUMMARY:
==20850==    definitely lost: 0 bytes in 0 blocks
==20850==    indirectly lost: 0 bytes in 0 blocks
==20850==      possibly lost: 0 bytes in 0 blocks
==20850==    still reachable: 1,373 bytes in 4 blocks
==20850==         suppressed: 0 bytes in 0 blocks
==20850== Rerun with --leak-check=full to see details of leaked memory
==20850== 
==20850== For lists of detected and suppressed errors, rerun with: -s
==20850== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
#################################################
==20852== Memcheck, a memory error detector
==20852== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==20852== Using Valgrind-3.18.1 and LibVEX; rerun with -h for copyright info
==20852== Command: ./test
==20852== 
Initing
Opened 0x4aab4f0
Closing 0x4aab4f0
Deiniting
==20852== 
==20852== HEAP SUMMARY:
==20852==     in use at exit: 0 bytes in 0 blocks
==20852==   total heap usage: 8 allocs, 8 frees, 4,765 bytes allocated
==20852== 
==20852== All heap blocks were freed -- no leaks are possible
==20852== 
==20852== For lists of detected and suppressed errors, rerun with: -s
==20852== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```
