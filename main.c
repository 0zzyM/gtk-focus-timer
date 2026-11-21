#include "glib.h"
#include "glibconfig.h"
#include <gtk/gtk.h>
#include <stdint.h>
#include <stdio.h>

const uint32_t POMODORO_TIME = 25 * 60, BREAK_TIME = 5 * 60;
gint64 ms, target, remaining_total_s, remaining_m, remaining_s, sourceID;
gboolean isFirstClick = 1;

static gboolean calculate_remaining(gpointer data) {

    ms = g_get_monotonic_time();

    remaining_total_s = (target - ms + 999999) / 1000000;
    remaining_m = remaining_total_s / 60;
    remaining_s = remaining_total_s % 60;

    gchar *lbl = g_strdup_printf("%ld:%ld", remaining_m, remaining_s);
    gtk_label_set_text(data, lbl);
    if (remaining_total_s <= 0)
        return 0;

    return 1;
}

static void start_timer(GtkToggleButton *start_button, gpointer data) {

    if (isFirstClick) {
        target = g_get_monotonic_time() + (POMODORO_TIME * 1000000);
        calculate_remaining(data);
        isFirstClick = 0;
    }
    if (gtk_toggle_button_get_active(start_button)) {
        target = g_get_monotonic_time() + remaining_total_s * 1000000;
        gtk_button_set_label(GTK_BUTTON(start_button), "⏸");
        sourceID =
            g_timeout_add(1000, G_SOURCE_FUNC(calculate_remaining), data);
    } else {
        gtk_button_set_label(GTK_BUTTON(start_button), "►");
        g_source_remove(sourceID);
    }
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *start_button;
    GtkWidget *pause_button;
    GtkWidget *reset_button;
    GtkWidget *grid;
    GtkCssProvider *css = gtk_css_provider_new();
    GtkWidget *timer_label;

    gtk_css_provider_load_from_path(css, "style.css");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(), GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref(css);

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Pomodoro");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);

    grid = gtk_grid_new();

    timer_label = gtk_label_new("25:00");
    gtk_widget_add_css_class(GTK_WIDGET(timer_label), "timer-label");

    gtk_window_set_child(GTK_WINDOW(window), grid);
    start_button = gtk_toggle_button_new_with_label("►");
    pause_button = gtk_button_new_with_label("Pause");
    reset_button = gtk_button_new_with_label("Reset");

    gtk_widget_add_css_class(GTK_WIDGET(start_button), "start-btn");

    g_signal_connect(start_button, "toggled", G_CALLBACK(start_timer),
                     timer_label);
    gtk_grid_attach(GTK_GRID(grid), start_button, 9, 32, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), pause_button, 10, 32, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), reset_button, 11, 32, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), timer_label, 0, 0, 30, 30);

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
