struct platform_s {
	struct multiplex_s *multiplex;
};

struct device_s {
	struct multiplex_s *multiplex;
};

typedef struct platform_s * platform_t;
typedef struct device_s * device_t;

int
getPlatformsExt(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret);

void *
platformGetFunc(platform_t platform, const char *name);
