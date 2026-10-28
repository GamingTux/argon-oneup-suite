/*
 * ksystemstats-ns-helper.c
 * Mounts a generated /proc/cpuinfo into a private mount namespace
 * and executes /usr/bin/ksystemstats so KDE System Monitor sees per-core CPU freqs.
 *
 * Compilation:
 *   gcc -O2 -Wall -Wextra -o ksystemstats-ns-helper ksystemstats-ns-helper.c
 */
#define _GNU_SOURCE
#include <sched.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <linux/capability.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void drop_all_caps(void) {
    struct __user_cap_header_struct hdr;
    struct __user_cap_data_struct data[2];
    memset(&hdr, 0, sizeof(hdr));
    memset(data, 0, sizeof(data));
    hdr.version = _LINUX_CAPABILITY_VERSION_3;
    hdr.pid = 0;
    (void)syscall(SYS_capset, &hdr, data);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <fake-cpuinfo-path>\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (lstat(argv[1], &st) != 0 || !S_ISREG(st.st_mode) || st.st_uid != getuid()) {
        fprintf(stderr, "ksystemstats-ns-helper: invalid or untrusted cpuinfo file\n");
        return 1;
    }

    if (unshare(CLONE_NEWNS) != 0) {
        perror("unshare(CLONE_NEWNS)");
        return 1;
    }
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0) {
        perror("mount --make-rprivate /");
        return 1;
    }
    if (mount(argv[1], "/proc/cpuinfo", NULL, MS_BIND | MS_NOSUID | MS_NOEXEC | MS_NODEV, NULL) != 0) {
        perror("mount --bind /proc/cpuinfo");
        return 1;
    }

    drop_all_caps();

    execl("/usr/bin/ksystemstats", "ksystemstats", (char *)NULL);
    perror("execl /usr/bin/ksystemstats");
    return 1;
}
