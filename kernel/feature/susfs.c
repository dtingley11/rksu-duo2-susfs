#include <linux/susfs.h>
#include <linux/susfs_def.h>

#ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
extern void susfs_run_try_umount_for_current_mnt_ns(void);
#endif

#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT
extern bool susfs_is_auto_add_sus_bind_mount_enabled;
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT
extern bool susfs_is_auto_add_sus_ksu_default_mount_enabled;
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT
extern bool susfs_is_auto_add_try_umount_for_bind_mount_enabled;
#endif

static bool susfs_umount_for_zygote_system_process_enabled;

bool susfs_is_current_ksu_domain(void)
{
    return is_ksu_domain();
}

bool susfs_is_current_zygote_domain(void)
{
    return is_zygote(current_cred());
}

bool ksu_susfs_should_umount_zygote_system_process(void)
{
    return susfs_umount_for_zygote_system_process_enabled;
}

static bool ksu_susfs_path_exists(const char *pathname)
{
    struct path path;

    if (kern_path(pathname, 0, &path)) {
        return false;
    }

    path_put(&path);
    return true;
}

void ksu_susfs_on_post_fs_data(void)
{
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
    susfs_umount_for_zygote_system_process_enabled =
        ksu_susfs_path_exists(DATA_ADB_UMOUNT_FOR_ZYGOTE_SYSTEM_PROCESS);
    pr_info("susfs: umount zygote system process: %d\n",
            susfs_umount_for_zygote_system_process_enabled);
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT
    if (ksu_susfs_path_exists(DATA_ADB_NO_AUTO_ADD_SUS_BIND_MOUNT)) {
        susfs_is_auto_add_sus_bind_mount_enabled = false;
    }
    pr_info("susfs: auto add sus bind mount: %d\n",
            susfs_is_auto_add_sus_bind_mount_enabled);
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT
    if (ksu_susfs_path_exists(DATA_ADB_NO_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT)) {
        susfs_is_auto_add_sus_ksu_default_mount_enabled = false;
    }
    pr_info("susfs: auto add KSU default mount: %d\n",
            susfs_is_auto_add_sus_ksu_default_mount_enabled);
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT
    if (ksu_susfs_path_exists(DATA_ADB_NO_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT)) {
        susfs_is_auto_add_try_umount_for_bind_mount_enabled = false;
    }
    pr_info("susfs: auto add bind try_umount: %d\n",
            susfs_is_auto_add_try_umount_for_bind_mount_enabled);
#endif
}

static int ksu_susfs_reply(unsigned long arg5, int error)
{
    if (!arg5) {
        return -EFAULT;
    }

    if (copy_to_user((int __user *)arg5, &error, sizeof(error))) {
        return -EFAULT;
    }

    return 0;
}

static int ksu_susfs_copy_string(unsigned long arg3, unsigned long arg5,
                                 const char *value)
{
    int error = 0;
    size_t size = strlen(value) + 1;

    if (!arg3 || copy_to_user((void __user *)arg3, value, size)) {
        error = -EFAULT;
    }

    return ksu_susfs_reply(arg5, error);
}

static u64 ksu_susfs_enabled_features(void)
{
    u64 features = 0;

#ifdef CONFIG_KSU_SUSFS_SUS_PATH
    features |= 1ULL << 0;
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
    features |= 1ULL << 1;
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT
    features |= 1ULL << 2;
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT
    features |= 1ULL << 3;
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
    features |= 1ULL << 4;
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_OVERLAYFS
    features |= 1ULL << 5;
#endif
#ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
    features |= 1ULL << 6;
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT
    features |= 1ULL << 7;
#endif
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
    features |= 1ULL << 8;
#endif
#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
    features |= 1ULL << 9;
#endif
#ifdef CONFIG_KSU_SUSFS_HIDE_KSU_SUSFS_SYMBOLS
    features |= 1ULL << 10;
#endif
#ifdef CONFIG_KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG
    features |= 1ULL << 11;
#endif
#ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT
    features |= 1ULL << 12;
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_SU
    features |= 1ULL << 13;
#endif
#ifdef CONFIG_KSU_SUSFS_HAS_MAGIC_MOUNT
    features |= 1ULL << 14;
#endif

    return features;
}

static int ksu_susfs_unsupported(unsigned long arg5)
{
    return ksu_susfs_reply(arg5, -EOPNOTSUPP);
}

