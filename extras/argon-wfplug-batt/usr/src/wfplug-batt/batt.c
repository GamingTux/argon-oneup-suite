/*============================================================================
Copyright (c) 2018-2025 Raspberry Pi
Enhanced Battery Telemetry Tooltip for wf-panel-pi (Argon ONE UP Suite)
============================================================================*/

#include <locale.h>
#include <string.h>
#include <glib/gi18n.h>

#include "plugin.h"

#include "batt_sys.h"
#include "batt.h"

/*----------------------------------------------------------------------------*/
/* Typedefs and macros                                                        */
/*----------------------------------------------------------------------------*/

#define SIM_INTERVAL 500
#define INTERVAL 5000

/* Battery states */
typedef enum
{
    STAT_UNKNOWN = -1,
    STAT_DISCHARGING = 0,
    STAT_CHARGING = 1,
    STAT_EXT_POWER = 2
} status_t;

/*----------------------------------------------------------------------------*/
/* Global data                                                                */
/*----------------------------------------------------------------------------*/

conf_table_t conf_table[2] = {
    {CONF_TYPE_INT,  "batt_num", N_("Battery number to monitor"),   NULL,   "0" },
    {CONF_TYPE_NONE, NULL,       NULL,                              NULL,   NULL}
};

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/

static int init_measurement (PtBattPlugin *pt);
static int charge_level (PtBattPlugin *pt, status_t *status, int *tim);
static void draw_icon (PtBattPlugin *pt, int lev, float r, float g, float b, int powered);
static void update_icon (PtBattPlugin *pt);
static gboolean timer_event (PtBattPlugin *pt);

/*----------------------------------------------------------------------------*/
/* Helper functions                                                           */
/*----------------------------------------------------------------------------*/

static gboolean is_german_locale (void)
{
    const char *loc = setlocale (LC_MESSAGES, NULL);
    if (!loc || !loc[0] || strcmp (loc, "C") == 0)
        loc = g_getenv ("LANG");
    return (loc && strncmp (loc, "de", 2) == 0);
}

static const char *translate_health (const char *health, gboolean is_de)
{
    if (!health) return NULL;
    if (!is_de) return health;

    if (strcasecmp (health, "Good") == 0) return "Gut";
    if (strcasecmp (health, "Overheat") == 0) return "Überhitzt";
    if (strcasecmp (health, "Dead") == 0) return "Schwach / Defekt";
    if (strcasecmp (health, "Over voltage") == 0) return "Überspannung";
    if (strcasecmp (health, "Unspec failure") == 0) return "Fehler";
    if (strcasecmp (health, "Cold") == 0) return "Unterkühlt";
    if (strcasecmp (health, "Over current") == 0) return "Überstrom";
    return health;
}

