#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <mntent.h>
#include <sys/statvfs.h>
#include <time.h>
#include <sys/time.h>

#define MAX_DISKS 8

typedef struct {
    char name[64];
    char mount[128];
    char fsname[64];
    char icon[32];
    double used_gb;
    double total_gb;
    int percent;
} DiskInfo;

typedef struct {
    int cores_mhz[4];
    int cores_load[4];
    int total_load;
    int max_mhz;
    float cpu_temp_c;
    char governor[32];

    int gpu_percent;
    char gpu_name[32];

    int bat_present;
    int bat_capacity;
    char bat_status[32];
    double bat_power_w;
    double bat_voltage_v;
    long bat_current_ma;
    float bat_temp_c;
    long bat_cycle_count;
    char bat_health[32];
    int ac_online;

    long mem_total_mb;
    long mem_used_mb;
    int mem_percent;

    DiskInfo disks[MAX_DISKS];
    int num_disks;

    float nvme_temp_c;

    int fan_rpm;
    int fan_pwm;
    int fan_percent;

    char profile[32];
    double query_ms;
} Telemetry;

static long read_int(const char *path, long def_val) {
    FILE *f = fopen(path, "r");
    if (!f) return def_val;
    long val = def_val;
    if (fscanf(f, "%ld", &val) != 1) val = def_val;
    fclose(f);
    return val;
}

static void read_str(const char *path, char *buf, size_t maxlen, const char *def_val) {
    FILE *f = fopen(path, "r");
    if (!f) {
        snprintf(buf, maxlen, "%s", def_val);
        return;
    }
    if (fgets(buf, maxlen, f)) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r' || buf[len - 1] == ' ')) {
            buf[len - 1] = '\0';
            len--;
        }
    } else {
        snprintf(buf, maxlen, "%s", def_val);
    }
    fclose(f);
}

static void get_cpu_loads(Telemetry *t) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return;

    unsigned long long cur_total[4] = {0}, cur_active[4] = {0};
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        int idx = -1;
        unsigned long long user = 0, nice = 0, sys = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;
        if (sscanf(line, "cpu%d %llu %llu %llu %llu %llu %llu %llu %llu",
                   &idx, &user, &nice, &sys, &idle, &iowait, &irq, &softirq, &steal) >= 5) {
            if (idx >= 0 && idx < 4) {
                unsigned long long idle_all = idle + iowait;
                unsigned long long active = user + nice + sys + irq + softirq + steal;
                cur_total[idx] = active + idle_all;
                cur_active[idx] = active;
            }
        }
    }
    fclose(f);

    unsigned long long prev_total[4] = {0}, prev_active[4] = {0};
    FILE *pf = fopen("/dev/shm/argon_cpu_prev", "r");
    if (pf) {
        for (int i = 0; i < 4; i++) {
            if (fscanf(pf, "%llu %llu", &prev_total[i], &prev_active[i]) != 2) {
                prev_total[i] = 0;
                prev_active[i] = 0;
            }
        }
        fclose(pf);
    }

    int sum_load = 0;
    for (int i = 0; i < 4; i++) {
        int pct = 0;
        if (prev_total[i] > 0 && cur_total[i] > prev_total[i] && cur_active[i] >= prev_active[i]) {
            unsigned long long d_tot = cur_total[i] - prev_total[i];
            unsigned long long d_act = cur_active[i] - prev_active[i];
            if (d_tot > 0) {
                pct = (int)((d_act * 100ULL + (d_tot / 2ULL)) / d_tot);
                if (pct > 100) pct = 100;
            }
        }
        t->cores_load[i] = pct;
        sum_load += pct;
    }
    t->total_load = (sum_load + 2) / 4;

    pf = fopen("/dev/shm/argon_cpu_prev", "w");
    if (pf) {
        for (int i = 0; i < 4; i++) {
            fprintf(pf, "%llu %llu\n", cur_total[i], cur_active[i]);
        }
        fclose(pf);
    }
}

