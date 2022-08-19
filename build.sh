gcc -Wall -Wextra -pedantic -std=c99 -g test.c -o test -L./ -lexp-loader
gcc -Wall -Wextra -pedantic -std=c99 -g -shared -DDRIVER_NUMBER=2 driver.c -o libdriver2.so
gcc -Wall -Wextra -pedantic -std=c99 -g -shared driver.c -o libdriver1.so
gcc -Wall -Wextra -pedantic -std=c99 -g -shared exp-loader.c -o libexp-loader.so -ldl
