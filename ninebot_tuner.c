#include "ninebot_tuner.h"
#include <gui/elements.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

// ─────────────────────────────────────────────────────────────────────────────
// Payloads
// ─────────────────────────────────────────────────────────────────────────────

static const uint8_t max2G_payload[] = {
    0x55,0xAB,0x4D,0x41,0x58,0x32,0x53,0x63,0x6F,0x6F,0x74,0x65,0x72,0x5F,0x31
};
static const uint8_t f2_payload[] = {
    0x55,0xAB,0x46,0x32,0x53,0x63,0x6F,0x6F,0x74,0x65,0x72,0x5F,0x31
};
static const uint8_t gen1_payload[] = {
    0x5A,0xA5,0x00,0x4B,0x48,0x94,0xE3,0x3A,0x91,0xE0,0x32,0x7E,0xC2
};
static const uint8_t gen2_payload[] = {
    0x5A,0xA5,0x00,0x4B,0x48,0x94,0xE3,0x3A,0x91,0xE0,0x43,0x3E,0xC2
};
static const uint8_t gen3_payload[] = {
    0x5A,0xA5,0x00,0x4B,0x48,0x94,0xE3,0x3A,0x91,0xE0,0x42,0x7E,0xC4
};

typedef struct {
    const char*    label;
    const uint8_t* data;
    uint8_t        len;
} PayloadDef;

static const PayloadDef payloads[PAYLOAD_COUNT] = {
    {"Ninebot Max 2G/G30", max2G_payload, sizeof(max2G_payload)},
    {"Ninebot F2",         f2_payload,    sizeof(f2_payload)   },
    {"Ninebot Generic 1",  gen1_payload,  sizeof(gen1_payload) },
    {"Ninebot Generic 2",  gen2_payload,  sizeof(gen2_payload) },
    {"Ninebot Generic 3",  gen3_payload,  sizeof(gen3_payload) },
};

// ─────────────────────────────────────────────────────────────────────────────
// Beacon helpers
// ─────────────────────────────────────────────────────────────────────────────

static void beacon_stop(NinebotApp* app) {
    if(app->beacon_active) {
        furi_hal_bt_extra_beacon_stop();
        app->beacon_active = false;
        notification_message(app->notifications, &sequence_blink_stop);
        notification_message(app->notifications, &sequence_reset_rgb);
    }
}

static bool beacon_start(NinebotApp* app) {
    beacon_stop(app);

    const PayloadDef* pd = &payloads[app->selected_model];

    uint8_t adv[EXTRA_BEACON_MAX_DATA_SIZE];
    uint8_t pos = 0;
    uint8_t field_len = pd->len + 3; // AD type(1) + company(2) + data
    if((uint8_t)(field_len + 1) > EXTRA_BEACON_MAX_DATA_SIZE) return false;

    adv[pos++] = field_len;
    adv[pos++] = 0xFF; // Manufacturer Specific
    adv[pos++] = 0xFF; // Company ID lo
    adv[pos++] = 0xFF; // Company ID hi
    memcpy(&adv[pos], pd->data, pd->len);
    pos += pd->len;

    GapExtraBeaconConfig cfg = {
        .min_adv_interval_ms = 50,
        .max_adv_interval_ms = 100,
        .adv_channel_map     = GapAdvChannelMapAll,
        .adv_power_level     = GapAdvPowerLevel_6dBm,
        .address_type        = GapAddressTypeRandom,
        .address             = {0xDE,0xAD,0xBE,0xEF,0x00,0x00},
    };

    if(!furi_hal_bt_extra_beacon_set_config(&cfg)) return false;
    if(!furi_hal_bt_extra_beacon_set_data(adv, pos)) return false;
    if(!furi_hal_bt_extra_beacon_start()) return false;

    app->beacon_active = true;
    notification_message(app->notifications, &sequence_blink_start_cyan);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Main menu — custom View
// ─────────────────────────────────────────────────────────────────────────────

// Labels for the 3 main menu items (dynamic for broadcast)
static const char* MAIN_LABELS[NinebotMainItemCount] = {
    "Select Model",
    "Start Broadcast",
    "About",
};

typedef struct {
    NinebotApp* app;
} MainViewModel;

static void main_view_draw(Canvas* canvas, void* model) {
    MainViewModel* m = model;
    NinebotApp*    app = m->app;

    canvas_clear(canvas);

    // Header bar
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_box(canvas, 0, 0, 128, 14);
    canvas_set_color(canvas, ColorWhite);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 2, AlignCenter, AlignTop, "Ninebot Tuner");

    // Selected model + TX status
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontSecondary);
    if(app->beacon_active) {
        canvas_draw_str_aligned(canvas, 58, 16, AlignCenter, AlignTop,
            payloads[app->selected_model].label);
        canvas_draw_str_aligned(canvas, 124, 16, AlignRight, AlignTop, "*");
    } else {
        canvas_draw_str_aligned(canvas, 64, 16, AlignCenter, AlignTop,
            payloads[app->selected_model].label);
    }

    // Divider
    canvas_draw_line(canvas, 0, 26, 128, 26);

    // Menu items — 3 items, 12px each, starting at y=29
    for(uint8_t i = 0; i < NinebotMainItemCount; i++) {
        uint8_t y = 29 + i * 12;
        bool selected = (app->main_cursor == i);

        if(selected) {
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_rbox(canvas, 2, y - 1, 124, 11, 2);
            canvas_set_color(canvas, ColorWhite);
        } else {
            canvas_set_color(canvas, ColorBlack);
        }

        canvas_set_font(canvas, FontSecondary);

        const char* label;
        if(i == NinebotMainItemBroadcast) {
            label = app->beacon_active ? "Stop Broadcast" : "Start Broadcast";
        } else {
            label = MAIN_LABELS[i];
        }

        canvas_draw_str_aligned(canvas, 64, y, AlignCenter, AlignTop, label);
    }
}