static void build_battery_tooltip (battery *b, status_t status, int capacity, GString *tip)
{
    gboolean is_de = is_german_locale ();

    /* 1. Status & Capacity (Header) */
    if (status == STAT_CHARGING) {
        g_string_append_printf (tip, is_de ? "<b>Akku: Lädt (%d %%)</b>" : "<b>Battery: Charging (%d%%)</b>", capacity);
    } else if (status == STAT_EXT_POWER) {
        if (b && b->state && strcasecmp (b->state, "Full") == 0)
            g_string_append_printf (tip, is_de ? "<b>Akku: Voll geladen (%d %%)</b>" : "<b>Battery: Fully charged (%d%%)</b>", capacity);
        else
            g_string_append_printf (tip, is_de ? "<b>Akku: Netzbetrieb (%d %%)</b>" : "<b>Battery: On external power (%d%%)</b>", capacity);
    } else {
        g_string_append_printf (tip, is_de ? "<b>Akku: Entlädt (%d %%)</b>" : "<b>Battery: Discharging (%d%%)</b>", capacity);
    }

    if (!b) return;

    /* 2. Remaining Time / Time to Full */
    if (b->seconds > 0 && (status == STAT_DISCHARGING || status == STAT_CHARGING)) {
        int total_mins = (b->seconds + 30) / 60;
        int hours = total_mins / 60;
        int mins = total_mins % 60;

        if (status == STAT_DISCHARGING) {
            if (hours > 0)
                g_string_append_printf (tip, is_de ? "\nRestlaufzeit: %d Std. %02d Min." : "\nTime remaining: %dh %02dm", hours, mins);
            else if (mins > 0)
                g_string_append_printf (tip, is_de ? "\nRestlaufzeit: %d Min." : "\nTime remaining: %d min", mins);
        } else if (status == STAT_CHARGING) {
            if (hours > 0)
                g_string_append_printf (tip, is_de ? "\nBis voll geladen: %d Std. %02d Min." : "\nTime to full: %dh %02dm", hours, mins);
            else if (mins > 0)
                g_string_append_printf (tip, is_de ? "\nBis voll geladen: %d Min." : "\nTime to full: %d min", mins);
        }
    }

    /* 3. Power & Current */
    if (b->power_now > 0 && b->current_now > 0) {
        g_string_append_printf (tip, is_de ? "\nLeistung: %.2f W (%.2f A)" : "\nPower: %.2f W (%.2f A)",
            b->power_now / 1000.0, b->current_now / 1000.0);
    } else if (b->power_now > 0) {
        g_string_append_printf (tip, is_de ? "\nLeistung: %.2f W" : "\nPower: %.2f W",
            b->power_now / 1000.0);
    } else if (b->current_now > 0) {
        g_string_append_printf (tip, is_de ? "\nStromstärke: %.2f A" : "\nCurrent: %.2f A",
            b->current_now / 1000.0);
    }

    /* 4. Voltage */
    if (b->voltage_now > 0) {
        if (b->voltage_min_design > 0 && b->voltage_max_design > 0) {
            g_string_append_printf (tip, is_de ? "\nSpannung: %.2f V (%.1f–%.1f V)" : "\nVoltage: %.2f V (%.1f–%.1f V)",
                b->voltage_now / 1000.0, b->voltage_min_design / 1000.0, b->voltage_max_design / 1000.0);
        } else if (b->voltage_min_design > 0) {
            g_string_append_printf (tip, is_de ? "\nSpannung: %.2f V (Min: %.1f V)" : "\nVoltage: %.2f V (Min: %.1f V)",
                b->voltage_now / 1000.0, b->voltage_min_design / 1000.0);
        } else {
            g_string_append_printf (tip, is_de ? "\nSpannung: %.2f V" : "\nVoltage: %.2f V",
                b->voltage_now / 1000.0);
        }
    }

    /* 5. Energy & Capacity */
    if (b->energy_now >= 0 && b->energy_full > 0 && b->charge_now >= 0 && b->charge_full > 0) {
        g_string_append_printf (tip, is_de ? "\nEnergie: %.1f / %.1f Wh (%d / %d mAh)" : "\nEnergy: %.1f / %.1f Wh (%d / %d mAh)",
            b->energy_now / 1000.0, b->energy_full / 1000.0, b->charge_now, b->charge_full);
    } else if (b->energy_now >= 0 && b->energy_full > 0) {
        g_string_append_printf (tip, is_de ? "\nEnergie: %.1f / %.1f Wh" : "\nEnergy: %.1f / %.1f Wh",
            b->energy_now / 1000.0, b->energy_full / 1000.0);
    } else if (b->charge_now >= 0 && b->charge_full > 0) {
        g_string_append_printf (tip, is_de ? "\nKapazität: %d / %d mAh" : "\nCapacity: %d / %d mAh",
            b->charge_now, b->charge_full);
    }

    /* 6. Health (State of Health) */
    if (b->health != NULL || b->health_pct > 0) {
        const char *h_str = translate_health (b->health, is_de);
        gchar *h_esc = h_str ? g_markup_escape_text (h_str, -1) : NULL;

        if (h_esc && b->health_pct > 0) {
            g_string_append_printf (tip, is_de ? "\nZustand: %s (%d %%)" : "\nHealth: %s (%d%%)", h_esc, b->health_pct);
        } else if (h_esc) {
            g_string_append_printf (tip, is_de ? "\nZustand: %s" : "\nHealth: %s", h_esc);
        } else if (b->health_pct > 0) {
            g_string_append_printf (tip, is_de ? "\nZustand: %d %%" : "\nHealth: %d%%", b->health_pct);
        }
        g_free (h_esc);
    }

    /* 7. Cycle Count */
    if (b->cycle_count >= 0) {
        g_string_append_printf (tip, is_de ? "\nLadezyklen: %d" : "\nCycle count: %d", b->cycle_count);
    }

    /* 8. Charge Threshold (if active < 100%) */
    if (b->charge_threshold > 0 && b->charge_threshold < 100) {
        g_string_append_printf (tip, is_de ? "\nLadelimit: %d %%" : "\nCharge limit: %d%%", b->charge_threshold);
    }

    /* 9. Battery Temperature */
    if (b->temp_deci_c != G_MININT) {
        g_string_append_printf (tip, is_de ? "\nTemperatur: %.1f °C" : "\nTemperature: %.1f °C",
            b->temp_deci_c / 10.0);
    }

    /* 10. Hardware Model / Manufacturer / Technology */
    if (b->manufacturer || b->model_name || b->technology) {
        gchar *mfg_esc  = b->manufacturer ? g_markup_escape_text (b->manufacturer, -1) : NULL;
        gchar *mdl_esc  = b->model_name   ? g_markup_escape_text (b->model_name, -1)   : NULL;
        gchar *tech_esc = b->technology   ? g_markup_escape_text (b->technology, -1)   : NULL;

        g_string_append (tip, is_de ? "\nModell: " : "\nModel: ");
        if (mfg_esc && mdl_esc) {
            g_string_append_printf (tip, "%s %s", mfg_esc, mdl_esc);
        } else if (mfg_esc) {
            g_string_append (tip, mfg_esc);
        } else if (mdl_esc) {
            g_string_append (tip, mdl_esc);
        }

        if (tech_esc) {
            if (mfg_esc || mdl_esc)
                g_string_append_printf (tip, " (%s)", tech_esc);
            else
                g_string_append (tip, tech_esc);
        }

        g_free (mfg_esc);
        g_free (mdl_esc);
        g_free (tech_esc);
    }

    if (b->serial_number) {
        gchar *ser_esc = g_markup_escape_text (b->serial_number, -1);
        g_string_append_printf (tip, is_de ? "\nSeriennr.: %s" : "\nSerial: %s", ser_esc);
        g_free (ser_esc);
    }
}

