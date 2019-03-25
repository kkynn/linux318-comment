#include <linux/mtd/physmap.h>
#include <linux/platform_device.h>

#define PART0_OFFSET (0)
#define PART0_SIZE (1024*1024)
#define PART1_OFFSET (PART0_OFFSET + PART0_SIZE)
#define PART1_SIZE (1024*1024)

static struct mtd_partition luna_snof_g3_mtd_partition_sample[] = {
	{
		.name = "mtd_partition_example",
		.offset =	PART0_OFFSET,
		.size =	PART0_SIZE,
		.mask_flags =	0,
	}, {
		.name = "customize_partition_if_favor_hardcode",
		.offset = PART1_OFFSET,
		.size = PART1_SIZE,
		.mask_flags = 0
	}
};

static struct physmap_flash_data luna_snof_g3_data = {
	.width = 4,
	.nr_parts	= ARRAY_SIZE(luna_snof_g3_mtd_partition_sample),
	.parts = luna_snof_g3_mtd_partition_sample
};

static struct resource luna_snof_g3_resource = {
	.start = (NORSF_CFLASH_BASE & 0x3fffffff),
	.end = 0x17ffffff,
	.flags = IORESOURCE_MEM
};

static struct platform_device luna_snof_g3_device = {
	.name = "physmap-flash",
	.id = -1, /* This has to be -1, otherwise MTD appends it to name, e.g., physmap-flash.0 */
	.dev = {
		.platform_data = &luna_snof_g3_data,
	},
	.num_resources = 1,
	.resource	= &luna_snof_g3_resource,
};

static int __init luna_snof_g3_add_devices(void) {
	int err;

	err = platform_device_register(&luna_snof_g3_device);
	if (err) {
		platform_device_unregister(&luna_snof_g3_device);
	}
  return err;
}

late_initcall(luna_snof_g3_add_devices);
