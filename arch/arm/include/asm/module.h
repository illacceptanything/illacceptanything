#ifndef _ASM_ARM_MODULE_H
#define _ASM_ARM_MODULE_H

#define Elf_Shdr	Elf32_Shdr
#define Elf_Sym		Elf32_Sym
#define Elf_Ehdr	Elf32_Ehdr

struct unwind_table;

#ifdef CONFIG_ARM_UNWIND
struct arm_unwind_mapping {
	Elf_Shdr *unw_sec;
	Elf_Shdr *sec_text;
	struct unwind_table *unwind;
};
enum {
	ARM_SEC_INIT,
	ARM_SEC_DEVINIT,
	ARM_SEC_CORE,
	ARM_SEC_EXIT,
	ARM_SEC_DEVEXIT,
	ARM_SEC_MAX,
};
struct mod_arch_specific {
	struct arm_unwind_mapping map[ARM_SEC_MAX];
};
#else
struct mod_arch_specific {
};
#endif

/*
 * Include the ARM architecture version.
 */
#define MODULE_ARCH_VERMAGIC	"ARMv" __stringify(__LINUX_ARM_ARCH__) " "

#endif /* _ASM_ARM_MODULE_H */
