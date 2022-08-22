gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g -shared driver.c -o libdriver.so
gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g -shared exp-loader.c -o libexp-loader.so -ldl -lpthread
gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g test.c -o test -L./ -lexp-loader
gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g test_dlopen.c -o test_dlopen -ldl
