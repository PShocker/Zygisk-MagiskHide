#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <android/log.h>
#include <vector>
#include <string>
#include <time.h>
#include <pthread.h>
#include <libgen.h>
#include <string>

using namespace std;

int switch_mnt_ns(int pid) {
    char mnt[32];
    snprintf(mnt, sizeof(mnt), "/proc/%d/ns/mnt", pid);
    if (access(mnt, R_OK) == -1) return 1; // Maybe process died..

    int fd, ret;
    fd = open(mnt, O_RDONLY);
    if (fd < 0) return 1;
    // Switch to its namespace
    ret = setns(fd, 0);
    close(fd);
    return ret;
}

struct mntent *getmntent(FILE *fp, struct mntent *e, char *buf, int buf_len)
{
    memset(e, 0, sizeof(*e));
    while (fgets(buf, buf_len, fp) != nullptr)
    {
        // Entries look like "proc /proc proc rw,nosuid,nodev,noexec,relatime 0 0".
        // That is: mnt_fsname mnt_dir mnt_type mnt_opts 0 0.
        int fsname0, fsname1, dir0, dir1, type0, type1, opts0, opts1;
        if (sscanf(buf, " %n%*s%n %n%*s%n %n%*s%n %n%*s%n %d %d",
                   &fsname0, &fsname1, &dir0, &dir1, &type0, &type1, &opts0, &opts1,
                   &e->mnt_freq, &e->mnt_passno) == 2)
        {
            e->mnt_fsname = &buf[fsname0];
            buf[fsname1] = '\0';
            e->mnt_dir = &buf[dir0];
            buf[dir1] = '\0';
            e->mnt_type = &buf[type0];
            buf[type1] = '\0';
            e->mnt_opts = &buf[opts0];
            buf[opts1] = '\0';
            return e;
        }
    }
    return nullptr;
}

void parse_mnt(const char *file, const function<bool(mntent *)> &fn)
{
    char path[PATH_MAX];
    FILE *fp;

    sprintf(path, "/proc/self/mounts");
    fp = fopen(path, "r");
    if (fp)
    {
        mntent mentry{};
        char buf[4096];
        while (getmntent(fp, &mentry, buf, sizeof(buf)))
        {
            if (!fn(&mentry))
                break;
        }
    }
}