static int get_gpu_load(void) {
    FILE *f = fopen("/sys/devices/platform/axi/1002000000.v3d/gpu_stats", "r");
    if (!f) return 0;

    char line[128];
    long long cur_ts = 0, cur_rt = 0;
    while (fgets(line, sizeof(line), f)) {
        char q[32];
        long long t, jobs, rt;
        if (sscanf(line, "%31s %lld %lld %lld", q, &t, &jobs, &rt) == 4) {
            cur_ts = t;
            if (strcmp(q, "bin") == 0 || strcmp(q, "render") == 0 || strcmp(q, "tfu") == 0) {
                cur_rt += rt;
            }
        }
    }
    fclose(f);

    long long prev_ts = 0, prev_rt = 0;
    FILE *pf = fopen("/dev/shm/argon_gpu_prev", "r");
    if (pf) {
        if (fscanf(pf, "%lld %lld", &prev_ts, &prev_rt) != 2) {
            prev_ts = 0;
            prev_rt = 0;
        }
        fclose(pf);
    }

    int percent = 0;
    if (prev_ts > 0 && cur_ts > prev_ts && cur_rt >= prev_rt) {
        long long d_ts = cur_ts - prev_ts;
        long long d_rt = cur_rt - prev_rt;
        if (d_ts > 0) {
            percent = (int)((d_rt * 100) / d_ts);
            if (percent > 100) percent = 100;
        }
    }

    pf = fopen("/dev/shm/argon_gpu_prev", "w");
    if (pf) {
        fprintf(pf, "%lld %lld\n", cur_ts, cur_rt);
        fclose(pf);
    }
    return percent;
}

static void collect_disks(Telemetry *t) {
    FILE *f = setmntent("/proc/mounts", "r");
    if (!f) return;

    struct mntent *ent;
    while ((ent = getmntent(f)) != NULL && t->num_disks < MAX_DISKS) {
        if (strncmp(ent->mnt_fsname, "/dev/", 5) != 0) continue;
        if (strncmp(ent->mnt_fsname, "/dev/loop", 9) == 0) continue;
        if (strncmp(ent->mnt_fsname, "/dev/zram", 9) == 0) continue;
        if (strcmp(ent->mnt_dir, "/boot/firmware") == 0) continue;
        if (strcmp(ent->mnt_dir, "/boot") == 0) continue;

        // Avoid duplicate Btrfs subvolumes on the same root device
        int already_added = 0;
        for (int i = 0; i < t->num_disks; i++) {
            if (strcmp(t->disks[i].fsname, ent->mnt_fsname) == 0) {
                already_added = 1;
                break;
            }
        }
        if (already_added) continue;

        struct statvfs s;
        if (statvfs(ent->mnt_dir, &s) == 0 && s.f_blocks > 0) {
            DiskInfo *d = &t->disks[t->num_disks];
            snprintf(d->fsname, sizeof(d->fsname), "%s", ent->mnt_fsname);
            snprintf(d->mount, sizeof(d->mount), "%s", ent->mnt_dir);

            if (strncmp(ent->mnt_fsname, "/dev/nvme", 9) == 0) {
                snprintf(d->name, sizeof(d->name), "NVMe SSD");
                snprintf(d->icon, sizeof(d->icon), "drive-harddisk");
            } else if (strncmp(ent->mnt_fsname, "/dev/mmcblk", 11) == 0) {
                snprintf(d->name, sizeof(d->name), "SD-Karte");
                snprintf(d->icon, sizeof(d->icon), "media-flash-sd");
            } else if (strncmp(ent->mnt_fsname, "/dev/sd", 7) == 0) {
                const char *base = strrchr(ent->mnt_dir, '/');
                if (base && strlen(base) > 1 && strcmp(ent->mnt_dir, "/media") != 0) {
                    snprintf(d->name, sizeof(d->name), "USB: %s", base + 1);
                } else {
                    snprintf(d->name, sizeof(d->name), "USB-Stick");
                }
                snprintf(d->icon, sizeof(d->icon), "drive-removable-media-usb");
            } else {
                snprintf(d->name, sizeof(d->name), "Laufwerk (%s)", ent->mnt_dir);
                snprintf(d->icon, sizeof(d->icon), "drive-harddisk");
            }

            d->total_gb = (double)s.f_blocks * s.f_frsize / (1024.0 * 1024.0 * 1024.0);
            double free_gb = (double)s.f_bavail * s.f_frsize / (1024.0 * 1024.0 * 1024.0);
            d->used_gb = (d->total_gb > free_gb) ? (d->total_gb - free_gb) : 0.0;
            d->percent = (d->total_gb > 0) ? (int)((d->used_gb * 100.0) / d->total_gb) : 0;
            t->num_disks++;
        }
    }
    endmntent(f);
}

