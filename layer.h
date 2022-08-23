typedef int layerInit_t(
		size_t              num_entries,
		struct dispatch_s  *target_dispatch,
		size_t             *num_entries_ret,
		struct dispatch_s **layer_dispatch_ret);

typedef layerInit_t *pfn_layerInit_t;

#define NUM_DISPATCH_ENTRIES (sizeof(struct dispatch_s)/sizeof(pfn_layerInit_t))
