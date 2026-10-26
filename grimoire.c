// Grimoire: a pendulum, a sigil, a talisman and a wand for the Flipper Zero.
// This is the scaffold: a title page that names the four pages to come.

#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>

static const char* const pages[] = {"PENDULUM", "SIGIL", "TALISMAN", "WAND"};

typedef struct {
    FuriMessageQueue* queue;
    ViewPort* view_port;
    bool running;
} App;

static void draw_callback(Canvas* canvas, void* context) {
    UNUSED(context);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 14, AlignCenter, AlignCenter, "G R I M O I R E");
    canvas_set_font(canvas, FontSecondary);
    for(size_t i = 0; i < COUNT_OF(pages); i++) {
        canvas_draw_str_aligned(
            canvas, 32 + (i % 2) * 64, 32 + (i / 2) * 12, AlignCenter, AlignCenter, pages[i]);
    }
    canvas_draw_str_aligned(canvas, 64, 60, AlignCenter, AlignBottom, "in the making");
}

static void input_callback(InputEvent* event, void* context) {
    furi_message_queue_put(context, event, 0);
}

int32_t grimoire_app(void* p) {
    UNUSED(p);
    App* app = malloc(sizeof(App));
    app->queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    app->running = true;
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, draw_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app->queue);
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, app->view_port, GuiLayerFullscreen);

    InputEvent event;
    while(app->running) {
        if(furi_message_queue_get(app->queue, &event, 100) != FuriStatusOk) continue;
        if(event.type == InputTypeShort && event.key == InputKeyBack) app->running = false;
    }

    gui_remove_view_port(gui, app->view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(app->view_port);
    furi_message_queue_free(app->queue);
    free(app);
    return 0;
}
