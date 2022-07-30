#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <mntent.h>
#include <sys/mount.h>

#include "util.hpp"
#include "log.hpp"
#include "magisk.hpp"

#define TMPFS_MNT(dir) (mentry->mnt_type == "tmpfs"sv && \
                        strncmp(mentry->mnt_dir, "/" #dir, sizeof("/" #dir) - 1) == 0)

using namespace std;

static void lazy_unmount(const char *mountpoint)
{
    if (umount2(mountpoint, MNT_DETACH) != -1)
        LOGD("hide: Unmounted (%s)\n", mountpoint);
}

void do_hide(int pid)
{
    if (pid > 0 && switch_mnt_ns(pid))
        return;

    char buf[64];
    readlink("/proc/self/exe", buf, sizeof(buf));
    string MAGISKTMP = dirname(buf);

    lazy_unmount(MAGISKTMP.data());

    parse_mnt("/proc/self/mounts", [&](mntent *mentry) {
        if (TMPFS_MNT(system) || TMPFS_MNT(vendor) || TMPFS_MNT(product) || TMPFS_MNT(system_ext))
        {
            lazy_unmount(mentry->mnt_dir);
        }
        return true;
    });

    // Unmount all Magisk created mounts
    parse_mnt("/proc/self/mounts", [&](mntent *mentry) {
        if (strstr(mentry->mnt_fsname, BLOCKDIR))
        {
            lazy_unmount(mentry->mnt_dir);
        }
        return true;
    });
    return;
}