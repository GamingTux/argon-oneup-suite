/*============================================================================
Copyright (c) 2024 Raspberry Pi
All rights reserved.
============================================================================*/

#include <glibmm.h>
#include "batt.hpp"

extern "C" {
    PanelWidget *create () { return new WidgetBatt; }
    void destroy (PanelWidget *w) { delete w; }

    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return PLUGIN_TITLE; };
    const char *display_desc (void) { return PLUGIN_DESC; };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}

void WidgetBatt::widget_set_icon (void)
{
    batt_update_display (pt);
}

void WidgetBatt::widget_config_reload (void)
{
    if (load_configuration_data (PLUGIN_NAME, conf_table)) batt_set_num (pt);
}

void WidgetBatt::widget_init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name (PLUGIN_NAME);
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    pt = g_new0 (PtBattPlugin, 1);
    pt->plugin = (GtkWidget *)((*plugin).gobj());

    /* Initialise the plugin */
    batt_set_values (pt);
    load_configuration_data (PLUGIN_NAME, conf_table);
    batt_init (pt);
}

WidgetBatt::~WidgetBatt()
{
    batt_destructor (pt);
}