/*----------------------------------------------------------------------------*/
/* Function definitions                                                       */
/*----------------------------------------------------------------------------*/

/* Initialise measurements and check for a battery */

static int init_measurement (PtBattPlugin *pt)
{
    if (pt->simulate) return 1;

    pt->batt = battery_get (pt->batt_num);
    if (pt->batt) return 1;

    return 0;
}

/* Read current capacity, status and time remaining from battery */

static int charge_level (PtBattPlugin *pt, status_t *status, int *tim)
{
    if (pt->simulate)
    {
        static int level = 0;
        *tim = 30;
        if (level < 100) level += 5;
        else level = -100;
        if (level < 0)
        {
            *status = STAT_DISCHARGING;
            return (level * -1);
        }
        else if (level == 100) *status = STAT_EXT_POWER;
        else *status = STAT_CHARGING;
        return level;
    }
    *status = STAT_UNKNOWN;
    *tim = 0;
    battery *b = pt->batt;
    int mins;
    if (b)
    {
        battery_update (b);
        if (battery_is_charging (b))
        {
            if (b->state && (strcasecmp (b->state, "full") == 0 || strcasecmp (b->state, "not charging") == 0))
                *status = STAT_EXT_POWER;
            else if (b->state && strcasecmp (b->state, "charging") == 0)
                *status = STAT_CHARGING;
            else
                *status = STAT_EXT_POWER;
        }
        else *status = STAT_DISCHARGING;
        mins = b->seconds;
        mins /= 60;
        *tim = mins;
        return b->percentage;
    }
    else return -1;
}

/* Draw the icon in relevant colour and fill level */

static void draw_icon (PtBattPlugin *pt, int lev, float r, float g, float b, int powered)
{
    int h, w, f, ic; 

    // calculate dimensions based on icon size
    ic = wrap_icon_size (pt);
    w = ic < 36 ? 36 : ic;
    h = ((w * 10) / 36) * 2; // force it to be even
    if (h < 18) h = 18;
    if (h >= ic) h = ic - 2;

    // create and clear the drawing surface
    cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = cairo_create (surface);
    cairo_set_source_rgba (cr, 0, 0, 0, 0);
    cairo_rectangle (cr, 0, 0, w, h);
    cairo_fill (cr);

    // draw base icon on surface
    cairo_set_source_rgb (cr, r, g, b);
    cairo_rectangle (cr, 4, 1, w - 10, 1);
    cairo_rectangle (cr, 3, 2, w - 8, 1);
    cairo_rectangle (cr, 3, h - 3, w - 8, 1);
    cairo_rectangle (cr, 4, h - 2, w - 10, 1);
    cairo_rectangle (cr, 2, 3, 2, h - 6);
    cairo_rectangle (cr, w - 6, 3, 2, h - 6);
    cairo_rectangle (cr, w - 4, (h >> 1) - 3, 2, 6);
    cairo_fill (cr);

    cairo_set_source_rgba (cr, r, g, b, 0.5);
    cairo_rectangle (cr, 3, 1, 1, 1);
    cairo_rectangle (cr, 2, 2, 1, 1);
    cairo_rectangle (cr, 2, h - 3, 1, 1);
    cairo_rectangle (cr, 3, h - 2, 1, 1);
    cairo_rectangle (cr, w - 6, 1, 1, 1);
    cairo_rectangle (cr, w - 5, 2, 1, 1);
    cairo_rectangle (cr, w - 5, h - 3, 1, 1);
    cairo_rectangle (cr, w - 6, h - 2, 1, 1);
    cairo_fill (cr);

    // fill the battery
    if (lev < 0) f = 0;
    else if (lev > 97) f = w - 12;
    else
    {
        f = (w - 12) * lev;
        f /= 97;
        if (f > w - 12) f = w - 12;
    }
    cairo_set_source_rgb (cr, r, g, b);
    cairo_rectangle (cr, 5, 4, f, h - 8);
    cairo_fill (cr);

    // show icons
    if (powered == 1 && pt->flash)
    {
        gdk_cairo_set_source_pixbuf (cr, pt->flash, (w >> 1) - 15, (h >> 1) - 16);
        cairo_paint (cr);
    }
    if (powered == 2 && pt->plug)
    {
        gdk_cairo_set_source_pixbuf (cr, pt->plug, (w >> 1) - 16, (h >> 1) - 16);
        cairo_paint (cr);
    }

    // create a pixbuf from the cairo surface
    GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0, w, h);

    // copy the pixbuf to the icon resource
    g_object_ref_sink (pt->tray_icon);
    gtk_image_set_from_pixbuf (GTK_IMAGE (pt->tray_icon), pixbuf);

    g_object_unref (pixbuf);
    cairo_destroy (cr);
    cairo_surface_destroy (surface);
}

