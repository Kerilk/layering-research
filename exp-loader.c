#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include "spec.h"
#include "dispatch.h"
#include "layer.h"

static int
getPlatforms_disp(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret);
static int
platformCreateDevice_disp(platform_t platform, device_t *device_ret);
static int
deviceFunc1_disp(device_t device, int param);
static int
deviceFunc2_disp(device_t device, int param);
static int
deviceDestroy_disp(device_t device);

/*static struct dispatch_s _mdispatch = {
	&getPlatforms_disp,
	&platformCreateDevice_disp,
	&deviceFunc1_disp,
	&deviceFunc2_disp,
	&deviceDestroy_disp
};*/

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

static struct layer_s _layer_terminator = {
	NULL,
	{
		&getPlatforms_disp,
		&platformCreateDevice_disp,
		&deviceFunc1_disp,
		&deviceFunc2_disp,
		&deviceDestroy_disp
	},
	NULL
};
static struct layer_s  *_first_layer = &_layer_terminator;
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
		plt->multiplex.dispatch.getPlatforms = NULL;
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
loadLayer(const char *path) {
	struct layer_s *layer = NULL;
	void *lib = loadLibrary(path);
	if (!lib)
		return;
	pfn_layerInit_t p_layerInit = (pfn_layerInit_t)(intptr_t)dlsym(lib, "layerInit");
	if (!p_layerInit)
		goto error;
	layer = (struct layer_s *)calloc(1, sizeof(struct layer_s));
	layer->library = lib;
	size_t num_entries = 0;
	struct dispatch_s *p_dispatch = NULL;
	if (p_layerInit(NUM_DISPATCH_ENTRIES, &_first_layer->dispatch, &num_entries, &p_dispatch))
		goto error;
	size_t count = num_entries < NUM_DISPATCH_ENTRIES ? num_entries : NUM_DISPATCH_ENTRIES;
	for (size_t i = 0; i < count; i++) {
		((void **)&(layer->dispatch))[i] = ((void **)p_dispatch)[i] ? ((void **)p_dispatch)[i] : ((void **)&(_first_layer->dispatch))[i];
	}
	for (size_t i = count; i < NUM_DISPATCH_ENTRIES; i++) {
		((void **)&(layer->dispatch))[i] = ((void **)&(_first_layer->dispatch))[i];
	}
	layer->next = _first_layer;
	_first_layer = layer;
	return;
error:
	if (layer)
		free(layer);
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
	char *layers = getenv("LAYERS");
	if (layers) {
		char *next_file = layers;
		while (NULL != next_file && *next_file != '\0') {
			char *cur_file = next_file;
			next_file = get_next(cur_file);
			loadLayer(cur_file);
		}
	}
}

static void
initOnce(void) {
	pthread_once(&initialized, initReal);
}

static inline int
getPlatforms_body(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret) {
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
getPlatforms(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret) {
	return _first_layer->dispatch.getPlatforms(num_platforms, platforms, num_platforms_ret);
}

static int
getPlatforms_disp(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret) {
	return getPlatforms_body(num_platforms, platforms, num_platforms_ret);
}

static inline int
platformCreateDevice_body(platform_t platform, device_t *device_ret) {
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
platformCreateDevice(platform_t platform, device_t *device_ret) {
	return _first_layer->dispatch.platformCreateDevice(platform, device_ret);
}

static int
platformCreateDevice_disp(platform_t platform, device_t *device_ret) {
	return platformCreateDevice_body(platform, device_ret);
}

static inline int
deviceFunc1_body(device_t device, int param) {
	if (!device)
		return SPEC_ERROR;
	if (!device->multiplex->dispatch.deviceFunc1)
		return SPEC_UNSUPPORTED;
	return device->multiplex->dispatch.deviceFunc1(device, param);
}

int deviceFunc1(device_t device, int param) {
	return _first_layer->dispatch.deviceFunc1(device, param);
}

static int deviceFunc1_disp(device_t device, int param) {
	return deviceFunc1_body(device, param);
}

static inline int
deviceFunc2_body(device_t device, int param) {
	if (!device)
		return SPEC_ERROR;
	if (!device->multiplex->dispatch.deviceFunc2)
		return SPEC_UNSUPPORTED;
	return device->multiplex->dispatch.deviceFunc2(device, param);
}

int
deviceFunc2(device_t device, int param) {
	return _first_layer->dispatch.deviceFunc2(device, param);
}

static int
deviceFunc2_disp(device_t device, int param) {
	return deviceFunc2_body(device, param);
}

static inline int
deviceDestroy_body(device_t device) {
	if (!device)
		return SPEC_ERROR;
	if (!device->multiplex->dispatch.deviceDestroy)
		return SPEC_UNSUPPORTED;
	return device->multiplex->dispatch.deviceDestroy(device);
}

int
deviceDestroy(device_t device) {
	return _first_layer->dispatch.deviceDestroy(device);
}

static int
deviceDestroy_disp(device_t device) {
	return deviceDestroy_body(device);
}

__attribute__((destructor))
void my_fini(void) {
	printf("Deiniting loader\n");
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
	struct layer_s *layer = _first_layer;
	while(layer != &_layer_terminator) {
		struct layer_s *next_layer = layer->next;
		dlclose(layer->library);
		free(layer);
		layer = next_layer;
	}
}
