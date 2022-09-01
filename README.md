# layering-research

This project contains a proof of concept demonstrator of an ICD loader for a toy API. This ICD loader supports multiplexing drivers, as well as global layers and instance layers.

Two approaches are proposed for instance layers, one using ffi (which can be extended to support arbitrary extension functions not known to the loader) and one that is not using FFI.

The multiplexing and the instance layering rely on the opaque handles returned by the drivers being structure containing a writable `void *` pointer as their first field.

## Building

Two simple build scripts are provided, to compile both, the ffi and non-ffi version of the demonstrator. They expect `gcc`, a working `libc`, and for the ffi version a `libffi` version supporting closures. Those scripts are called `build_ffi.sh` and `build.sh`.

## Runing

A simple runscript is provided, `run.sh`, that uses valgrind for memory validation. The script instanciates 2 drivers, 2 global layers, and invokes a simple test program. The test program lists and test all supported platforms and tests their functionalites by creating an object and calling related APIs. The first platform is enhanced by 2 instance layers. The drivers and the layers are printing a log that enables validating the loader and layers behavior.

## Results

For reference, the expected output of the test, is supposed to look similar to this (irrespective of the version built):
```
DRIVER 1: entering getPlatformsExt(num_platforms = 0, platforms = (nil), num_platforms_ret = 0x1ffefff8f0)
DRIVER 1: entering getPlatformsExt(num_platforms = 1, platforms = 0x4aad528, num_platforms_ret = (nil))
DRIVER 1: entering platformGetFunc(platform = 0x4865048, name = platformCreateDevice)
DRIVER 1: entering platformGetFunc(platform = 0x4865048, name = deviceFunc1)
DRIVER 1: entering platformGetFunc(platform = 0x4865048, name = deviceFunc2)
DRIVER 1: entering platformGetFunc(platform = 0x4865048, name = deviceDestroy)
DRIVER 2: entering getPlatformsExt(num_platforms = 0, platforms = (nil), num_platforms_ret = 0x1ffefff8f0)
DRIVER 2: entering getPlatformsExt(num_platforms = 1, platforms = 0x4aae6f8, num_platforms_ret = (nil))
DRIVER 2: entering platformGetFunc(platform = 0x486a048, name = platformCreateDevice)
DRIVER 2: entering platformGetFunc(platform = 0x486a048, name = deviceFunc1)
DRIVER 2: entering platformGetFunc(platform = 0x486a048, name = deviceFunc2)
DRIVER 2: entering platformGetFunc(platform = 0x486a048, name = deviceDestroy)
LAYER 1: entering layerInit(num_entries = 6, target_dispatch = 0x4860160, layer_dispatch = 0x4aaef60)
LAYER 2: entering layerInit(num_entries = 6, target_dispatch = 0x4aaef60, layer_dispatch = 0x4aaf730)
LAYER 2: entering getPlatforms(num_platforms = 0, platforms = (nil), num_platforms_ret = 0x1ffefffa20)
LAYER 1: entering getPlatforms(num_platforms = 0, platforms = (nil), num_platforms_ret = 0x1ffefffa20)
LAYER 1: leaving getPlatforms, result = 0
LAYER 2: leaving getPlatforms, result = 0
Found 2 platforms, err = 0
LAYER 2: entering getPlatforms(num_platforms = 2, platforms = 0x4aaf8b0, num_platforms_ret = (nil))
LAYER 1: entering getPlatforms(num_platforms = 2, platforms = 0x4aaf8b0, num_platforms_ret = (nil))
LAYER 1: leaving getPlatforms, result = 0
LAYER 2: leaving getPlatforms, result = 0
Got platforms, err = 0
INSTANCE LAYER 1: entering layerInit(num_entries = 4, layer_instance_dispatch = 0x4ab0060, layer_data_ret = 0x4ab00a0)
Added instance layer1, err = 0
INSTANCE LAYER 2: entering layerInit(num_entries = 4, layer_instance_dispatch = 0x4ab08c0, layer_data_ret = 0x4ab0900)
Added instance layer2, err = 0
Testing platform 0x486a048
LAYER 2: entering platformCreateDevice(platform = 0x486a048, device_ret = 0x1ffefff9f0)
LAYER 1: entering platformCreateDevice(platform = 0x486a048, device_ret = 0x1ffefff9f0)
INSTANCE LAYER 2: entering platformCreateDevice(platform = 0x486a048, device_ret = 0x1ffefff9f0)
INSTANCE LAYER 1: entering platformCreateDevice(platform = 0x486a048, device_ret = 0x1ffefff9f0)
DRIVER 2: entering platformCreateDevice(platform = 0x486a048, device_ret = 0x1ffefff9f0)
DRIVER 2: allocated device 0x4ab09c0
INSTANCE LAYER 1: leaving platformCreateDevice, result = 0, device_ret_val = 0x4ab09c0
INSTANCE LAYER 2: leaving platformCreateDevice, result = 0, device_ret_val = 0x4ab09c0
LAYER 1: leaving platformCreateDevice, result = 0, device_ret_val = 0x4ab09c0
LAYER 2: leaving platformCreateDevice, result = 0, device_ret_val = 0x4ab09c0
Created device = 0x4ab09c0, err = 0
LAYER 1: entering deviceFunc1(device = 0x4ab09c0, param 0)
INSTANCE LAYER 1: entering deviceFunc1(device = 0x4ab09c0, param 0)
DRIVER 2: entering deviceFunc1(device = 0x4ab09c0, param 0)
INSTANCE LAYER 1: eaving deviceFunc1, result = 0
LAYER 1: leaving deviceFunc1, result = 0
Called deviceFunc1, err = 0
LAYER 2: entering deviceFunc2(device = 0x4ab09c0, param 1)
LAYER 1: entering deviceFunc2(device = 0x4ab09c0, param 1)
INSTANCE LAYER 2: entering deviceFunc2(device = 0x4ab09c0, param 1)
INSTANCE LAYER 1: entering deviceFunc2(device = 0x4ab09c0, param 1)
INSTANCE LAYER 1: leaving deviceFunc2, result = -2
INSTANCE LAYER 2: leaving deviceFunc2, result = -2
LAYER 1: leaving deviceFunc2, result = -2
LAYER 2: leaving deviceFunc2, result = -2
Called deviceFunc2, err = -2
LAYER 2: entering deviceDestroy(device = 0x4ab09c0)
LAYER 1: entering deviceDestroy(device = 0x4ab09c0)
INSTANCE LAYER 2: entering deviceDestroy(device = 0x4ab09c0)
INSTANCE LAYER 1: entering deviceDestroy(device = 0x4ab09c0)
DRIVER 2: entering deviceDestroy(device = 0x4ab09c0)
INSTANCE LAYER 1: leaving deviceDestroy, result = 0
INSTANCE LAYER 2: leaving deviceDestroy, result = 0
LAYER 1: leaving deviceDestroy, result = 0
LAYER 2: leaving deviceDestroy, result = 0
Destroyed device = 0x4ab09c0, err = 0
Testing platform 0x4865048
LAYER 2: entering platformCreateDevice(platform = 0x4865048, device_ret = 0x1ffefff9f0)
LAYER 1: entering platformCreateDevice(platform = 0x4865048, device_ret = 0x1ffefff9f0)
DRIVER 1: entering platformCreateDevice(platform = 0x4865048, device_ret = 0x1ffefff9f0)
DRIVER 1: allocated device 0x4ab0a10
LAYER 1: leaving platformCreateDevice, result = 0, device_ret_val = 0x4ab0a10
LAYER 2: leaving platformCreateDevice, result = 0, device_ret_val = 0x4ab0a10
Created device = 0x4ab0a10, err = 0
LAYER 1: entering deviceFunc1(device = 0x4ab0a10, param 0)
DRIVER 1: entering deviceFunc1(device = 0x4ab0a10, param 0)
LAYER 1: leaving deviceFunc1, result = 0
Called deviceFunc1, err = 0
LAYER 2: entering deviceFunc2(device = 0x4ab0a10, param 1)
LAYER 1: entering deviceFunc2(device = 0x4ab0a10, param 1)
DRIVER 1: entering deviceFunc2(device = 0x4ab0a10, param 1)
LAYER 1: leaving deviceFunc2, result = 0
LAYER 2: leaving deviceFunc2, result = 0
Called deviceFunc2, err = 0
LAYER 2: entering deviceDestroy(device = 0x4ab0a10)
LAYER 1: entering deviceDestroy(device = 0x4ab0a10)
DRIVER 1: entering deviceDestroy(device = 0x4ab0a10)
LAYER 1: leaving deviceDestroy, result = 0
LAYER 2: leaving deviceDestroy, result = 0
Destroyed device = 0x4ab0a10, err = 0
Deiniting loader
INSTANCE LAYER 2: entering layerInstanceDeinit(layer_data = 0x4ab0960)
INSTANCE LAYER 1: entering layerInstanceDeinit(layer_data = 0x4ab0100)
LAYER 1: entering layerDeinit()
```
