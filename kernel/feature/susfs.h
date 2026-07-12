#ifndef __KSU_H_SUSFS
#define __KSU_H_SUSFS

#define KERNEL_SU_OPTION 0xDEADBEEF

int ksu_susfs_handle_prctl(int option, unsigned long arg2,
                           unsigned long arg3, unsigned long arg4,
                           unsigned long arg5);
void __init ksu_susfs_init(void);
void ksu_susfs_on_post_fs_data(void);
bool ksu_susfs_should_umount_zygote_system_process(void);

/* Referenced directly by the SUSFS kernel hooks. */
bool susfs_is_current_ksu_domain(void);
bool susfs_is_current_zygote_domain(void);

#endif
