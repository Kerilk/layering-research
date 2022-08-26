#if FFI_INSTANCE_LAYERS
#include <ffi.h>

struct platformCreateDevice_ffi_args {
	platform_t  *p_platform;
	device_t   **p_device_ret;
};
static const unsigned int platformCreateDevice_ffi_nargs = 2;
static ffi_type *platformCreateDevice_ffi_types[] = {
	&ffi_type_pointer,
	&ffi_type_pointer
};
static ffi_type *platformCreateDevice_ffi_ret = &ffi_type_sint;
static void platformCreateDevice_ffi(
	ffi_cif                              *cif,
	int                                  *ffi_ret,
	struct platformCreateDevice_ffi_args *args,
	void                                 *data);

struct deviceFunc1_ffi_args {
	device_t *p_device;
	int      *p_param;
};
static const unsigned int deviceFunc1_ffi_nargs = 2;
static ffi_type *deviceFunc1_ffi_types[] = {
	&ffi_type_pointer,
	&ffi_type_sint
};
static ffi_type *deviceFunc1_ffi_ret = &ffi_type_sint;
static void deviceFunc1_ffi(
	ffi_cif                     *cif,
	int                         *ffi_ret,
	struct deviceFunc1_ffi_args *args,
	void                        *data);

struct deviceFunc2_ffi_args {
	device_t *p_device;
	int      *p_param;
};
static const unsigned int deviceFunc2_ffi_nargs = 2;
static ffi_type *deviceFunc2_ffi_types[] = {
	&ffi_type_pointer,
	&ffi_type_sint
};
static ffi_type *deviceFunc2_ffi_ret = &ffi_type_sint;
static void deviceFunc2_ffi(
	ffi_cif                     *cif,
	int                         *ffi_ret,
	struct deviceFunc2_ffi_args *args,
	void                        *data);

struct deviceDestroy_ffi_args {
	device_t *p_device;
};
static const unsigned int deviceDestroy_ffi_nargs = 1;
static ffi_type *deviceDestroy_ffi_types[] = {
	&ffi_type_pointer
};
static ffi_type *deviceDestroy_ffi_ret = &ffi_type_sint;
static void deviceDestroy_ffi(
	ffi_cif                       *cif,
	int                           *ffi_ret,
	struct deviceDestroy_ffi_args *args,
	void                          *data);

#endif //FFI_INSTANCE_LAYERS
