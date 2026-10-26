// Grimoire: a pendulum, a sigil, a talisman and a wand for the Flipper Zero.
//
// Every page shares the planetary hour (hours.c): whose hour it is now and
// until when, computed on the device from the clock and Portland's sunrise.

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>
#include <notification/notification_messages.h>
#include <math.h>
#include "hours.h"

#define TICK_MS 50
#define W       128
#define H       64

// Where the sun is reckoned from. The Flipper's clock is local time.
static const HoursPlace place = {
    .latitude = 45.5152f,
    .longitude = -122.6784f,
    .utc_offset = -8 * 60,
    .us_dst = true,
};

typedef enum {
    PagePendulum,
    PageSigil,
    PageTalisman,
    PageWand,
    PageCount,
} Page;

static const char* const page_names[PageCount] = {"PENDULUM", "SIGIL", "TALISMAN", "WAND"};

typedef enum {
    SwingIdle,
    SwingSearch, // the question is in: the bob wanders before it settles
    SwingAnswer,
    SwingFade,
} SwingState;

typedef enum {
    AnswerYes, // back and forth
    AnswerNo, // side to side
    AnswerAgain, // round and round
} Answer;

#define TRAIL   14
#define PEND_CX 98
#define PEND_CY 33
#define PEND_R  20

typedef struct {
    FuriMutex* mutex;
    FuriMessageQueue* queue;
    ViewPort* view_port;
    NotificationApp* notifications;
    bool running;
    Page page;
    uint32_t frame;
    bool sound;
    bool speaker_held;

    // the hour
    bool hour_known;
    PlanetaryHour hour;
    HoursTime now;
    uint32_t hour_checked;
    bool table_open;
    bool table_night;

    // pendulum
    SwingState swing;
    Answer answer;
    uint16_t swing_left;
    float phase;
    float amp;
    float ax, ay; // the pattern: how much of the swing goes each way
    float turn; // 0 = a line, 1 = a circle
    int8_t trail_x[TRAIL];
    int8_t trail_y[TRAIL];
    uint8_t trail_i;
    float last_s;

    // LED, persistent for the notification thread
    NotificationMessage led_msg[3];
    const NotificationMessage* led_seq[5];
    uint32_t led_last;
    uint8_t flash;
} App;

static uint32_t rnd(uint32_t n) {
    return furi_hal_random_get() % n;
}

// ---------------------------------------------------------------- sound

static void sound_click(App* app, float freq) {
    if(!app->sound || furi_hal_rtc_is_flag_set(FuriHalRtcFlagStealthMode)) return;
    if(!app->speaker_held) {
        if(!furi_hal_speaker_acquire(10)) return;
        app->speaker_held = true;
    }
    furi_hal_speaker_start(freq, 0.35f);
    furi_delay_ms(3);
    furi_hal_speaker_stop();
}

static void sound_release(App* app) {
    if(app->speaker_held) {
        furi_hal_speaker_stop();
        furi_hal_speaker_release();
        app->speaker_held = false;
    }
}

// ---------------------------------------------------------------- LED

static void led_init(App* app) {
    app->led_msg[0].type = NotificationMessageTypeLedRed;
    app->led_msg[1].type = NotificationMessageTypeLedGreen;
    app->led_msg[2].type = NotificationMessageTypeLedBlue;
    for(size_t i = 0; i < 3; i++)
        app->led_seq[i] = &app->led_msg[i];
    app->led_seq[3] = &message_do_not_reset;
    app->led_seq[4] = NULL;
    app->led_last = 0xFFFFFFFF;
}

static void led_update(App* app) {
    uint8_t r, g, b;
    if(app->flash) {
        r = g = b = 255;
    } else {
        // a slow violet breath, a little brighter while the pendulum works
        uint32_t t = app->frame / 4 % 20;
        uint32_t breath = t < 10 ? t : 20 - t;
        uint32_t level = 6 + breath * (app->swing == SwingIdle ? 3 : 8);
        r = 150 * level / 255;
        g = 0;
        b = 255 * level / 255;
    }
    uint32_t packed = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    if(packed == app->led_last) return;
    app->led_last = packed;
    app->led_msg[0].data.led.value = r;
    app->led_msg[1].data.led.value = g;
    app->led_msg[2].data.led.value = b;
    notification_message(app->notifications, (const NotificationSequence*)&app->led_seq);
}

// ---------------------------------------------------------------- the hour

