/*
 *      batt_sys.c
 *
 *      Enhanced Linux power_supply battery reader for wf-panel-pi (libbatt.so)
 *      Reads all available /sys/class/power_supply/BAT* attributes dynamically.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "batt_sys.h"
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>

battery *battery_new(void)
{
    static int battery_num = 1;
    battery *b = g_new0(battery, 1);
    b->type_battery = TRUE;
    b->capacity = -1;
    b->energy_full = -1;
    b->charge_full = -1;
    b->voltage_now = -1;
    b->voltage_min_design = -1;
    b->voltage_max_design = -1;
    b->energy_full_design = -1;
    b->charge_full_design = -1;
    b->energy_now = -1;
    b->charge_now = -1;
    b->current_now = -1;
    b->power_now = -1;
    b->time_to_empty = -1;
    b->time_to_full = -1;
    b->cycle_count = -1;
    b->temp_deci_c = G_MININT;
    b->charge_threshold = -1;
    b->health_pct = -1;
    b->state = NULL;
    b->capacity_level = NULL;
    b->health = NULL;
    b->technology = NULL;
    b->manufacturer = NULL;
    b->model_name = NULL;
    b->serial_number = NULL;
    b->scope = NULL;
    b->battery_num = battery_num;
    b->seconds = -1;
    b->percentage = -1;
    battery_num++;
    return b;
}

static gchar *parse_info_file(battery *b, const char *sys_file)
{
    char *buf = NULL;
    GString *filename = g_string_new(ACPI_PATH_SYS_POWER_SUPPLY);

    g_string_append_printf(filename, "/%s/%s", b->path, sys_file);

    if (g_file_get_contents(filename->str, &buf, NULL, NULL) == TRUE) {
        g_strstrip(buf);
        if (buf[0] == '\0') {
            g_free(buf);
            buf = NULL;
        }
    }

    g_string_free(filename, TRUE);
    return buf;
}

/* Reads a micro-unit sysfs file (µV, µA, µW, µAh, µWh) and converts to milli-units.
 * Returns -1 if the file does not exist or cannot be parsed. */
static gint get_milli_from_infofile(battery *b, const gchar *sys_file, gboolean take_abs)
{
    gchar *file_content = parse_info_file(b, sys_file);
    gint value = -1;

    if (file_content != NULL) {
        gchar *endptr = NULL;
        gint64 raw = g_ascii_strtoll(file_content, &endptr, 10);
        if (endptr != file_content) {
            if (take_abs && raw < 0)
                raw = -raw;
            if (raw >= 0)
                value = (gint)((raw + 500) / 1000);
        }
        g_free(file_content);
    }

    return value;
}

/* Reads an unscaled integer sysfs file (capacity %, seconds, cycle_count, temp 0.1°C).
 * Returns default_val if the file does not exist or cannot be parsed. */
static gint get_raw_int_from_infofile(battery *b, const gchar *sys_file, gint default_val)
{
    gchar *file_content = parse_info_file(b, sys_file);
    gint value = default_val;

    if (file_content != NULL) {
        gchar *endptr = NULL;
        gint64 raw = g_ascii_strtoll(file_content, &endptr, 10);
        if (endptr != file_content) {
            value = (gint)raw;
        }
        g_free(file_content);
    }

    return value;
}

static gchar *get_gchar_from_infofile(battery *b, const gchar *sys_file)
{
    gchar *val = parse_info_file(b, sys_file);
    if (val && (strcasecmp(val, "N/A") == 0 || strcasecmp(val, "Unknown") == 0)) {
        g_free(val);
        return NULL;
    }
    return val;
}

static gboolean battery_inserted(gchar *path)
{
    if (path == NULL)
        return FALSE;

    GString *dirname = g_string_new(ACPI_PATH_SYS_POWER_SUPPLY);
    GDir *dir;

    g_string_append_printf(dirname, "/%s/", path);
    dir = g_dir_open(dirname->str, 0, NULL);
    if (dir)
        g_dir_close(dir);
    g_string_free(dirname, TRUE);

    return dir ? TRUE : FALSE;
}