/* Read the current charge state and update the icon accordingly */

static void update_icon (PtBattPlugin *pt)
{
    int capacity, time;
    status_t status;

    if (!pt->timer)
    {
        gtk_widget_hide (pt->plugin);
        return;
    }

    // read the charge status
    capacity = charge_level (pt, &status, &time);
    if (status == STAT_UNKNOWN) return;

    // draw the battery symbol
    if (status == STAT_CHARGING)
    {
        draw_icon (pt, capacity, 0.95, 0.64, 0, 1);
    }
    else if (status == STAT_EXT_POWER)
    {
        draw_icon (pt, capacity, 0, 0.85, 0, 2);
    }
    else
    {
        if (capacity <= 20) draw_icon (pt, capacity, 1, 0, 0, 0);
        else draw_icon (pt, capacity, 0, 0.85, 0, 0);
    }

    // build and set the rich tooltip
    GString *tip = g_string_new (NULL);
    build_battery_tooltip (pt->batt, status, capacity, tip);
    gtk_widget_set_tooltip_markup (pt->tray_icon, tip->str);
    g_string_free (tip, TRUE);

    gtk_widget_show_all (pt->plugin);
}

static gboolean timer_event (PtBattPlugin *pt)
{
    update_icon (pt);
    return TRUE;
}

/*----------------------------------------------------------------------------*/
/* wf-panel plugin functions                                                  */
/*----------------------------------------------------------------------------*/

/* Handler for system config changed message from panel */
void batt_update_display (PtBattPlugin *pt)
{
    update_icon (pt);
}

void batt_set_values (PtBattPlugin *pt)
{
    conf_table[0].value = (void *) &pt->batt_num;
}

/* Handler for battery number update from variable watcher */
void batt_set_num (PtBattPlugin *pt)
{
    if (pt->timer) g_source_remove (pt->timer);
    if (init_measurement (pt))
    {
        pt->timer = g_timeout_add (pt->simulate ? SIM_INTERVAL : INTERVAL, (GSourceFunc) timer_event, (gpointer) pt);
        update_icon (pt);
    }
    else pt->timer = 0;
}

void batt_init (PtBattPlugin *pt)
{
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

    /* Allocate icon as a child of top level */
    pt->tray_icon = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (pt->plugin), pt->tray_icon);

    /* Set up button */
    wrap_add_longpress (pt->gesture, pt->plugin, NULL, NULL);

    /* Load the symbols */
    pt->plug = gdk_pixbuf_new_from_file (PACKAGE_DATA_DIR "/images/plug.png", NULL);
    pt->flash = gdk_pixbuf_new_from_file (PACKAGE_DATA_DIR "/images/flash.png", NULL);

    if (getenv ("PLUGIN_SIMBAT")) pt->simulate = TRUE;
    else pt->simulate = FALSE;

    /* Start timed events to monitor status */
    batt_set_num (pt);
}

void batt_destructor (gpointer user_data)
{
    PtBattPlugin *pt = (PtBattPlugin *) user_data;

    wrap_free_gesture (pt->gesture);

    /* Disconnect the timer */
    if (pt->timer) g_source_remove (pt->timer);

    if (pt->batt) battery_free (pt->batt);
    if (pt->plug) g_object_unref (pt->plug);
    if (pt->flash) g_object_unref (pt->flash);

    g_free (pt);
}
