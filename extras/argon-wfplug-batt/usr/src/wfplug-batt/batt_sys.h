/*
 *      batt_sys.h
 *
 *      Enhanced Linux power_supply battery reader for wf-panel-pi (libbatt.so)
 *      Supports all standard /sys/class/power_supply/BAT* attributes dynamically.
 */

#ifndef BATT_SYS_H
#define BATT_SYS_H

#define BUF_SIZE 1024
#define ACPI_PATH_SYS_POWER_SUPPLY  "/sys/class/power_supply"
#define ACPI_BATTERY_DEVICE_NAME    "BAT"
#define MIN_CAPACITY     0.01
#define MIN_PRESENT_RATE 0.01
#define BATTERY_DESC     "Battery"

#include <glib.h>

typedef struct battery {
    int battery_num;
    /* path to battery dir */
    gchar *path;

    /* sysfs file contents (-1 = unavailable, temp_deci_c uses G_MININT) */
    int capacity;            /* % (0..100) from "capacity" */
    int charge_now;          /* mAh (from µAh / 1000) */
    int energy_now;          /* mWh (from µWh / 1000) */
    int current_now;         /* mA  (abs value, from µA / 1000) */
    int power_now;           /* mW  (abs value, from µW / 1000) */
    int voltage_now;         /* mV  (from µV / 1000) */
    int voltage_min_design;  /* mV  (from µV / 1000) */
    int voltage_max_design;  /* mV  (from µV / 1000) */
    int charge_full_design;  /* mAh */
    int energy_full_design;  /* mWh */
    int charge_full;         /* mAh */
    int energy_full;         /* mWh */
    int time_to_empty;       /* seconds */
    int time_to_full;        /* seconds */
    int cycle_count;         /* charge cycles */
    int temp_deci_c;         /* 0.1 °C (G_MININT if unavailable) */
    int charge_threshold;    /* % from charge_control_end_threshold (-1 if unavailable) */

    /* calculated / extra info */
    int seconds;             /* active remaining seconds (discharge or charge) */
    int percentage;          /* 0..100 */
    int health_pct;          /* SoH % (full / full_design * 100), -1 if unavailable */

    /* string attributes (NULL if unavailable) */
    char *state;
    char *capacity_level;
    char *health;
    char *technology;
    char *manufacturer;
    char *model_name;
    char *serial_number;
    char *scope;
    int type_battery;
} battery;

battery *battery_new(void);
battery *battery_get(int);
battery *battery_update(battery *b);
gboolean battery_is_charging(battery *b);
gint battery_get_remaining(battery *b);
void battery_free(battery *bat);

#endif