battery *battery_update(battery *b)
{
    gchar *gctmp;

    if (b == NULL)
        return NULL;

    if (!battery_inserted(b->path))
        return NULL;

    /* Read capacity & charge/energy metrics from sysfs */
    b->capacity   = get_raw_int_from_infofile(b, "capacity", -1);
    b->charge_now = get_milli_from_infofile(b, "charge_now", FALSE);
    b->energy_now = get_milli_from_infofile(b, "energy_now", FALSE);

    b->current_now = get_milli_from_infofile(b, "current_now", TRUE);
    if (b->current_now < 0)
        b->current_now = get_milli_from_infofile(b, "current_avg", TRUE);

    b->power_now = get_milli_from_infofile(b, "power_now", TRUE);
    if (b->power_now < 0)
        b->power_now = get_milli_from_infofile(b, "power_avg", TRUE);

    b->charge_full = get_milli_from_infofile(b, "charge_full", FALSE);
    b->energy_full = get_milli_from_infofile(b, "energy_full", FALSE);

    b->charge_full_design = get_milli_from_infofile(b, "charge_full_design", FALSE);
    b->energy_full_design = get_milli_from_infofile(b, "energy_full_design", FALSE);

    b->voltage_now = get_milli_from_infofile(b, "voltage_now", FALSE);
    if (b->voltage_now < 0)
        b->voltage_now = get_milli_from_infofile(b, "voltage_avg", FALSE);
    b->voltage_min_design = get_milli_from_infofile(b, "voltage_min_design", FALSE);
    b->voltage_max_design = get_milli_from_infofile(b, "voltage_max_design", FALSE);

    /* Calculate power_now (P = U * I) if driver only exports voltage_now + current_now */
    if (b->power_now < 0 && b->voltage_now > 0 && b->current_now >= 0) {
        b->power_now = (gint)(((gint64)b->voltage_now * b->current_now + 500) / 1000);
    }

    /* Read time_to_empty and time_to_full (prefer smoothed _avg when available) */
    b->time_to_empty = get_raw_int_from_infofile(b, "time_to_empty_avg", -1);
    if (b->time_to_empty <= 0)
        b->time_to_empty = get_raw_int_from_infofile(b, "time_to_empty_now", -1);

    b->time_to_full = get_raw_int_from_infofile(b, "time_to_full_now", -1);
    if (b->time_to_full <= 0)
        b->time_to_full = get_raw_int_from_infofile(b, "time_to_full_avg", -1);

    /* Read cycle count, temperature (0.1 °C), and charge threshold */
    b->cycle_count = get_raw_int_from_infofile(b, "cycle_count", -1);
    b->temp_deci_c = get_raw_int_from_infofile(b, "temp", G_MININT);
    if (b->temp_deci_c < -400 || b->temp_deci_c > 1200)
        b->temp_deci_c = G_MININT;

    b->charge_threshold = get_raw_int_from_infofile(b, "charge_control_end_threshold", -1);

    /* Verify battery type */
    gctmp = parse_info_file(b, "type");
    b->type_battery = gctmp ? (strcasecmp(gctmp, "battery") == 0) : TRUE;
    g_free(gctmp);

    /* Read status & descriptive strings */
    g_free(b->state);
    b->state = parse_info_file(b, "status");
    if (!b->state)
        b->state = parse_info_file(b, "state");
    if (!b->state) {
        if (b->charge_now != -1 || b->energy_now != -1
                || b->charge_full != -1 || b->energy_full != -1)
            b->state = g_strdup("available");
        else
            b->state = g_strdup("unavailable");
    }

    g_free(b->capacity_level);
    b->capacity_level = get_gchar_from_infofile(b, "capacity_level");

    g_free(b->health);
    b->health = get_gchar_from_infofile(b, "health");

    g_free(b->technology);
    b->technology = get_gchar_from_infofile(b, "technology");

    g_free(b->manufacturer);
    b->manufacturer = get_gchar_from_infofile(b, "manufacturer");

    g_free(b->model_name);
    b->model_name = get_gchar_from_infofile(b, "model_name");

    g_free(b->serial_number);
    b->serial_number = get_gchar_from_infofile(b, "serial_number");

    g_free(b->scope);
    b->scope = parse_info_file(b, "scope");

    /* Calculate percentage */
    if (b->capacity >= 0 && b->capacity <= 100) {
        b->percentage = b->capacity;
    } else if (b->charge_now >= 0 && b->charge_full > 0) {
        int promille = (int)(((gint64)b->charge_now * 1000) / b->charge_full);
        b->percentage = (promille + 5) / 10;
    } else if (b->energy_now >= 0 && b->energy_full > 0) {
        int promille = (int)(((gint64)b->energy_now * 1000) / b->energy_full);
        b->percentage = (promille + 5) / 10;
    } else {
        b->percentage = 0;
    }
    if (b->percentage > 100)
        b->percentage = 100;

    /* Calculate battery health percentage (State of Health: full vs design) */
    if (b->energy_full > 0 && b->energy_full_design > 0) {
        b->health_pct = (int)(((gint64)b->energy_full * 100 + b->energy_full_design / 2) / b->energy_full_design);
    } else if (b->charge_full > 0 && b->charge_full_design > 0) {
        b->health_pct = (int)(((gint64)b->charge_full * 100 + b->charge_full_design / 2) / b->charge_full_design);
    } else {
        b->health_pct = -1;
    }
    if (b->health_pct > 100)
        b->health_pct = 100;

    /* Determine remaining seconds (prefer kernel-reported time_to_empty / time_to_full) */
    b->seconds = -1;
    if (!strcasecmp(b->state, "charging")) {
        if (b->time_to_full > 0 && b->time_to_full < 86400 * 3) {
            b->seconds = b->time_to_full;
        } else if (b->current_now > MIN_PRESENT_RATE && b->charge_full > b->charge_now && b->charge_now >= 0) {
            b->seconds = (int)((3600LL * (b->charge_full - b->charge_now)) / b->current_now);
        } else if (b->power_now > 0 && b->energy_full > b->energy_now && b->energy_now >= 0) {
            b->seconds = (int)((3600LL * (b->energy_full - b->energy_now)) / b->power_now);
        }
    } else if (!strcasecmp(b->state, "discharging")) {
        if (b->time_to_empty > 0 && b->time_to_empty < 86400 * 7) {
            b->seconds = b->time_to_empty;
        } else if (b->current_now > MIN_PRESENT_RATE && b->charge_now > 0) {
            b->seconds = (int)((3600LL * b->charge_now) / b->current_now);
        } else if (b->power_now > 0 && b->energy_now > 0) {
            b->seconds = (int)((3600LL * b->energy_now) / b->power_now);
        }
    }

    return b;
}

