#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x87a14630, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x5bed0a7d, __VMLINUX_SYMBOL_STR(i2c_del_driver) },
	{ 0x25574e4, __VMLINUX_SYMBOL_STR(i2c_register_driver) },
	{ 0xa671b895, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xa9d13d0a, __VMLINUX_SYMBOL_STR(sysfs_create_group) },
	{ 0xdeffe049, __VMLINUX_SYMBOL_STR(kobject_create_and_add) },
	{ 0x246a7405, __VMLINUX_SYMBOL_STR(i2c_smbus_write_byte_data) },
	{ 0x9d669763, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0xa9795959, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x7d11c268, __VMLINUX_SYMBOL_STR(jiffies) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x1c11d5a1, __VMLINUX_SYMBOL_STR(i2c_smbus_read_byte_data) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
	{ 0x8b98f67b, __VMLINUX_SYMBOL_STR(kobject_put) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "32B9D0F11B59656413E868A");
