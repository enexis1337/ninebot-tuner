#pragma once

#include <furi.h>
#include <furi_hal_bt.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/widget.h>
#include <gui/modules/variable_item_list.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

#define APP_VERSION   "1.0.0"
#define APP_GITHUB    "https://github.com/enexis1337/ninebot-tuner"
#define PAYLOAD_COUNT 5

typedef enum {
    NinebotViewMain,       // главное меню (custom draw)
    NinebotViewSelectModel,// submenu выбора модели
    NinebotViewAbout,      // widget about
} NinebotView;

typedef enum {
    NinebotMainItemSelect = 0,
    NinebotMainItemBroadcast,
    NinebotMainItemAbout,
    NinebotMainItemCount,
} NinebotMainItem;

typedef struct {
    Gui*            gui;
    ViewDispatcher* view_dispatcher;

    // Main menu — кастомный View
    View*           view_main;

    // Select model submenu
    Submenu*        submenu_model;

    // About widget
    Widget*         widget_about;

    NotificationApp* notifications;
    uint8_t         selected_model;   // индекс выбранной модели
    uint8_t         main_cursor;      // текущий пункт главного меню
    bool            beacon_active;
    NinebotView     current_view;     // для навигации back
} NinebotApp;