static void hour_refresh(App* app) {
    DateTime dt;
    furi_hal_rtc_get_datetime(&dt);
    app->now.year = dt.year;
    app->now.month = dt.month;
    app->now.day = dt.day;
    app->now.minute = dt.hour * 60 + dt.minute;
    app->hour_known = hours_now(&place, &app->now, &app->hour);
    app->hour_checked = furi_get_tick();
}

static void clock_str(char* buf, size_t size, int16_t minute) {
    minute %= 1440;
    if(minute < 0) minute += 1440;
    snprintf(buf, size, "%02d:%02d", minute / 60, minute % 60);
}

// ---------------------------------------------------------------- pendulum

static void pendulum_ask(App* app) {
    if(app->swing == SwingSearch || app->swing == SwingAnswer) return;
    app->swing = SwingSearch;
    app->swing_left = 50 + rnd(30);
    uint32_t r = rnd(100);
    app->answer = r < 45 ? AnswerYes : r < 85 ? AnswerNo : AnswerAgain;
}

static void pendulum_step(App* app) {
    // where the swing is heading
    float tx, ty, tturn, tamp;
    switch(app->swing) {
    case SwingSearch:
        // the bob wanders: a new direction every half second
        if(app->swing_left % 10 == 0) {
            float a = rnd(628) / 100.0f;
            app->ax = cosf(a);
            app->ay = sinf(a);
            app->turn = rnd(100) / 100.0f;
        }
        tx = app->ax;
        ty = app->ay;
        tturn = app->turn;
        tamp = 14.0f;
        if(--app->swing_left == 0) {
            app->swing = SwingAnswer;
            app->swing_left = 140;
            app->flash = 2;
            notification_message(app->notifications, &sequence_single_vibro);
        }
        break;
    case SwingAnswer:
        tx = app->answer == AnswerNo ? 1.0f : 0.0f;
        ty = app->answer == AnswerYes ? 1.0f : 0.0f;
        tturn = app->answer == AnswerAgain ? 1.0f : 0.0f;
        tamp = 19.0f;
        if(--app->swing_left == 0) {
            app->swing = SwingFade;
            app->swing_left = 40;
        }
        break;
    case SwingFade:
        tx = app->ax;
        ty = app->ay;
        tturn = app->turn;
        tamp = 3.0f;
        if(--app->swing_left == 0) app->swing = SwingIdle;
        break;
    default:
        tx = 0.6f;
        ty = 0.8f;
        tturn = 0.3f;
        tamp = 3.0f;
        break;
    }
    // ease towards it
    app->ax += (tx - app->ax) * 0.12f;
    app->ay += (ty - app->ay) * 0.12f;
    app->turn += (tturn - app->turn) * 0.12f;
    app->amp += (tamp - app->amp) * 0.08f;

    app->phase += 0.26f;
    if(app->phase > 6.2832f) app->phase -= 6.2832f;
    float s = sinf(app->phase), c = cosf(app->phase);
    // a line along (ax, ay), bent into a circle by turn
    float x = app->amp * (app->ax * s - app->turn * app->ay * c);
    float y = app->amp * (app->ay * s + app->turn * app->ax * c);
    app->trail_i = (app->trail_i + 1) % TRAIL;
    app->trail_x[app->trail_i] = (int8_t)x;
    app->trail_y[app->trail_i] = (int8_t)y;

    // a tick at each end of the swing while it is working
    if(app->swing != SwingIdle && app->last_s * s <= 0.0f && app->amp > 8.0f) {
        sound_click(app, app->swing == SwingAnswer ? 2600.0f : 1900.0f);
    }
    app->last_s = c; // the extremes are where cos crosses zero
}

// ---------------------------------------------------------------- drawing

// The page name in a bar, arrows at the ends, a dot per page
static void draw_tabs(Canvas* canvas, App* app) {
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_box(canvas, 0, 0, W, 11);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_str_aligned(canvas, 3, 6, AlignLeft, AlignCenter, "<");
    canvas_draw_str_aligned(canvas, W - 3, 6, AlignRight, AlignCenter, ">");
    canvas_draw_str_aligned(canvas, W / 2, 6, AlignCenter, AlignCenter, page_names[app->page]);
    for(int i = 0; i < PageCount; i++) {
        int x = W / 2 + 36 + i * 5;
        if(i == (int)app->page) {
            canvas_draw_box(canvas, x - 1, 4, 3, 3);
        } else {
            canvas_draw_dot(canvas, x, 5);
        }
    }
    canvas_set_color(canvas, ColorBlack);
}

