/*============================================================================
Copyright (c) 2024 Raspberry Pi
All rights reserved.
============================================================================*/

#ifndef WIDGETS_BATT_HPP
#define WIDGETS_BATT_HPP

#include <widget.hpp>
#include <gtkmm/button.h>

extern "C" {
#include "plugin.h"
#include "batt.h"
}

class WidgetBatt : public PanelWidget
{
    PtBattPlugin *pt;

    std::unique_ptr <Gtk::Button> plugin;

  public:

    void widget_init (Gtk::HBox *container) override;
    virtual ~WidgetBatt ();
    void widget_set_icon (void);
    void widget_config_reload (void);
};

#endif /* end of include guard: WIDGETS_BATT_HPP */
