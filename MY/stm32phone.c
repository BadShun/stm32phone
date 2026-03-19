#include "stm32phone.h"
#include "phone_bg_img.h"
#include "files_icon.h"
#include "gallery_icon.h"
#include "gameboy_icon.h"
#include "camera_icon.h"
#include "message_icon.h"
#include "app_launcher.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdbool.h>

#define APP_ICON_SIZE 40
#define DOCK_HEIGHT 60

#define BAR_WIDTH 80
#define BAR_HEIGHT 7

#define BAR_INITIAL_POS_Y 4
#define BAR_SLIDE_THRESHOLD 20
#define SLIDE_TIME_THRESHOLD 500

#define NOTIFICATION_BAR_WIDTH 20
#define NOTIFICATION_BAR_HEIGTH 20

#define NOTIFICATION_BAR_EXPAND_WIDTH 140

#define LAUNCH_ANIM_SPEED 200

static struct {
    lv_coord_t x;
    lv_coord_t y;
    lv_obj_t *mask_obj;
    lv_img_dsc_t *app_icon;
    uint8_t app_id;
} launched_app = { 0 };

typedef struct {
    const char *sender;
    const char *content;
} Notification;

typedef struct {
    lv_obj_t *sender_text;
    lv_obj_t *content_text;
} Notification_TextAreas;

static void create_slide_bar(lv_obj_t *parent);
static void slide_bar_expand_anim_cb(void *var, int32_t v);
static void slide_bar_move_expand_anim_cb(void *var, int32_t v);
static void create_dock_app(lv_obj_t *outer_cont, lv_img_dsc_t **app_icons);
static void dock_icon_anim_cb(void *var, int32_t v);
static void drag_slide_bar_cb(lv_event_t *event);
static void release_slide_bar_cb(lv_event_t *event);
static void release_slide_bar_anim_cb(void *var, int32_t v);
static void back_to_desktop(lv_anim_t *anim);
static void launch_app_cb(lv_event_t *event);
static void launch_app_anim_mask_resize_cb(void *var, int32_t v);
static void set_x_anim_cb(void *var, int32_t v);
static void set_y_anim_cb(void *var, int32_t v);
static void launch_app_anim_ready_cb(lv_anim_t *anim);
static void launch_app_anim_icon_zoom_cb(void *var, int32_t v);
static void launch_app_scr(lv_anim_t *anim);
static void del_icon_mask(lv_anim_t *anim);
static void send_btn_event_cb(lv_event_t *event);
static void create_notification_bar(Notification *notification);
static void notification_bar_expand_anim(lv_anim_t *anim);
static void resize_notification_cb(void *var, int32_t v);
static void set_notification_icon(lv_anim_t *anim);
static void notification_bar_clicked_cb(lv_event_t *event);
static void resize_notification_show_cb(void *var, int32_t v);
static void notification_read_cb(lv_event_t *event);
static void resize_width_notification_read_cb(void *var, int32_t v);
static void resize_height_notification_read_cb(void *var, int32_t v);
static void notification_to_circle(lv_anim_t *anim);

const static lv_img_dsc_t *g_app_icons[] = {
    &gameboy_icon, &files_icon, &camera_icon, &gallery_icon
};

static lv_img_dsc_t *launched_app_icon;

static lv_obj_t *desktop_src;
static lv_obj_t *app_src;

static uint32_t slide_start_time = 0;

static bool in_desktop = true;