static void collect_telemetry(Telemetry *t) {
    struct timeval tv0, tv1;
    gettimeofday(&tv0, NULL);

    memset(t, 0, sizeof(*t));

    // 1. CPU Cores, Load & Governor
    t->max_mhz = (int)(read_int("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", 2400000) / 1000);
    char path[128];
    for (int i = 0; i < 4; i++) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", i);
        long val = read_int(path, 0);
        t->cores_mhz[i] = (val > 0) ? (int)(val / 1000) : t->max_mhz;
    }
    get_cpu_loads(t);
    t->cpu_temp_c = read_int("/sys/class/thermal/thermal_zone0/temp", 0) / 1000.0f;
    read_str("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", t->governor, sizeof(t->governor), "ondemand");

    // 2. GPU Load (V3D)
    t->gpu_percent = get_gpu_load();
    snprintf(t->gpu_name, sizeof(t->gpu_name), "VideoCore VII (V3D)");

    // 3. Fan & NVMe Hwmon
    DIR *dir = opendir("/sys/class/hwmon");
    if (dir) {
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            if (de->d_name[0] == '.') continue;
            char npath[128], name[64];
            snprintf(npath, sizeof(npath), "/sys/class/hwmon/%s/name", de->d_name);
            read_str(npath, name, sizeof(name), "");
            if (strcmp(name, "pwmfan") == 0) {
                snprintf(path, sizeof(path), "/sys/class/hwmon/%s/fan1_input", de->d_name);
                t->fan_rpm = (int)read_int(path, 0);
                snprintf(path, sizeof(path), "/sys/class/hwmon/%s/pwm1", de->d_name);
                t->fan_pwm = (int)read_int(path, 0);
                t->fan_percent = (int)((t->fan_pwm * 100 + 127) / 255);
            } else if (strcmp(name, "nvme") == 0) {
                snprintf(path, sizeof(path), "/sys/class/hwmon/%s/temp1_input", de->d_name);
                t->nvme_temp_c = read_int(path, 0) / 1000.0f;
            }
        }
        closedir(dir);
    }

    // 4. Battery (CW2217 via oneUpPower)
    if (access("/sys/class/power_supply/BAT0", F_OK) == 0) {
        t->bat_present = 1;
        t->bat_capacity = (int)read_int("/sys/class/power_supply/BAT0/capacity", 0);
        read_str("/sys/class/power_supply/BAT0/status", t->bat_status, sizeof(t->bat_status), "Unknown");
        long p_now = read_int("/sys/class/power_supply/BAT0/power_now", 0);
        t->bat_power_w = (p_now < 0 ? -p_now : p_now) / 1000000.0;
        t->bat_voltage_v = read_int("/sys/class/power_supply/BAT0/voltage_now", 0) / 1000000.0;
        t->bat_current_ma = read_int("/sys/class/power_supply/BAT0/current_now", 0) / 1000;
        t->bat_temp_c = read_int("/sys/class/power_supply/BAT0/temp", 0) / 10.0f;
        t->bat_cycle_count = read_int("/sys/class/power_supply/BAT0/cycle_count", 0);
        read_str("/sys/class/power_supply/BAT0/health", t->bat_health, sizeof(t->bat_health), "Good");
    }

    // AC Online check
    dir = opendir("/sys/class/power_supply");
    if (dir) {
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            if (strncmp(de->d_name, "AC", 2) == 0) {
                snprintf(path, sizeof(path), "/sys/class/power_supply/%s/online", de->d_name);
                if (read_int(path, 0) == 1) t->ac_online = 1;
            }
        }
        closedir(dir);
    }

    // 5. Memory (/proc/meminfo)
    FILE *mf = fopen("/proc/meminfo", "r");
    if (mf) {
        char line[128];
        long total_kb = 0, avail_kb = 0;
        while (fgets(line, sizeof(line), mf)) {
            if (sscanf(line, "MemTotal: %ld kB", &total_kb) == 1) {}
            else if (sscanf(line, "MemAvailable: %ld kB", &avail_kb) == 1) {}
        }
        fclose(mf);
        t->mem_total_mb = total_kb / 1024;
        long used_kb = (total_kb > avail_kb) ? (total_kb - avail_kb) : 0;
        t->mem_used_mb = used_kb / 1024;
        t->mem_percent = (total_kb > 0) ? (int)((used_kb * 100) / total_kb) : 0;
    }

    // 6. Disks & Mounts
    collect_disks(t);

    // 7. Active Profile
    snprintf(t->profile, sizeof(t->profile), "balanced");
    FILE *cf = fopen("/var/lib/argon-power-profiles/config.json", "r");
    if (cf) {
        char cbuf[512];
        size_t n = fread(cbuf, 1, sizeof(cbuf) - 1, cf);
        cbuf[n] = '\0';
        fclose(cf);
        char *p = strstr(cbuf, "\"active_profile\":");
        if (p) {
            char val[32];
            if (sscanf(p + 17, " \"%31[^\"]\"", val) == 1) {
                snprintf(t->profile, sizeof(t->profile), "%s", val);
            }
        }
    }

    gettimeofday(&tv1, NULL);
    t->query_ms = ((tv1.tv_sec - tv0.tv_sec) * 1000.0) + ((tv1.tv_usec - tv0.tv_usec) / 1000.0);
}

