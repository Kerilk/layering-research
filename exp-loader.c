#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include "spec.h"

typedef int (*pfn_platformCreateDevice_t)(platform_t plt, device_t *dev);
typedef int (*pfn_deviceFunc1_t)(device_t dev, int param);
typedef int (*pfn_deviceFunc2_t)(device_t dev, int param);
typedef int (*pfn_deviceDestroy_t)(device_t dev);

struct dispatch_s {
	pfn_platformCreateDevice_t platformCreateDevice;
	pfn_deviceFunc1_t          deviceFunc1;
	pfn_deviceFunc2_t          deviceFunc2;
	pfn_deviceDestroy_t        deviceDestroy;
};

struct multiplex_s {
	struct dispatch_s dispatch;
};

struct layer_s;
struct layer_s {
	void              *library;
	struct dispatch_s  dispatch;
	struct layer_s    *next;
};

struct plt_s;
struct plt_s {
	platform_t          platform;
	struct multiplex_s  multiplex;
	struct plt_s       *next;
};

typedef int (*pfn_getPaltfomsExt_t)(size_t num_platform, platform_t *platforms, size_t *num_platform_ret);
typedef void * (*pfn_platformGetFunc_t)(platform_t platform, const char *name);

struct driver_s;
struct driver_s {
	void                  *library;
	pfn_getPaltfomsExt_t   getPaltfomsExt;
	pfn_platformGetFunc_t  platformGetFunc;
	size_t                 num_platforms;
	platform_t            *platforms;
	struct driver_s       *next;
};

//static struct layer_s  *_first_layer = NULL;
static struct plt_s    *_first_platform = NULL;
static size_t           _num_platforms = 0;
static struct driver_s *_first_driver = NULL;

struct platform_s {
	struct multiplex_s *multiplex;
};

struct device_s {
	struct multiplex_s *multiplex;
};

static pthread_once_t initialized = PTHREAD_ONCE_INIT;

static void *
loadLibrary(const char *libraryName) {
	return dlopen(libraryName, RTLD_LAZY|RTLD_LOCAL);
}

char *get_next(char *paths) {
	char *next;
	next = strchr(paths, ':');
	if (next) {
		*next = '\0';
		next++;
	}
	return next;
}

static void
loadPlatforms(struct driver_s *driver) {
	for (size_t i = 0; i < driver->num_platforms; i++) {
		struct plt_s *plt = (struct plt_s *)calloc(1, sizeof(struct plt_s));
		platform_t platform = driver->platforms[i];
		plt->platform = platform;
		plt->multiplex.dispatch.platformCreateDevice = (pfn_platformCreateDevice_t)(intptr_t)driver->platformGetFunc(platform, "platformCreateDevice");
		plt->multiplex.dispatch.deviceFunc1 = (pfn_deviceFunc1_t)(intptr_t)driver->platformGetFunc(platform, "deviceFunc1");
		plt->multiplex.dispatch.deviceFunc2 = (pfn_deviceFunc2_t)(intptr_t)driver->platformGetFunc(platform, "deviceFunc2");
		plt->multiplex.dispatch.deviceDestroy = (pfn_deviceDestroy_t)(intptr_t)driver->platformGetFunc(platform, "deviceDestroy");
		plt->next = _first_platform;
		plt->platform->multiplex = &plt->multiplex;
		_first_platform = plt;
		_num_platforms++;
	}
}

static void
loadDriver(const char *path) {
	struct driver_s *driver = NULL;
	size_t num_platforms;
	void *lib = loadLibrary(path);
	if (!lib)
		return;
	pfn_getPaltfomsExt_t p_getPaltfomsExt = (pfn_getPaltfomsExt_t)(intptr_t)dlsym(lib, "getPlatformsExt");
	if (!p_getPaltfomsExt)
		goto error;
	pfn_platformGetFunc_t p_platformGetFunc = (pfn_platformGetFunc_t)(intptr_t)dlsym(lib, "platformGetFunc");
	if (!p_platformGetFunc)
		goto error;
	if (p_getPaltfomsExt(0, NULL, &num_platforms) || !num_platforms)
		goto error;
	driver = (struct driver_s *)calloc(1, sizeof(struct driver_s) + num_platforms * sizeof(platform_t));
	driver->library = lib;
	driver->getPaltfomsExt = p_getPaltfomsExt;
	driver->platformGetFunc = p_platformGetFunc;
	driver->num_platforms = num_platforms;
	driver->platforms = (platform_t *)((intptr_t)driver + sizeof(struct platform_s));
	if (p_getPaltfomsExt(num_platforms, driver->platforms, NULL))
		goto error;
	loadPlatforms(driver);
	driver->next = _first_driver;
	_first_driver = driver;
	return;
error:
	if (driver)
		free(driver);
	dlclose(lib);
}

static void
initReal() {
	char *drivers = getenv("DRIVERS");
	if (drivers) {
		char *next_file = drivers;
		while (NULL != next_file && *next_file != '\0') {
			char *cur_file = next_file;
			next_file = get_next(cur_file);
			loadDriver(cur_file);
		}
	}
}

static void
initOnce(void) {
	pthread_once(&initialized, initReal);
}

int
getPlatforms(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret) {
	initOnce();
	if (num_platforms_ret)
		*num_platforms_ret = _num_platforms;
	if (num_platforms && platforms) {
		if (num_platforms < _num_platforms)
			return SPEC_ERROR;
		struct plt_s *plt = _first_platform;
		size_t i = 0;
		while (plt) {
			platforms[i] = plt->platform;
			plt = plt->next;
			i++;
		}
		while (i < num_platforms)
			platforms[i] = NULL;
	}
	return SPEC_SUCCESS;
}

int
platformCreateDevice(platform_t platform, device_t *device_ret) {
	if (!platform)
		return SPEC_ERROR;
	if (!platform->multiplex->dispatch.platformCreateDevice)
		return SPEC_UNSUPPORTED;
	int result = platform->multiplex->dispatch.platformCreateDevice(platform, device_ret);
	if (result == SPEC_SUCCESS) {
		(*device_ret)->multiplex = platform->multiplex;
	}
	return result;
}

int
deviceFunc1(device_t device, int param) {
	if (!device)
		return SPEC_ERROR;
	if (!device->multiplex->dispatch.deviceFunc1)
		return SPEC_UNSUPPORTED;
	return device->multiplex->dispatch.deviceFunc1(device, param);
}

int
deviceFunc2(device_t device, int param) {
	if (!device)
		return SPEC_ERROR;
	if (!device->multiplex->dispatch.deviceFunc2)
		return SPEC_UNSUPPORTED;
	return device->multiplex->dispatch.deviceFunc2(device, param);
}

int
deviceDestroy(device_t device) {
	if (!device)
		return SPEC_ERROR;
	if (!device->multiplex->dispatch.deviceDestroy)
		return SPEC_UNSUPPORTED;
	return device->multiplex->dispatch.deviceDestroy(device);
}

__attribute__((destructor))
void my_fini(void) {
	printf("Deinitiing loader\n");
	struct plt_s *platform = _first_platform;
	while(platform) {
		struct plt_s *next_platform = platform->next;
		free(platform);
		platform = next_platform;
	}
	struct driver_s *driver = _first_driver;
	while(driver) {
		struct driver_s *next_driver = driver->next;
		dlclose(driver->library);
		free(driver);
		driver = next_driver;
	}
}
