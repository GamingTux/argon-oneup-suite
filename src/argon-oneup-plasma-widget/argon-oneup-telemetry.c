#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>

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

typedef struct {
    int cores_mhz[4];
    int max_mhz;
    float temp_c;
    char governor[32];
    int fan_rpm;
    int fan_pwm;
    int fan_percent;
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
    float nvme_temp_c;
    long mem_total_mb;
    long mem_used_mb;
    int mem_percent;
    char profile[32];
    double query_ms;
} Telemetry;

static void collect_telemetry(Telemetry *t) {
    struct timeval tv0, tv1;
    gettimeofday(&tv0, NULL);

    memset(t, 0, sizeof(*t));

    // 1. CPU Cores & Max Freq
    t->max_mhz = (int)(read_int("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", 2400000) / 1000);
    char path[128];
    for (int i = 0; i < 4; i++) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", i);
        long val = read_int(path, 0);
        t->cores_mhz[i] = (val > 0) ? (int)(val / 1000) : t->max_mhz;
    }
    t->temp_c = read_int("/sys/class/thermal/thermal_zone0/temp", 0) / 1000.0f;
    read_str("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", t->governor, sizeof(t->governor), "ondemand");

    // 2. Fan & Cooling (scan hwmon)
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

    // 3. Battery (CW2217 via oneUpPower)
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

    // 4. Memory (/proc/meminfo)
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

    // 5. Active Profile
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
    printf("    \"max_mhz\": %d,\n", t->max_mhz);
    printf("    \"temp_c\": %.1f,\n", t->temp_c);
    printf("    \"governor\": \"%s\"\n", t->governor);
    printf("  },\n");
    printf("  \"fan\": {\n");
    printf("    \"rpm\": %d,\n", t->fan_rpm);
    printf("    \"pwm\": %d,\n", t->fan_pwm);
    printf("    \"percent\": %d\n", t->fan_percent);
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
    printf("  \"storage\": {\n");
    printf("    \"nvme_temp_c\": %.1f\n", t->nvme_temp_c);
    printf("  },\n");
    printf("  \"memory\": {\n");
    printf("    \"used_mb\": %ld,\n", t->mem_used_mb);
    printf("    \"total_mb\": %ld,\n", t->mem_total_mb);
    printf("    \"percent\": %d\n", t->mem_percent);
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

    const char *t_color = (t->temp_c < 50.0f) ? GREEN : ((t->temp_c < 70.0f) ? YELLOW : RED);
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

    char cores_buf[64];
    snprintf(cores_buf, sizeof(cores_buf), "%d MHz, %d MHz, %d MHz, %d MHz",
             t->cores_mhz[0], t->cores_mhz[1], t->cores_mhz[2], t->cores_mhz[3]);
    printf("%s%s║%s  %sCPU:%s     %-44s (Max: %d MHz) %s%s║%s\n",
           BOLD, CYAN, RESET, BOLD, RESET, cores_buf, t->max_mhz, BOLD, CYAN, RESET);
    printf("%s%s║%s  %sTemp:%s    %s%.1f °C%s  |  Governor: %-15s NVMe Temp: %.1f °C   %s%s║%s\n",
           BOLD, CYAN, RESET, BOLD, RESET, t_color, t->temp_c, RESET, t->governor, t->nvme_temp_c, BOLD, CYAN, RESET);
    printf("%s%s╠════════════════════════════════════════════════════════════════════════════╣%s\n", BOLD, CYAN, RESET);

    char fan_buf[64];
    if (t->fan_percent > 0) snprintf(fan_buf, sizeof(fan_buf), "%d RPM (%d%%)", t->fan_rpm, t->fan_percent);
    else snprintf(fan_buf, sizeof(fan_buf), "0 RPM (Silent / 0%%)");
    printf("%s%s║%s  %sLüfter:%s  %-30s RAM: %ld MB / %ld MB (%d%%) %s%s║%s\n",
           BOLD, CYAN, RESET, BOLD, RESET, fan_buf, t->mem_used_mb, t->mem_total_mb, t->mem_percent, BOLD, CYAN, RESET);
    printf("%s%s╠════════════════════════════════════════════════════════════════════════════╣%s\n", BOLD, CYAN, RESET);

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