static void draw_hour_footer(Canvas* canvas, App* app) {
    canvas_set_font(canvas, FontSecondary);
    char buf[40];
    if(app->hour_known) {
        char until[8];
        clock_str(until, sizeof(until), app->hour.end);
        snprintf(
            buf,
            sizeof(buf),
            "%s  %s %u  to %s",
            planet_names[app->hour.planet],
            app->hour.is_day ? "day" : "night",
            app->hour.index,
            until);
    } else {
        strlcpy(buf, "no sunrise here today", sizeof(buf));
    }
    canvas_draw_line(canvas, 0, H - 10, W - 1, H - 10);
    canvas_draw_str_aligned(canvas, W / 2, H - 1, AlignCenter, AlignBottom, buf);
}

static void draw_pendulum(Canvas* canvas, App* app) {
    // the board, seen from above
    canvas_draw_circle(canvas, PEND_CX, PEND_CY, PEND_R);
    for(int d = -PEND_R + 3; d <= PEND_R - 3; d += 4) {
        canvas_draw_dot(canvas, PEND_CX + d, PEND_CY);
        canvas_draw_dot(canvas, PEND_CX, PEND_CY + d);
    }
    canvas_draw_str_aligned(canvas, PEND_CX, PEND_CY - PEND_R + 5, AlignCenter, AlignCenter, "Y");
    canvas_draw_str_aligned(canvas, PEND_CX, PEND_CY + PEND_R - 4, AlignCenter, AlignCenter, "Y");
    canvas_draw_str_aligned(canvas, PEND_CX - PEND_R + 4, PEND_CY, AlignCenter, AlignCenter, "N");
    canvas_draw_str_aligned(canvas, PEND_CX + PEND_R - 4, PEND_CY, AlignCenter, AlignCenter, "N");

    // the trail, oldest first, then the bob on its thread
    for(int k = TRAIL - 1; k > 0; k--) {
        int i = (app->trail_i + TRAIL - k) % TRAIL;
        int x = PEND_CX + app->trail_x[i], y = PEND_CY + app->trail_y[i];
        if(k < TRAIL / 2) {
            canvas_draw_dot(canvas, x, y);
        } else if(k % 2 == 0) {
            canvas_draw_dot(canvas, x, y);
        }
    }
    int bx = PEND_CX + app->trail_x[app->trail_i], by = PEND_CY + app->trail_y[app->trail_i];
    canvas_draw_line(canvas, PEND_CX, PEND_CY, bx, by);
    canvas_draw_disc(canvas, bx, by, 3);

    canvas_set_font(canvas, FontSecondary);
    switch(app->swing) {
    case SwingSearch:
        canvas_draw_str(canvas, 2, 24, "the bob");
        canvas_draw_str(canvas, 2, 34, "searches");
        break;
    case SwingAnswer: {
        const char* word = app->answer == AnswerYes ? "YES" :
                           app->answer == AnswerNo  ? "NO" :
                                                      "AGAIN";
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_rbox(canvas, 2, 21, 60, 16, 3);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_str_aligned(canvas, 32, 29, AlignCenter, AlignCenter, word);
        canvas_set_color(canvas, ColorBlack);
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(
            canvas,
            2,
            48,
            app->answer == AnswerYes ? "to and fro" :
            app->answer == AnswerNo  ? "side to side" :
                                       "round: ask again");
        break;
    }
    default:
        canvas_draw_str(canvas, 2, 24, "hold a question");
        canvas_draw_str(canvas, 2, 34, "in mind, then OK");
        break;
    }
}

static void draw_stub(Canvas* canvas, App* app) {
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "in the making");
    UNUSED(app);
}

static void draw_table(Canvas* canvas, App* app) {
    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);
    if(!app->hour_known) {
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "no sunrise here today");
        return;
    }
    static const char* const days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char* const short_names[PlanetCount] = {
        "SAT", "JUP", "MAR", "SUN", "VEN", "MER", "MOON"};
    char head[40], a[8], b[8];
    clock_str(a, sizeof(a), app->hour.sunrise);
    clock_str(b, sizeof(b), app->hour.sunset);
    snprintf(
        head,
        sizeof(head),
        "%s of %s  %s %s",
        app->table_night ? "Night" : "Day",
        planet_names[app->hour.day_ruler],
        app->table_night ? "set" : "rise",
        app->table_night ? b : a);
    canvas_draw_str(canvas, 2, 7, head);
    canvas_draw_str_aligned(canvas, W - 2, 7, AlignRight, AlignBottom, days[app->hour.weekday]);
    canvas_draw_line(canvas, 0, 9, W - 1, 9);

    int16_t from = app->table_night ? app->hour.sunset : app->hour.sunrise;
    int16_t to = app->table_night ? app->hour.next_sunrise : app->hour.sunset;
    float len = (to - from) / 12.0f;
    for(int n = 0; n < 12; n++) {
        int col = n / 6, row = n % 6;
        int x = 2 + col * 64, y = 18 + row * 9;
        char t[8], line[24];
        clock_str(t, sizeof(t), from + (int16_t)(n * len));
        Planet p = hours_planet_of(app->hour.day_ruler, n + (app->table_night ? 12 : 0));
        snprintf(line, sizeof(line), "%2d %s %s", n + 1, t, short_names[p]);
        bool current = app->hour.is_day != app->table_night && app->hour.index == n + 1;
        if(current) {
            canvas_draw_box(canvas, x - 2, y - 8, 62, 10);
            canvas_set_color(canvas, ColorWhite);
        }
        canvas_draw_str(canvas, x, y, line);
        canvas_set_color(canvas, ColorBlack);
    }
}

