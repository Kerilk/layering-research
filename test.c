#include <stdlib.h>
extern void initLoader();
extern void deinitLoader();

int main() {
	initLoader();
	if (getenv("FIX"))
		deinitLoader();
	return 0;
}
