gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g -shared -DDRIVER_NUMBER=2 driver.c -o libdriver2.so
gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g -shared driver.c -o libdriver1.so
gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g -shared -DLAYER_NUMBER=2 layer.c -o liblayer2.so
gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g -shared layer.c -o liblayer1.so
gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g -shared -DLAYER_NUMBER=2 -DFFI_INSTANCE_LAYERS=0 instance_layer.c -o libinstance_layer2.so
gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g -shared -DFFI_INSTANCE_LAYERS=0 instance_layer.c -o libinstance_layer1.so
gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g -shared -DFFI_INSTANCE_LAYERS=0 exp-loader.c -o libexp-loader.so -ldl -lpthread
gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g test.c -o test -L./ -lexp-loader
gcc -Wall -Wextra -pedantic -std=c99 -fPIC -g test.c -DNO_PROTOTYPES -o test_dlopen -L./ -ldl