static void draw_callback(Canvas* canvas, void* context) {
    App* app = context;
    furi_mutex_acquire(app->mutex, FuriWaitForever);
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    if(app->table_open) {
        draw_table(canvas, app);
    } else {
        draw_tabs(canvas, app);
        switch(app->page) {
        case PagePendulum:
            draw_pendulum(canvas, app);
            break;
        default:
            draw_stub(canvas, app);
            break;
        }
        draw_hour_footer(canvas, app);
        if(app->flash) canvas_invert_color(canvas);
    }
    furi_mutex_release(app->mutex);
}

// ---------------------------------------------------------------- input

static void input_callback(InputEvent* event, void* context) {
    furi_message_queue_put(context, event, 0);
}

static void handle_input(App* app, const InputEvent* event) {
    if(event->type == InputTypeLong && event->key == InputKeyOk) {
        furi_mutex_acquire(app->mutex, FuriWaitForever);
        app->table_open = !app->table_open;
        app->table_night = app->hour_known && !app->hour.is_day;
        furi_mutex_release(app->mutex);
        return;
    }
    if(event->type != InputTypeShort) return;
    furi_mutex_acquire(app->mutex, FuriWaitForever);
    if(app->table_open) {
        if(event->key == InputKeyBack || event->key == InputKeyOk) {
            app->table_open = false;
        } else if(event->key == InputKeyUp || event->key == InputKeyDown) {
            app->table_night = !app->table_night;
        }
        furi_mutex_release(app->mutex);
        return;
    }
    switch(event->key) {
    case InputKeyBack:
        app->running = false;
        break;
    case InputKeyLeft:
        app->page = (Page)((app->page + PageCount - 1) % PageCount);
        break;
    case InputKeyRight:
        app->page = (Page)((app->page + 1) % PageCount);
        break;
    case InputKeyOk:
        if(app->page == PagePendulum) pendulum_ask(app);
        break;
    default:
        break;
    }
    furi_mutex_release(app->mutex);
}

// ---------------------------------------------------------------- main

static void tick(App* app) {
    furi_mutex_acquire(app->mutex, FuriWaitForever);
    if((int32_t)(furi_get_tick() - app->hour_checked) >= 1000) hour_refresh(app);
    if(app->page == PagePendulum) pendulum_step(app);
    app->frame++;
    if(app->flash) app->flash--;
    furi_mutex_release(app->mutex);
    led_update(app);
    view_port_update(app->view_port);
}

int32_t grimoire_app(void* p) {
    UNUSED(p);
    App* app = malloc(sizeof(App));
    app->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    app->queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    app->running = true;
    app->sound = true;
    app->ax = 0.6f;
    app->ay = 0.8f;
    app->amp = 3.0f;
    led_init(app);
    hour_refresh(app);

    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, draw_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app->queue);
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, app->view_port, GuiLayerFullscreen);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    InputEvent event;
    uint32_t next = furi_get_tick();
    while(app->running) {
        uint32_t now = furi_get_tick();
        uint32_t wait = (int32_t)(now - next) >= 0 ? 0 : next - now;
        if(furi_message_queue_get(app->queue, &event, wait) == FuriStatusOk) {
            handle_input(app, &event);
            continue;
        }
        next += TICK_MS;
        if((int32_t)(furi_get_tick() - next) > 200) next = furi_get_tick();
        tick(app);
    }

    sound_release(app);
    notification_message(app->notifications, &sequence_reset_rgb);
    furi_record_close(RECORD_NOTIFICATION);
    gui_remove_view_port(gui, app->view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(app->view_port);
    furi_message_queue_free(app->queue);
    furi_mutex_free(app->mutex);
    free(app);
    return 0;
}