void phone_desktop() {
    desktop_src = lv_obj_create(NULL);
    lv_scr_load(desktop_src);

    lv_obj_t *scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    LV_IMG_DECLARE(phone_bg_img);
    lv_obj_t *bg_img = lv_img_create(scr);
    lv_img_set_src(bg_img, &phone_bg_img);
    lv_obj_align(bg_img, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_move_background(bg_img);

    lv_obj_t *dock = lv_obj_create(scr);
    lv_obj_set_size(dock, LV_HOR_RES - 10, DOCK_HEIGHT);
    lv_obj_set_style_bg_color(dock, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(dock, 230, 0);
    lv_obj_set_style_pad_all(dock, 0, 0);
    lv_obj_clear_flag(dock, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(dock, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_t *dock_cont = lv_obj_create(dock);
    lv_obj_set_size(dock_cont, LV_HOR_RES - 10, APP_ICON_SIZE);
    lv_obj_align(dock_cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(dock_cont, LV_OPA_0, 0);
    lv_obj_clear_flag(dock_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(dock_cont, 0, 0);
    lv_obj_set_style_border_width(dock_cont, 0, 0);
    lv_obj_set_flex_flow(dock_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dock_cont,
        LV_FLEX_ALIGN_SPACE_EVENLY,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);

    LV_IMG_DECLARE(camera_icon);
    LV_IMG_DECLARE(gameboy_icon);
    LV_IMG_DECLARE(files_icon);
    LV_IMG_DECLARE(gallery_icon);

    create_dock_app(dock_cont, g_app_icons);
    create_slide_bar(scr);
}

static void create_slide_bar(lv_obj_t *parent) {
    lv_obj_t *slide_bar = lv_obj_create(parent);
    lv_obj_set_size(slide_bar, 0, BAR_HEIGHT);
    lv_obj_set_style_bg_color(slide_bar, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slide_bar, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_border_width(slide_bar, 0, 0);
    lv_obj_set_pos(slide_bar, (LV_HOR_RES - BAR_WIDTH) / 2 + BAR_WIDTH / 2, LV_VER_RES - BAR_HEIGHT - BAR_INITIAL_POS_Y);
    lv_obj_add_event_cb(slide_bar, drag_slide_bar_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(slide_bar, release_slide_bar_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(slide_bar, release_slide_bar_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_move_foreground(slide_bar);

    lv_anim_t len_anim;
    lv_anim_init(&len_anim);
    lv_anim_set_var(&len_anim, slide_bar);
    lv_anim_set_values(&len_anim, 0, BAR_WIDTH);
    lv_anim_set_time(&len_anim, 500);
    lv_anim_set_exec_cb(&len_anim, slide_bar_expand_anim_cb);
    lv_anim_set_path_cb(&len_anim, lv_anim_path_overshoot);
    lv_anim_start(&len_anim);

    lv_anim_t move_anim;
    lv_anim_init(&move_anim);
    lv_anim_set_var(&move_anim, slide_bar);
    lv_anim_set_values(&move_anim, (LV_HOR_RES - BAR_WIDTH) / 2 + BAR_WIDTH / 2, (LV_HOR_RES - BAR_WIDTH) / 2);
    lv_anim_set_time(&move_anim, 500);
    lv_anim_set_exec_cb(&move_anim, slide_bar_move_expand_anim_cb);
    lv_anim_set_path_cb(&move_anim, lv_anim_path_overshoot);
    lv_anim_start(&move_anim);
}

static void slide_bar_expand_anim_cb(void *var, int32_t v) {
    lv_obj_set_size(var, v, BAR_HEIGHT);
}

static void slide_bar_move_expand_anim_cb(void *var, int32_t v) {
    lv_obj_set_pos(var, v, LV_VER_RES - BAR_HEIGHT - BAR_INITIAL_POS_Y);
}

static void create_dock_app(lv_obj_t *outer_cont, lv_img_dsc_t **app_icons) {
    for (int i = 0; i < 4; i++) {
        lv_obj_t *app = lv_obj_create(outer_cont);
        lv_obj_set_size(app, APP_ICON_SIZE, APP_ICON_SIZE);
        lv_obj_set_style_border_width(app, 0, 0);
        lv_obj_add_event_cb(app, launch_app_cb, LV_EVENT_SHORT_CLICKED, app_icons[i]);
        lv_obj_clear_flag(app, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(app, 0, 0);
        
        lv_obj_t *app_icon = lv_img_create(app);
        lv_img_set_src(app_icon, app_icons[i]);
        lv_obj_align(app_icon, LV_ALIGN_TOP_LEFT, 0, 0);

        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, app_icon);
        lv_anim_set_values(&anim, 0, 255);
        lv_anim_set_time(&anim, 500);
        lv_anim_set_exec_cb(&anim, dock_icon_anim_cb);
        lv_anim_set_path_cb(&anim, lv_anim_path_overshoot);
        lv_anim_start(&anim);
    }
}

static void dock_icon_anim_cb(void *var, int32_t v) {
    lv_img_set_zoom(var, v);
}

static void drag_slide_bar_cb(lv_event_t *event) {
    lv_obj_t *obj = lv_event_get_target(event);

    if (lv_event_get_code(event) == LV_EVENT_PRESSING) {
        slide_start_time = lv_tick_get();

        lv_indev_t *indev = lv_indev_get_act();
        lv_point_t vect;

        lv_indev_get_vect(indev, &vect);

        if ((lv_obj_get_y(obj) + vect.y) < (LV_VER_RES - BAR_HEIGHT - BAR_INITIAL_POS_Y) &&
            (lv_obj_get_y(obj) + vect.y) > (LV_VER_RES - BAR_HEIGHT - BAR_INITIAL_POS_Y - BAR_SLIDE_THRESHOLD)) {
            lv_obj_set_pos(obj, lv_obj_get_x(obj), lv_obj_get_y(obj) + vect.y);
        }
    }
}

static void release_slide_bar_cb(lv_event_t *event) {
    lv_obj_t *obj = lv_event_get_target(event);

    if (lv_event_get_code(event) == LV_EVENT_PRESS_LOST || lv_event_get_code(event) == LV_EVENT_RELEASED) {
        uint32_t during = lv_tick_elaps(slide_start_time);

        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, obj);
        lv_anim_set_values(&anim, lv_obj_get_y(obj), LV_VER_RES - BAR_HEIGHT - BAR_INITIAL_POS_Y);
        lv_anim_set_time(&anim, 200);
        lv_anim_set_exec_cb(&anim, release_slide_bar_anim_cb);
        lv_anim_set_path_cb(&anim, lv_anim_path_overshoot);
        lv_anim_set_ready_cb(&anim, back_to_desktop);
        lv_anim_start(&anim);
    }
}

static void release_slide_bar_anim_cb(void *var, int32_t v) {
    lv_obj_set_y(var, v);
}

static void back_to_desktop(lv_anim_t *anim) {
    if (in_desktop) {
        return;
    }

    in_desktop = true;
    lv_scr_load(desktop_src);

    lv_obj_t *scr = lv_scr_act();

    lv_obj_t *app_icon = lv_img_create(scr);
    lv_img_set_src(app_icon, launched_app.app_icon);
    lv_img_set_zoom(app_icon, 256 * 6);
    lv_obj_set_pos(app_icon, (LV_HOR_RES - APP_ICON_SIZE) / 2 - (launched_app.x - LV_HOR_RES / 2), (LV_VER_RES - APP_ICON_SIZE) / 2);

    launched_app.mask_obj = app_icon;

    lv_area_t coords;
    lv_obj_get_coords(app_icon, &coords);

    lv_anim_t pos_x_anim;
    lv_anim_init(&pos_x_anim);
    lv_anim_set_var(&pos_x_anim, app_icon);
    lv_anim_set_values(&pos_x_anim, (LV_HOR_RES - APP_ICON_SIZE) / 2 - (launched_app.x - LV_HOR_RES / 2), launched_app.x);
    lv_anim_set_time(&pos_x_anim, 300);
    lv_anim_set_exec_cb(&pos_x_anim, set_x_anim_cb);
    lv_anim_set_path_cb(&pos_x_anim, lv_anim_path_linear);
    lv_anim_start(&pos_x_anim);

    lv_anim_t pos_y_anim;
    lv_anim_init(&pos_y_anim);
    lv_anim_set_var(&pos_y_anim, app_icon);
    lv_anim_set_values(&pos_y_anim, coords.y1, launched_app.y);
    lv_anim_set_time(&pos_y_anim, 300);
    lv_anim_set_exec_cb(&pos_y_anim, set_y_anim_cb);
    lv_anim_set_path_cb(&pos_y_anim, lv_anim_path_linear);
    lv_anim_start(&pos_y_anim);

    lv_anim_t zoom_anim;
    lv_anim_init(&zoom_anim);
    lv_anim_set_var(&zoom_anim, app_icon);
    lv_anim_set_values(&zoom_anim, 256 * 6 , 256);
    lv_anim_set_time(&zoom_anim, 300);
    lv_anim_set_exec_cb(&zoom_anim, launch_app_anim_icon_zoom_cb);
    lv_anim_set_path_cb(&zoom_anim, lv_anim_path_linear);
    lv_anim_set_ready_cb(&zoom_anim, del_icon_mask);
    lv_anim_start(&zoom_anim);
}

static void del_icon_mask(lv_anim_t *anim) {
    lv_obj_del(launched_app.mask_obj);
    lv_obj_invalidate(lv_scr_act());
}

static void launch_app_cb(lv_event_t *event) {
    lv_obj_t *app_obj = lv_event_get_target(event);
    launched_app_icon = (lv_img_dsc_t *)lv_event_get_user_data(event);

    if ((lv_event_get_code(event) == LV_EVENT_SHORT_CLICKED)) {
        lv_area_t coords;
        lv_obj_get_coords(app_obj, &coords);

        lv_obj_t *mask_obj = lv_obj_create(lv_scr_act());
        lv_obj_set_pos(mask_obj, coords.x1, coords.y1);
        lv_obj_set_size(mask_obj, APP_ICON_SIZE, APP_ICON_SIZE);
        lv_obj_set_style_border_width(mask_obj, 0, 0);
        lv_obj_clear_flag(mask_obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(mask_obj, 0, 0);
        lv_obj_set_pos(mask_obj, 0, 0);

        launched_app.mask_obj = mask_obj;
        launched_app.x = coords.x1;
        launched_app.y = coords.y1;
        launched_app.app_icon = launched_app_icon;

        lv_anim_t pos_x_anim;
        lv_anim_init(&pos_x_anim);
        lv_anim_set_var(&pos_x_anim, mask_obj);
        lv_anim_set_values(&pos_x_anim, coords.x1, 0);
        lv_anim_set_time(&pos_x_anim, LAUNCH_ANIM_SPEED);
        lv_anim_set_exec_cb(&pos_x_anim, set_x_anim_cb);
        lv_anim_set_path_cb(&pos_x_anim, lv_anim_path_linear);
        lv_anim_start(&pos_x_anim);

        lv_anim_t pos_y_anim;
        lv_anim_init(&pos_y_anim);
        lv_anim_set_var(&pos_y_anim, mask_obj);
        lv_anim_set_values(&pos_y_anim, coords.y1, 0);
        lv_anim_set_time(&pos_y_anim, LAUNCH_ANIM_SPEED);
        lv_anim_set_exec_cb(&pos_y_anim, set_y_anim_cb);
        lv_anim_set_path_cb(&pos_y_anim, lv_anim_path_linear);
        lv_anim_start(&pos_y_anim);

        lv_anim_t size_anim;
        lv_anim_init(&size_anim);
        lv_anim_set_var(&size_anim, mask_obj);
        lv_anim_set_values(&size_anim, APP_ICON_SIZE, LV_VER_RES);
        lv_anim_set_time(&size_anim, LAUNCH_ANIM_SPEED);
        lv_anim_set_exec_cb(&size_anim, launch_app_anim_mask_resize_cb);
        lv_anim_set_path_cb(&size_anim, lv_anim_path_linear);
        lv_anim_set_ready_cb(&size_anim, launch_app_anim_ready_cb);
        lv_anim_start(&size_anim);
    }
}

static void launch_app_anim_mask_resize_cb(void *var, int32_t v) {
    lv_obj_set_size(var, v * 0.75, v);
}

static void set_x_anim_cb(void *var, int32_t v) {
    lv_obj_set_x(var, v);
}

static void set_y_anim_cb(void *var, int32_t v) {
    lv_obj_set_y(var, v);
}

static void launch_app_anim_ready_cb(lv_anim_t *anim) {
    lv_obj_set_style_radius(anim->var, 0, 0);

    lv_obj_t *app_icon = lv_img_create(anim->var);
    lv_img_set_src(app_icon, launched_app_icon);
    lv_obj_center(app_icon);

    lv_anim_t zoom_anim;
    lv_anim_init(&zoom_anim);
    lv_anim_set_var(&zoom_anim, app_icon);
    lv_anim_set_values(&zoom_anim, 0, 256);
    lv_anim_set_time(&zoom_anim, 400);
    lv_anim_set_exec_cb(&zoom_anim, launch_app_anim_icon_zoom_cb);
    lv_anim_set_path_cb(&zoom_anim, lv_anim_path_overshoot);
    lv_anim_set_ready_cb(&zoom_anim, launch_app_scr);
    lv_anim_start(&zoom_anim);
}

static void launch_app_anim_icon_zoom_cb(void *var, int32_t v) {
    lv_img_set_zoom(var, v);
}

static void launch_app_scr(lv_anim_t *anim) {
    app_src = lv_obj_create(NULL);
    lv_scr_load(app_src);

    in_desktop = false;

    lv_obj_del(launched_app.mask_obj);
	
	Launcher_Download("camera");
	Launch_App();

    lv_obj_t *scr = lv_scr_act();

    lv_obj_t *sender_text = lv_textarea_create(scr);
    lv_textarea_set_one_line(sender_text, true);
    lv_textarea_set_password_mode(sender_text, false);
    lv_obj_set_width(sender_text, lv_pct(60));
    lv_obj_align(sender_text, LV_ALIGN_TOP_MID, 0, 100);

    lv_obj_t *sender_label = lv_label_create(scr);
    lv_label_set_text(sender_label, "Sender:");
    lv_obj_align_to(sender_label, sender_text, LV_ALIGN_OUT_TOP_LEFT, 0, -10);

    lv_obj_t *content_text = lv_textarea_create(scr);
    lv_textarea_set_one_line(content_text, true);
    lv_textarea_set_password_mode(content_text, false);
    lv_obj_set_width(content_text, lv_pct(60));
    lv_obj_align(content_text, LV_ALIGN_TOP_MID, 0, 170);

    lv_obj_t *content_label = lv_label_create(scr);
    lv_label_set_text(content_label, "Content:");
    lv_obj_align_to(content_label, content_text, LV_ALIGN_OUT_TOP_LEFT, 0, -10);

    static Notification_TextAreas notification_ta;
    notification_ta.sender_text = sender_text;
    notification_ta.content_text = content_text;

    lv_obj_t *send_btn = lv_btn_create(scr);
    lv_obj_add_event_cb(send_btn, send_btn_event_cb, LV_EVENT_CLICKED, &notification_ta);
    lv_obj_align(send_btn, LV_ALIGN_TOP_MID, 0, 230);

    lv_obj_t *send_btn_label = lv_label_create(send_btn);
    lv_label_set_text(send_btn_label, "Send");
    lv_obj_center(send_btn_label);

    create_slide_bar(scr);
}

static void send_btn_event_cb(lv_event_t *event) {
    Notification_TextAreas *data = lv_event_get_user_data(event);
    static Notification notification;
    notification.sender = lv_textarea_get_text(data->sender_text);
    notification.content = lv_textarea_get_text(data->content_text);
    create_notification_bar(&notification);
}

static void create_notification_bar(Notification *notification) {
    lv_obj_t *scr = lv_scr_act();

    lv_obj_t *notification_bar = lv_obj_create(scr);
    lv_obj_clear_flag(notification_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(notification_bar, lv_color_black(), 0);
    lv_obj_set_size(notification_bar, NOTIFICATION_BAR_WIDTH,  NOTIFICATION_BAR_HEIGTH);
    lv_obj_set_pos(notification_bar, (LV_HOR_RES - NOTIFICATION_BAR_WIDTH) / 2, -NOTIFICATION_BAR_HEIGTH);
    lv_obj_set_style_radius(notification_bar, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_user_data(notification_bar, notification);

    lv_anim_t move_anim;
    lv_anim_init(&move_anim);
    lv_anim_set_var(&move_anim, notification_bar);
    lv_anim_set_values(&move_anim, -NOTIFICATION_BAR_HEIGTH,  20);
    lv_anim_set_time(&move_anim, 300);
    lv_anim_set_exec_cb(&move_anim, set_y_anim_cb);
    lv_anim_set_path_cb(&move_anim, lv_anim_path_overshoot);
    lv_anim_set_ready_cb(&move_anim, notification_bar_expand_anim);
    lv_anim_start(&move_anim);
}

static void notification_bar_expand_anim(lv_anim_t *anim) {
    lv_obj_t *notification_bar = anim->var;
    lv_obj_set_pos(notification_bar, (LV_HOR_RES - NOTIFICATION_BAR_WIDTH) / 2, 20);

    lv_anim_t pos_x_anim;
    lv_anim_init(&pos_x_anim);
    lv_anim_set_var(&pos_x_anim, notification_bar);
    lv_anim_set_values(&pos_x_anim, (LV_HOR_RES - NOTIFICATION_BAR_WIDTH) / 2, (LV_HOR_RES - NOTIFICATION_BAR_EXPAND_WIDTH) / 2);
    lv_anim_set_time(&pos_x_anim, 200);
    lv_anim_set_exec_cb(&pos_x_anim, set_x_anim_cb);
    lv_anim_set_path_cb(&pos_x_anim, lv_anim_path_linear);
    lv_anim_start(&pos_x_anim);

    lv_anim_t pos_y_anim;
    lv_anim_init(&pos_y_anim);
    lv_anim_set_var(&pos_y_anim, notification_bar);
    lv_anim_set_values(&pos_y_anim, 20,  20 - NOTIFICATION_BAR_EXPAND_WIDTH / 4 / 2);
    lv_anim_set_time(&pos_y_anim, LAUNCH_ANIM_SPEED);
    lv_anim_set_exec_cb(&pos_y_anim, set_y_anim_cb);
    lv_anim_set_path_cb(&pos_y_anim, lv_anim_path_linear);
    lv_anim_start(&pos_y_anim);

    lv_anim_t size_anim;
    lv_anim_init(&size_anim);
    lv_anim_set_var(&size_anim, notification_bar);
    lv_anim_set_values(&size_anim, NOTIFICATION_BAR_WIDTH, NOTIFICATION_BAR_EXPAND_WIDTH);
    lv_anim_set_time(&size_anim, 200);
    lv_anim_set_exec_cb(&size_anim, resize_notification_cb);
    lv_anim_set_path_cb(&size_anim, lv_anim_path_linear);
    lv_anim_set_ready_cb(&size_anim, set_notification_icon);
    lv_anim_start(&size_anim);
}

static void resize_notification_cb(void *var, int32_t v) {
    lv_obj_set_size(var, v, v / 4);
}

static void set_notification_icon(lv_anim_t *anim) {
    LV_IMG_DECLARE(message_icon);

    lv_obj_t *notification_bar = anim->var;

    lv_obj_t *icon = lv_img_create(notification_bar);
    lv_img_set_src(icon, &message_icon);
    lv_obj_set_align(icon, LV_ALIGN_LEFT_MID);

    lv_obj_t *notice_label = lv_label_create(notification_bar);
    lv_label_set_text(notice_label, "new message");
    lv_obj_set_style_text_color(notice_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(notice_label, &lv_font_montserrat_10, 0);
    lv_obj_set_align(notice_label, LV_ALIGN_RIGHT_MID);

    lv_obj_add_event_cb(notification_bar, notification_bar_clicked_cb, LV_EVENT_CLICKED, NULL);
}

static void notification_bar_clicked_cb(lv_event_t *event) {
    lv_obj_t *notification_bar = event->current_target;
    
    lv_obj_set_size(notification_bar, 210, 70);
    lv_obj_set_pos(notification_bar, (LV_HOR_RES - 210) / 2, 20);
    lv_obj_set_style_radius(notification_bar, 20, 0);

    lv_obj_t *icon = lv_obj_get_child(notification_bar, 0);
    lv_img_set_zoom(icon, 256* 1.5);

    Notification *notification = lv_obj_get_user_data(notification_bar);

    lv_obj_t *sender_label = lv_label_create(notification_bar);
    lv_obj_set_style_text_color(sender_label, lv_color_white(), 0);
    lv_label_set_text(sender_label, notification->sender);
    lv_obj_set_style_text_font(sender_label, &lv_font_montserrat_14, 0);
    lv_obj_align_to(sender_label, icon, LV_ALIGN_OUT_RIGHT_TOP, 10, -5);

    lv_obj_t *content_label = lv_label_create(notification_bar);
    lv_obj_set_style_text_color(content_label, lv_color_white(), 0);
    lv_label_set_text(content_label, notification->content);
    lv_obj_set_style_text_font(content_label, &lv_font_montserrat_10, 0);
    lv_obj_align_to(content_label, icon, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 5);

    lv_obj_t *notice_label = lv_obj_get_child(notification_bar, 1);
    lv_obj_set_style_opa(notice_label, LV_OPA_0, 0);

    lv_obj_t *read_btn = lv_btn_create(notification_bar);
    lv_obj_set_style_bg_color(read_btn, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_outline_opa(read_btn, LV_OPA_0, 0);
    lv_obj_set_align(read_btn, LV_ALIGN_RIGHT_MID);
    lv_obj_add_event_cb(read_btn, notification_read_cb, LV_EVENT_CLICKED, notification_bar);

    lv_obj_t *btn_label = lv_label_create(read_btn);
    lv_label_set_text(btn_label, "Read");
    lv_obj_center(btn_label);

    lv_anim_t pos_x_anim;
    lv_anim_init(&pos_x_anim);
    lv_anim_set_var(&pos_x_anim, notification_bar);
    lv_anim_set_values(&pos_x_anim, (LV_HOR_RES - NOTIFICATION_BAR_EXPAND_WIDTH) / 2, (LV_HOR_RES - 210) / 2);
    lv_anim_set_time(&pos_x_anim, 200);
    lv_anim_set_exec_cb(&pos_x_anim, set_x_anim_cb);
    lv_anim_set_path_cb(&pos_x_anim, lv_anim_path_linear);
    lv_anim_start(&pos_x_anim);

    lv_anim_t size_anim;
    lv_anim_init(&size_anim);
    lv_anim_set_var(&size_anim, notification_bar);
    lv_anim_set_values(&size_anim, NOTIFICATION_BAR_EXPAND_WIDTH, 210);
    lv_anim_set_time(&size_anim, 200);
    lv_anim_set_exec_cb(&size_anim, resize_notification_show_cb);
    lv_anim_set_path_cb(&size_anim, lv_anim_path_linear);
    lv_anim_start(&size_anim);
}

static void resize_notification_show_cb(void *var, int32_t v) {
    lv_obj_set_size(var, v, v / 3);
}

static void notification_read_cb(lv_event_t *event) {
    lv_obj_t *notification_bar = event->user_data;
    lv_obj_clean(notification_bar);

    lv_anim_t pos_x_anim;
    lv_anim_init(&pos_x_anim);
    lv_anim_set_var(&pos_x_anim, notification_bar);
    lv_anim_set_values(&pos_x_anim, (LV_HOR_RES - 210) / 2, (LV_HOR_RES - NOTIFICATION_BAR_WIDTH) / 2);
    lv_anim_set_time(&pos_x_anim, 200);
    lv_anim_set_exec_cb(&pos_x_anim, set_x_anim_cb);
    lv_anim_set_path_cb(&pos_x_anim, lv_anim_path_linear);
    lv_anim_start(&pos_x_anim);

    lv_anim_t size_width_anim;
    lv_anim_init(&size_width_anim);
    lv_anim_set_var(&size_width_anim, notification_bar);
    lv_anim_set_values(&size_width_anim, 210, NOTIFICATION_BAR_WIDTH);
    lv_anim_set_time(&size_width_anim, 200);
    lv_anim_set_exec_cb(&size_width_anim, resize_width_notification_read_cb);
    lv_anim_set_path_cb(&size_width_anim, lv_anim_path_linear);
    lv_anim_start(&size_width_anim);

    lv_anim_t size_height_anim;
    lv_anim_init(&size_height_anim);
    lv_anim_set_var(&size_height_anim, notification_bar);
    lv_anim_set_values(&size_height_anim, 70, NOTIFICATION_BAR_HEIGTH);
    lv_anim_set_time(&size_height_anim, 200);
    lv_anim_set_exec_cb(&size_height_anim, resize_height_notification_read_cb);
    lv_anim_set_path_cb(&size_height_anim, lv_anim_path_linear);
    lv_anim_set_ready_cb(&size_height_anim, notification_to_circle);
    lv_anim_start(&size_height_anim);
}

static void resize_width_notification_read_cb(void *var, int32_t v) {
    lv_obj_set_width(var, v);
}

static void resize_height_notification_read_cb(void *var, int32_t v) {
    lv_obj_set_height(var, v);
}

static void notification_to_circle(lv_anim_t *anim) {
    lv_obj_set_style_radius(anim->var, LV_RADIUS_CIRCLE, 0);

    lv_anim_t move_anim;
    lv_anim_init(&move_anim);
    lv_anim_set_var(&move_anim, anim->var);
    lv_anim_set_values(&move_anim, 20, -NOTIFICATION_BAR_HEIGTH);
    lv_anim_set_time(&move_anim, 300);
    lv_anim_set_exec_cb(&move_anim, set_y_anim_cb);
    lv_anim_set_path_cb(&move_anim, lv_anim_path_linear);
    lv_anim_start(&move_anim);
}