static void print_json(const Telemetry *t) {
    printf("{\n");
    printf("  \"model\": \"Argon ONE UP Laptop (CM5)\",\n");
    printf("  \"cpu\": {\n");
    printf("    \"cores_mhz\": [\n");
    printf("      %d,\n      %d,\n      %d,\n      %d\n",
           t->cores_mhz[0], t->cores_mhz[1], t->cores_mhz[2], t->cores_mhz[3]);
    printf("    ],\n");
    printf("    \"cores_load\": [\n");
    printf("      %d,\n      %d,\n      %d,\n      %d\n",
           t->cores_load[0], t->cores_load[1], t->cores_load[2], t->cores_load[3]);
    printf("    ],\n");
    printf("    \"total_load\": %d,\n", t->total_load);
    printf("    \"max_mhz\": %d,\n", t->max_mhz);
    printf("    \"temp_c\": %.1f,\n", t->cpu_temp_c);
    printf("    \"governor\": \"%s\"\n", t->governor);
    printf("  },\n");
    printf("  \"gpu\": {\n");
    printf("    \"percent\": %d,\n", t->gpu_percent);
    printf("    \"name\": \"%s\"\n", t->gpu_name);
    printf("  },\n");
    printf("  \"battery\": {\n");
    printf("    \"present\": %s,\n", t->bat_present ? "true" : "false");
    printf("    \"capacity\": %d,\n", t->bat_capacity);
    printf("    \"status\": \"%s\",\n", t->bat_status);
    printf("    \"power_w\": %.2f,\n", t->bat_power_w);
    printf("    \"voltage_v\": %.2f,\n", t->bat_voltage_v);
    printf("    \"current_ma\": %ld,\n", t->bat_current_ma);
    printf("    \"temp_c\": %.1f,\n", t->bat_temp_c);
    printf("    \"cycle_count\": %ld,\n", t->bat_cycle_count);
    printf("    \"health\": \"%s\",\n", t->bat_health);
    printf("    \"ac_online\": %s\n", t->ac_online ? "true" : "false");
    printf("  },\n");
    printf("  \"memory\": {\n");
    printf("    \"used_mb\": %ld,\n", t->mem_used_mb);
    printf("    \"total_mb\": %ld,\n", t->mem_total_mb);
    printf("    \"percent\": %d\n", t->mem_percent);
    printf("  },\n");
    printf("  \"disks\": [\n");
    for (int i = 0; i < t->num_disks; i++) {
        const DiskInfo *d = &t->disks[i];
        printf("    {\n");
        printf("      \"name\": \"%s\",\n", d->name);
        printf("      \"mount\": \"%s\",\n", d->mount);
        printf("      \"icon\": \"%s\",\n", d->icon);
        printf("      \"used_gb\": %.1f,\n", d->used_gb);
        printf("      \"total_gb\": %.1f,\n", d->total_gb);
        printf("      \"percent\": %d\n", d->percent);
        printf("    }%s\n", (i + 1 < t->num_disks) ? "," : "");
    }
    printf("  ],\n");
    printf("  \"temperatures\": {\n");
    printf("    \"cpu_c\": %.1f,\n", t->cpu_temp_c);
    printf("    \"battery_c\": %.1f,\n", t->bat_temp_c);
    printf("    \"nvme_c\": %.1f\n", t->nvme_temp_c);
    printf("  },\n");
    printf("  \"fan\": {\n");
    printf("    \"rpm\": %d,\n", t->fan_rpm);
    printf("    \"pwm\": %d,\n", t->fan_pwm);
    printf("    \"percent\": %d\n", t->fan_percent);
    printf("  },\n");
    printf("  \"profile\": \"%s\",\n", t->profile);
    printf("  \"query_ms\": %.2f\n", t->query_ms);
    printf("}\n");
}

