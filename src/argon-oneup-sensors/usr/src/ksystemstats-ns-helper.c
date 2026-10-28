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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <fake-cpuinfo-path>\n", argv[0]);
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
    if (mount(argv[1], "/proc/cpuinfo", NULL, MS_BIND, NULL) != 0) {
        perror("mount --bind /proc/cpuinfo");
        return 1;
    }
    execl("/usr/bin/ksystemstats", "ksystemstats", (char *)NULL);
    perror("execl /usr/bin/ksystemstats");
    return 1;
}
