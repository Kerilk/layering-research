LD_LIBRARY_PATH=`pwd` valgrind -- ./test
echo "#################################################"
FIX=1 LD_LIBRARY_PATH=`pwd` valgrind -- ./test
echo "#################################################"
LD_LIBRARY_PATH=`pwd` valgrind -- ./test_dlopen
echo "#################################################"
FIX=1 LD_LIBRARY_PATH=`pwd` valgrind -- ./test_dlopen