static bool main_view_input(InputEvent* event, void* context) {
    NinebotApp* app = context;

    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return false;

    switch(event->key) {
    case InputKeyUp:
        if(app->main_cursor > 0) app->main_cursor--;
        with_view_model(app->view_main, MainViewModel* m, { m->app = app; }, true);
        return true;

    case InputKeyDown:
        if(app->main_cursor < NinebotMainItemCount - 1) app->main_cursor++;
        with_view_model(app->view_main, MainViewModel* m, { m->app = app; }, true);
        return true;

    case InputKeyOk:
        switch(app->main_cursor) {
        case NinebotMainItemSelect:
            app->current_view = NinebotViewSelectModel;
            view_dispatcher_switch_to_view(app->view_dispatcher, NinebotViewSelectModel);
            break;
        case NinebotMainItemBroadcast:
            if(app->beacon_active) {
                beacon_stop(app);
            } else {
                beacon_start(app);
            }
            with_view_model(app->view_main, MainViewModel* m, { m->app = app; }, true);
            break;
        case NinebotMainItemAbout:
            app->current_view = NinebotViewAbout;
            view_dispatcher_switch_to_view(app->view_dispatcher, NinebotViewAbout);
            break;
        }
        return true;

    default:
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Select Model submenu
// ─────────────────────────────────────────────────────────────────────────────

static void model_submenu_callback(void* context, uint32_t index) {
    NinebotApp* app = context;
    if(index < PAYLOAD_COUNT) {
        // If broadcast was running — restart with new model
        bool was_active = app->beacon_active;
        if(was_active) beacon_stop(app);
        app->selected_model = (uint8_t)index;
        if(was_active) beacon_start(app);
    }
    // Return to main regardless
    app->current_view = NinebotViewMain;
    with_view_model(app->view_main, MainViewModel* m, { m->app = app; }, true);
    view_dispatcher_switch_to_view(app->view_dispatcher, NinebotViewMain);
}

// ─────────────────────────────────────────────────────────────────────────────
// Navigation back callback
// ─────────────────────────────────────────────────────────────────────────────

static bool navigation_callback(void* context) {
    NinebotApp* app = context;
    if(app->current_view == NinebotViewMain) {
        beacon_stop(app);
        view_dispatcher_stop(app->view_dispatcher);
    } else {
        app->current_view = NinebotViewMain;
        view_dispatcher_switch_to_view(app->view_dispatcher, NinebotViewMain);
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// App entry point
// ─────────────────────────────────────────────────────────────────────────────

int32_t ninebot_tuner_app(void* p) {
    UNUSED(p);

    NinebotApp* app = malloc(sizeof(NinebotApp));
    memset(app, 0, sizeof(NinebotApp));
    app->selected_model = 0;
    app->main_cursor    = 0;
    app->current_view   = NinebotViewMain;

    app->gui             = furi_record_open(RECORD_GUI);
    app->notifications   = furi_record_open(RECORD_NOTIFICATION);
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, navigation_callback);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // ── Main View ──────────────────────────────────────────────────────────
    app->view_main = view_alloc();
    view_set_context(app->view_main, app);
    view_allocate_model(app->view_main, ViewModelTypeLocking, sizeof(MainViewModel));
    with_view_model(app->view_main, MainViewModel* m, { m->app = app; }, false);
    view_set_draw_callback(app->view_main, main_view_draw);
    view_set_input_callback(app->view_main, main_view_input);
    view_dispatcher_add_view(app->view_dispatcher, NinebotViewMain, app->view_main);

    // ── Select Model Submenu ───────────────────────────────────────────────
    app->submenu_model = submenu_alloc();
    submenu_set_header(app->submenu_model, "Select Scooter Model");
    for(uint8_t i = 0; i < PAYLOAD_COUNT; i++) {
        submenu_add_item(app->submenu_model, payloads[i].label, i, model_submenu_callback, app);
    }
    view_dispatcher_add_view(
        app->view_dispatcher, NinebotViewSelectModel, submenu_get_view(app->submenu_model));

    // ── About Widget ───────────────────────────────────────────────────────
    app->widget_about = widget_alloc();
    widget_add_string_element(
        app->widget_about, 64, 0, AlignCenter, AlignTop, FontPrimary, "About");
    widget_add_text_scroll_element(
        app->widget_about, 0, 13, 128, 51,
        "Ninebot Tuner v" APP_VERSION "\n"
        "by enexis1337\n"
        "\n"
        APP_GITHUB "\n"
        "\n"
        "Sends BLE tuning payloads\n"
        "to Ninebot scooters via\n"
        "BLE advertisement broadcast.\n");
    view_dispatcher_add_view(
        app->view_dispatcher, NinebotViewAbout, widget_get_view(app->widget_about));

    // ── Start ──────────────────────────────────────────────────────────────
    view_dispatcher_switch_to_view(app->view_dispatcher, NinebotViewMain);
    view_dispatcher_run(app->view_dispatcher);

    // ── Cleanup ────────────────────────────────────────────────────────────
    beacon_stop(app);

    view_dispatcher_remove_view(app->view_dispatcher, NinebotViewMain);
    view_dispatcher_remove_view(app->view_dispatcher, NinebotViewSelectModel);
    view_dispatcher_remove_view(app->view_dispatcher, NinebotViewAbout);

    view_free(app->view_main);
    submenu_free(app->submenu_model);
    widget_free(app->widget_about);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_GUI);
    free(app);

    return 0;
}