int ksu_susfs_handle_prctl(int option, unsigned long arg2,
                           unsigned long arg3, unsigned long arg4,
                           unsigned long arg5)
{
    int error = 0;

    (void)arg4;

    if (option != KERNEL_SU_OPTION) {
        return -ENOSYS;
    }

    if (current_uid().val != 0 || !is_ksu_domain()) {
        return -EPERM;
    }

    switch (arg2) {
    case CMD_SUSFS_ADD_SUS_PATH:
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
        error = susfs_add_sus_path((struct st_susfs_sus_path __user *)arg3);
        return ksu_susfs_reply(arg5, error);
#else
        return ksu_susfs_unsupported(arg5);
#endif

    case CMD_SUSFS_ADD_SUS_MOUNT:
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
        error = susfs_add_sus_mount((struct st_susfs_sus_mount __user *)arg3);
        return ksu_susfs_reply(arg5, error);
#else
        return ksu_susfs_unsupported(arg5);
#endif

    case CMD_SUSFS_ADD_SUS_KSTAT:
    case CMD_SUSFS_ADD_SUS_KSTAT_STATICALLY:
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
        error = susfs_add_sus_kstat((struct st_susfs_sus_kstat __user *)arg3);
        return ksu_susfs_reply(arg5, error);
#else
        return ksu_susfs_unsupported(arg5);
#endif

    case CMD_SUSFS_UPDATE_SUS_KSTAT:
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
        error = susfs_update_sus_kstat((struct st_susfs_sus_kstat __user *)arg3);
        return ksu_susfs_reply(arg5, error);
#else
        return ksu_susfs_unsupported(arg5);
#endif

    case CMD_SUSFS_ADD_TRY_UMOUNT:
#ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
        error = susfs_add_try_umount((struct st_susfs_try_umount __user *)arg3);
        return ksu_susfs_reply(arg5, error);
#else
        return ksu_susfs_unsupported(arg5);
#endif

    case CMD_SUSFS_RUN_UMOUNT_FOR_CURRENT_MNT_NS:
#ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
        susfs_run_try_umount_for_current_mnt_ns();
        return ksu_susfs_reply(arg5, 0);
#else
        return ksu_susfs_unsupported(arg5);
#endif

    case CMD_SUSFS_SET_UNAME:
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
        error = susfs_set_uname((struct st_susfs_uname __user *)arg3);
        return ksu_susfs_reply(arg5, error);
#else
        return ksu_susfs_unsupported(arg5);
#endif

    case CMD_SUSFS_ENABLE_LOG:
#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
        if (arg3 > 1) {
            return ksu_susfs_reply(arg5, -EINVAL);
        }
        susfs_set_log(arg3 != 0);
        return ksu_susfs_reply(arg5, 0);
#else
        return ksu_susfs_unsupported(arg5);
#endif

    case CMD_SUSFS_SET_CMDLINE_OR_BOOTCONFIG:
#ifdef CONFIG_KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG
        error = susfs_set_cmdline_or_bootconfig((char __user *)arg3);
        return ksu_susfs_reply(arg5, error);
#else
        return ksu_susfs_unsupported(arg5);
#endif

    case CMD_SUSFS_ADD_OPEN_REDIRECT:
#ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT
        error = susfs_add_open_redirect((struct st_susfs_open_redirect __user *)arg3);
        return ksu_susfs_reply(arg5, error);
#else
        return ksu_susfs_unsupported(arg5);
#endif

    case CMD_SUSFS_SHOW_VERSION:
        return ksu_susfs_copy_string(arg3, arg5, SUSFS_VERSION);

    case CMD_SUSFS_SHOW_ENABLED_FEATURES: {
        u64 features = ksu_susfs_enabled_features();

        if (!arg3 || copy_to_user((u64 __user *)arg3, &features,
                                  sizeof(features))) {
            error = -EFAULT;
        }
        return ksu_susfs_reply(arg5, error);
    }

    case CMD_SUSFS_SHOW_VARIANT:
        return ksu_susfs_copy_string(arg3, arg5, SUSFS_VARIANT);

    case CMD_SUSFS_SUS_SU:
    case CMD_SUSFS_SHOW_SUS_SU_WORKING_MODE:
    case CMD_SUSFS_IS_SUS_SU_READY:
        return ksu_susfs_unsupported(arg5);

    default:
        return -ENOSYS;
    }
}

void __init ksu_susfs_init(void)
{
    susfs_init();
}
