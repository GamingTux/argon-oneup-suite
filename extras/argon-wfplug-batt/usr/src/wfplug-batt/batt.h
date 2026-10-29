/*============================================================================
Copyright (c) 2018-2025 Raspberry Pi
All rights reserved.
============================================================================*/

#include "batt_sys.h"

#define PLUGIN_TITLE N_("Battery")
#define PLUGIN_DESC N_("Monitors laptop battery")

typedef struct 
{
    GtkWidget *plugin;
    GtkGesture *gesture;
    GtkWidget *tray_icon;           /* Displayed image */
    battery *batt;
    GdkPixbuf *plug;
    GdkPixbuf *flash;
    guint timer;
    guint vtimer;
    int batt_num;
    gboolean simulate;
    LXPLUG_VARS
} PtBattPlugin;

extern conf_table_t conf_table[2];

extern void batt_init (PtBattPlugin *pt);
extern void batt_set_values (PtBattPlugin *pt);
extern void batt_update_display (PtBattPlugin *pt);
extern void batt_set_num (PtBattPlugin *pt);
extern void batt_destructor (gpointer user_data);
