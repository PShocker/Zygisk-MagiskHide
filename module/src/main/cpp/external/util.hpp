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
#include <sys/mount.h>

using namespace std;

#define getline       compat_getline

static inline bool str_starts(std::string_view s, std::string_view ss) {
    return s.size() >= ss.size() && s.compare(0, ss.size(), ss) == 0;
}

ssize_t compat_getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp) {
    char *ptr, *eptr;

    if (*buf == nullptr || *bufsiz == 0) {
        *bufsiz = BUFSIZ;
        if ((*buf = (char *) malloc(*bufsiz)) == nullptr)
            return -1;
    }

    for (ptr = *buf, eptr = *buf + *bufsiz;;) {
        int c = fgetc(fp);
        if (c == -1) {
            if (feof(fp))
                return ptr == *buf ? -1 : ptr - *buf;
            else
                return -1;
        }
        *ptr++ = c;
        if (c == delimiter) {
            *ptr = '\0';
            return ptr - *buf;
        }
        if (ptr + 2 >= eptr) {
            char *nbuf;
            size_t nbufsiz = *bufsiz * 2;
            ssize_t d = ptr - *buf;
            if ((nbuf = (char *) realloc(*buf, nbufsiz)) == nullptr)
                return -1;
            *buf = nbuf;
            *bufsiz = nbufsiz;
            eptr = nbuf + nbufsiz;
            ptr = nbuf + d;
        }
    }
}

ssize_t compat_getline(char **buf, size_t *bufsiz, FILE *fp) {
    return compat_getdelim(buf, bufsiz, '\n', fp);
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

void file_readline(bool trim, const char *file, const function<bool(string_view)> &fn) {
    FILE *fp = fopen(file, "re");
    if (fp == nullptr)
        return;
    size_t len = 1024;
    char *buf = (char *) malloc(len);
    char *start;
    ssize_t read;
    while ((read = getline(&buf, &len, fp)) >= 0) {
        start = buf;
        if (trim) {
            while (read && "\n\r "sv.find(buf[read - 1]) != string::npos)
                --read;
            buf[read] = '\0';
            while (*start == ' ')
                ++start;
        }
        if (!fn(start))
            break;
    }
    fclose(fp);
    free(buf);
}

void parse_prop_file(const char *file, const function<bool(string_view, string_view)> &fn) {
    file_readline(true, file, [&](string_view line_view) -> bool {
        char *line = (char *) line_view.data();
        if (line[0] == '#')
            return true;
        char *eql = strchr(line, '=');
        if (eql == nullptr || eql == line)
            return true;
        *eql = '\0';
        return fn(line, eql + 1);
    });
}