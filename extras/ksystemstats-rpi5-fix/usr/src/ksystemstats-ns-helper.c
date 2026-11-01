/*
 * ksystemstats-ns-helper.c
 * Mounts a generated /proc/cpuinfo into a private mount namespace
 * and executes /usr/bin/ksystemstats so KDE System Monitor sees per-core CPU freqs.
 *
 * Compilation:
 *   gcc -O2 -Wall -Wextra -o ksystemstats-ns-helper ksystemstats-ns-helper.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <sched.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <linux/capability.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int drop_all_caps(void) {
    gid_t rgid = getgid();
    uid_t ruid = getuid();
    if (setresgid(rgid, rgid, rgid) != 0 || setresuid(ruid, ruid, ruid) != 0) {
        return -1;
    }
    struct __user_cap_header_struct hdr;
    struct __user_cap_data_struct data[2];
    memset(&hdr, 0, sizeof(hdr));
    memset(data, 0, sizeof(data));
    hdr.version = _LINUX_CAPABILITY_VERSION_3;
    hdr.pid = 0;
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
        return -1;
    }
    return (int)syscall(SYS_capset, &hdr, data);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <fake-cpuinfo-path>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "ksystemstats-ns-helper: cannot open cpuinfo file\n");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_uid != getuid() ||
        (st.st_mode & (S_IWGRP | S_IWOTH)) != 0) {
        fprintf(stderr, "ksystemstats-ns-helper: invalid or untrusted cpuinfo file\n");
        close(fd);
        return 1;
    }

    if (unshare(CLONE_NEWNS) != 0) {
        perror("unshare(CLONE_NEWNS)");
        close(fd);
        return 1;
    }
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0) {
        perror("mount --make-rprivate /");
        close(fd);
        return 1;
    }

    char fd_path[64];
    snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", fd);
    if (mount(fd_path, "/proc/cpuinfo", NULL, MS_BIND | MS_NOSUID | MS_NOEXEC | MS_NODEV, NULL) != 0) {
        perror("mount --bind /proc/cpuinfo");
        close(fd);
        return 1;
    }
    close(fd);

    if (drop_all_caps() != 0) {
        perror("drop_all_caps");
        return 1;
    }

    execl("/usr/bin/ksystemstats", "ksystemstats", (char *)NULL);
    perror("execl /usr/bin/ksystemstats");
    return 1;
}