static void print_dashboard(const Telemetry *t) {
    const char *BOLD = "\033[1m";
    const char *CYAN = "\033[36m";
    const char *GREEN = "\033[32m";
    const char *YELLOW = "\033[33m";
    const char *RED = "\033[31m";
    const char *MAGENTA = "\033[35m";
    const char *RESET = "\033[0m";

    const char *p_color = (strcmp(t->profile, "power-saver") == 0) ? GREEN :
                          ((strcmp(t->profile, "balanced") == 0) ? CYAN : MAGENTA);

    char prof_upper[32];
    snprintf(prof_upper, sizeof(prof_upper), "%s", t->profile);
    for (int i = 0; prof_upper[i]; i++) {
        if (prof_upper[i] >= 'a' && prof_upper[i] <= 'z') prof_upper[i] -= 32;
    }

    printf("\n%s%s╔════════════════════════════════════════════════════════════════════════════╗%s\n", BOLD, CYAN, RESET);
    printf("%s%s║%s  %sARGON ONE UP HARDWARE DASHBOARD%s                  %sProfile:%s %s%s%-14s%s%s%s║%s\n",
           BOLD, CYAN, RESET, BOLD, RESET, CYAN, RESET, p_color, BOLD, prof_upper, RESET, BOLD, CYAN, RESET);
    printf("%s%s╠════════════════════════════════════════════════════════════════════════════╣%s\n", BOLD, CYAN, RESET);

    // CPU Cores & GPU
    char cores_buf[80];
    snprintf(cores_buf, sizeof(cores_buf), "C0:%d%% C1:%d%% C2:%d%% C3:%d%% @ %d MHz",
             t->cores_load[0], t->cores_load[1], t->cores_load[2], t->cores_load[3], t->cores_mhz[0]);
    printf("%s%s║%s  %sCPU:%s     %-44s (Max: %d MHz) %s%s║%s\n",
           BOLD, CYAN, RESET, BOLD, RESET, cores_buf, t->max_mhz, BOLD, CYAN, RESET);
    printf("%s%s║%s  %sGPU:%s     %-25s  Auslastung: %2d%%   Governor: %-10s  %s%s║%s\n",
           BOLD, CYAN, RESET, BOLD, RESET, t->gpu_name, t->gpu_percent, t->governor, BOLD, CYAN, RESET);
    printf("%s%s╠════════════════════════════════════════════════════════════════════════════╣%s\n", BOLD, CYAN, RESET);

    // Battery
    if (t->bat_present) {
        const char *b_color = (t->bat_capacity > 40) ? GREEN : ((t->bat_capacity > 15) ? YELLOW : RED);
        const char *ac_str = t->ac_online ? "[AC Netzteil]" : "[Akku]";
        printf("%s%s║%s  %sAkku:%s    %s%d%%%s (%s) %-14s  Leistung: %.2f W           %s%s║%s\n",
               BOLD, CYAN, RESET, BOLD, RESET, b_color, t->bat_capacity, RESET, t->bat_status, ac_str, t->bat_power_w, BOLD, CYAN, RESET);
        printf("%s%s║%s  %sPack:%s    %.2f V  |  %ld mA   |  Zyklen: %ld  |  Zustand: %-8s  %s%s║%s\n",
               BOLD, CYAN, RESET, BOLD, RESET, t->bat_voltage_v, t->bat_current_ma, t->bat_cycle_count, t->bat_health, BOLD, CYAN, RESET);
    } else {
        printf("%s%s║%s  %sAkku:%s    Nicht vorhanden (Desktop-Betrieb)                                 %s%s║%s\n",
               BOLD, CYAN, RESET, BOLD, RESET, BOLD, CYAN, RESET);
    }
    printf("%s%s╠════════════════════════════════════════════════════════════════════════════╣%s\n", BOLD, CYAN, RESET);

    // Speicher
    printf("%s%s║%s  %sRAM:%s     %ld MB / %ld MB (%d%%)                                           %s%s║%s\n",
           BOLD, CYAN, RESET, BOLD, RESET, t->mem_used_mb, t->mem_total_mb, t->mem_percent, BOLD, CYAN, RESET);
    for (int i = 0; i < t->num_disks; i++) {
        const DiskInfo *d = &t->disks[i];
        printf("%s%s║%s  %s%-8s%s %.1f / %.1f GB (%d%%) auf %-37s %s%s║%s\n",
               BOLD, CYAN, RESET, BOLD, d->name, RESET, d->used_gb, d->total_gb, d->percent, d->mount, BOLD, CYAN, RESET);
    }
    printf("%s%s╠════════════════════════════════════════════════════════════════════════════╣%s\n", BOLD, CYAN, RESET);

    // Temperaturen & Lüfter
    char fan_buf[32];
    if (t->fan_percent > 0) snprintf(fan_buf, sizeof(fan_buf), "%d RPM (%d%%)", t->fan_rpm, t->fan_percent);
    else snprintf(fan_buf, sizeof(fan_buf), "0 RPM (Silent)");
    printf("%s%s║%s  %sTemp:%s    CPU: %.1f °C  |  Akku: %.1f °C  |  NVMe: %.1f °C                     %s%s║%s\n",
           BOLD, CYAN, RESET, BOLD, RESET, t->cpu_temp_c, t->bat_temp_c, t->nvme_temp_c, BOLD, CYAN, RESET);
    printf("%s%s║%s  %sLüfter:%s  %-65s %s%s║%s\n",
           BOLD, CYAN, RESET, BOLD, RESET, fan_buf, BOLD, CYAN, RESET);

    printf("%s%s╚════════════════════════════════════════════════════════════════════════════╝%s\n", BOLD, CYAN, RESET);
    printf("  %sAbfragezeit:%s %.2f ms   |   %sWidget-Option:%s --json\n\n",
           CYAN, RESET, t->query_ms, CYAN, RESET);
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--set-profile") == 0) {
            if (i + 1 < argc) {
                const char *prof = argv[i + 1];
                if (strcmp(prof, "power-saver") != 0 && strcmp(prof, "balanced") != 0 && strcmp(prof, "performance") != 0) {
                    fprintf(stderr, "Error: Unknown profile '%s'. Choose: power-saver, balanced, performance\n", prof);
                    return 1;
                }
                char cmd[256];
                snprintf(cmd, sizeof(cmd),
                    "gdbus call --system --dest net.hadess.PowerProfiles --object-path /net/hadess/PowerProfiles "
                    "--method org.freedesktop.DBus.Properties.Set net.hadess.PowerProfiles ActiveProfile \"<'%s'>\" > /dev/null 2>&1",
                    prof);
                int ret = system(cmd);
                if (ret == 0) printf("Power profile switched to '%s'.\n", prof);
                else { fprintf(stderr, "Failed to switch power profile.\n"); return 1; }
                return 0;
            } else {
                fprintf(stderr, "Missing profile name\n");
                return 1;
            }
        }
    }

    Telemetry t;
    collect_telemetry(&t);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--json") == 0 || strcmp(argv[i], "-j") == 0) {
            print_json(&t);
            return 0;
        }
    }

    print_dashboard(&t);
    return 0;
}