battery *battery_get(int battery_number)
{
    GError *error = NULL;
    const gchar *entry;
    gchar *batt_name = NULL;
    gchar *batt_path = NULL;
    GDir *dir;
    battery *b = NULL;

    /* Try the expected path in sysfs first */
    batt_name = g_strdup_printf(ACPI_BATTERY_DEVICE_NAME "%d", battery_number);
    batt_path = g_strdup_printf(ACPI_PATH_SYS_POWER_SUPPLY "/%s", batt_name);
    if (g_file_test(batt_path, G_FILE_TEST_IS_DIR) == TRUE) {
        b = battery_new();
        b->path = g_strdup(batt_name);
        battery_update(b);

        if (!b->type_battery) {
            g_message("batt: not a battery: %s", batt_path);
            battery_free(b);
            b = NULL;
        }
    }

    g_free(batt_name);
    g_free(batt_path);

    if (b != NULL)
        return b;

    /* Walk the dir and return any system battery */
    dir = g_dir_open(ACPI_PATH_SYS_POWER_SUPPLY, 0, &error);
    if (dir == NULL) {
        g_message("batt: no ACPI/sysfs support in kernel: %s", error->message);
        g_error_free(error);
        return NULL;
    }

    while ((entry = g_dir_read_name(dir)) != NULL) {
        b = battery_new();
        b->path = g_strdup(entry);
        battery_update(b);

        if (b->type_battery == TRUE && g_strcmp0(b->scope, "Device")) {
            break;
        }
        battery_free(b);
        b = NULL;
    }
    if (b != NULL)
        g_message("batt: battery entry " ACPI_BATTERY_DEVICE_NAME "%d not found, using %s",
            battery_number, b->path);
    else
        g_message("batt: battery %d not found", battery_number);

    g_dir_close(dir);
    return b;
}

void battery_free(battery *bat)
{
    if (bat) {
        g_free(bat->path);
        g_free(bat->state);
        g_free(bat->capacity_level);
        g_free(bat->health);
        g_free(bat->technology);
        g_free(bat->manufacturer);
        g_free(bat->model_name);
        g_free(bat->serial_number);
        g_free(bat->scope);
        g_free(bat);
    }
}

gboolean battery_is_charging(battery *b)
{
    if (!b->state)
        return TRUE;
    return (strcasecmp(b->state, "Unknown") == 0
            || strcasecmp(b->state, "Full") == 0
            || strcasecmp(b->state, "Not charging") == 0
            || strcasecmp(b->state, "Charging") == 0
            || b->current_now == 0);
}

gint battery_get_remaining(battery *b)
{
    return b->seconds;
}
