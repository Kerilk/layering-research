gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g -shared -DDRIVER_NUMBER=2 driver.c -o libdriver2.so
gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g -shared driver.c -o libdriver1.so
gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g -shared exp-loader.c -o libexp-loader.so -ldl -lpthread
gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g test.c -o test -L./ -lexp-loader
