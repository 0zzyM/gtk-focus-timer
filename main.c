#include "glib.h"
#include "glibconfig.h"
#include <gtk/gtk.h>
#include <stdint.h>
#include <stdio.h>
const uint16_t FOCUS_M = 25, BREAK_M = 5;
const uint32_t FOCUS_TIME = FOCUS_M * 60, BREAK_TIME = BREAK_M * 60;
gint64 ms, target, remaining_total_s, remaining_m, remaining_s, sourceID = 0;
gboolean isFirstClick = 1, isFocusMode = 1;

typedef struct {
    GtkWidget *timerLabel;
    GtkWidget *playButton;
} userData;

static void switch_to_focus_mode(GtkButton *focus_button, gpointer user_data) {
    userData *uD = user_data;
    isFirstClick = 1;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(uD->playButton), 0);
    if (isFocusMode != 1) {
        isFocusMode = 1;

        gchar *new_label = g_strdup_printf("%02d:00", FOCUS_TIME / 60);
        gtk_label_set_text(GTK_LABEL(uD->timerLabel), new_label);
        if (sourceID != 0) {
            g_source_remove(sourceID);
            sourceID = 0;
            gtk_button_set_label(GTK_BUTTON(uD->playButton), "►");
        }
    }
}

static void switch_to_break_mode(GtkButton *break_button, gpointer user_data) {
    userData *uD = user_data;
    isFirstClick = 1;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(uD->playButton), 0);
    if (isFocusMode != 0) {
        isFocusMode = 0;
        gchar *new_label = g_strdup_printf(
            "%02d:00", (isFocusMode ? FOCUS_TIME / 60 : BREAK_TIME / 60));
        gtk_label_set_text(GTK_LABEL(uD->timerLabel), new_label);
        if (sourceID != 0) {
            g_source_remove(sourceID);
            sourceID = 0;
            gtk_button_set_label(GTK_BUTTON(uD->playButton), "►");
        }
    }
}

static void reset_timer(GtkButton *reset_button, gpointer user_data) {
    isFirstClick = 1;
    if (sourceID != 0) {
        g_source_remove(sourceID);
        sourceID = 0;
    }
    userData *uD = user_data;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(uD->playButton), 0);
    gtk_button_set_label(GTK_BUTTON(uD->playButton), "►");

    gchar *new_label = g_strdup_printf(
        "%d:00", (isFocusMode ? FOCUS_TIME / 60 : BREAK_TIME / 60));

    gtk_label_set_text(GTK_LABEL(uD->timerLabel), new_label);
}

static gboolean calculate_remaining(gpointer data) {

    ms = g_get_monotonic_time();

    remaining_total_s = (target - ms + 999999) / 1000000;
    remaining_m = remaining_total_s / 60;
    remaining_s = remaining_total_s % 60;

    gchar *lbl = g_strdup_printf("%02ld:%02ld", remaining_m, remaining_s);
    gtk_label_set_text(data, lbl);
    if (remaining_total_s <= 0)
        return 0;

    return 1;
}

static void play_timer(GtkToggleButton *play_button, gpointer data) {

    if (isFirstClick) {
        target = g_get_monotonic_time() +
                 ((isFocusMode ? FOCUS_TIME : BREAK_TIME) * 1000000);
        calculate_remaining(data);
        isFirstClick = 0;
    }
    if (gtk_toggle_button_get_active(play_button)) {
        target = g_get_monotonic_time() + remaining_total_s * 1000000;
        gtk_button_set_label(GTK_BUTTON(play_button), "⏸");
        sourceID =
            g_timeout_add(1000, G_SOURCE_FUNC(calculate_remaining), data);
    } else {
        gtk_button_set_label(GTK_BUTTON(play_button), "►");
        if (sourceID != 0) {
            g_source_remove(sourceID);
            sourceID = 0;
        }
    }
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window, *play_button, *settings_button, *reset_button, *grid,
        *timer_label, *focus_button, *toggle_mode_button, *break_button;
    GtkCssProvider *css = gtk_css_provider_new();

    gtk_css_provider_load_from_path(css, "style.css");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(), GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Focus Timer");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
    grid = gtk_grid_new();
    gtk_window_set_child(GTK_WINDOW(window), grid);

    timer_label = gtk_label_new("25:00");
    play_button = gtk_toggle_button_new_with_label("►");
    settings_button = gtk_button_new_with_label("⚙");
    reset_button = gtk_button_new_with_label("↻");
    focus_button = gtk_button_new_with_label(" Focus");
    break_button = gtk_button_new_with_label("Break");

    gtk_widget_add_css_class(GTK_WIDGET(timer_label), "timer-label");
    gtk_widget_add_css_class(GTK_WIDGET(play_button), "action-btn");
    gtk_widget_add_css_class(GTK_WIDGET(settings_button), "action-btn");
    gtk_widget_add_css_class(GTK_WIDGET(reset_button), "action-btn");
    gtk_widget_add_css_class(GTK_WIDGET(focus_button), "action-btn");
    gtk_widget_add_css_class(GTK_WIDGET(break_button), "action-btn");

    gtk_widget_set_cursor_from_name(play_button, "pointer");
    gtk_widget_set_cursor_from_name(settings_button, "pointer");
    gtk_widget_set_cursor_from_name(reset_button, "pointer");
    gtk_widget_set_cursor_from_name(focus_button, "pointer");
    gtk_widget_set_cursor_from_name(break_button, "pointer");

    static userData ud;
    ud.timerLabel = timer_label;
    ud.playButton = play_button;

    g_signal_connect(play_button, "toggled", G_CALLBACK(play_timer),
                     timer_label);
    g_signal_connect(reset_button, "clicked", G_CALLBACK(reset_timer), &ud);
    g_signal_connect(focus_button, "clicked", G_CALLBACK(switch_to_focus_mode),
                     &ud);
    g_signal_connect(break_button, "clicked", G_CALLBACK(switch_to_break_mode),
                     &ud);

    gtk_grid_attach(GTK_GRID(grid), settings_button, 22, 0, 2, 2);
    gtk_grid_attach(GTK_GRID(grid), timer_label, 0, 2, 24, 24);
    gtk_grid_attach(GTK_GRID(grid), play_button, 11, 26, 2, 2);
    gtk_grid_attach(GTK_GRID(grid), reset_button, 0, 0, 2, 2);
    gtk_grid_attach(GTK_GRID(grid), focus_button, 8, 26, 2, 2);
    gtk_grid_attach(GTK_GRID(grid), break_button, 14, 26, 2, 2);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app =
        gtk_application_new("org.gnome.Pomodoro", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
