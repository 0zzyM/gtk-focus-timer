#include "glib.h"
#include "glibconfig.h"
#include <gtk/gtk.h>
#include <stdint.h>
#include <stdio.h>

gint64 ms, target, remaining_s;

static gboolean calculate_remaining(gpointer data) {

    ms = g_get_monotonic_time();

    remaining_s = (target - ms + 999999) / 1000000;

    gchar *lbl = g_strdup_printf("%ld", remaining_s);
    gtk_label_set_text(data, lbl);
    if (remaining_s <= 0)
        return 0;

    return 1;
}

static void start_timer(GtkWidget *start_button, gpointer data) {

    target = g_get_monotonic_time() + 15 * 1000000;
    calculate_remaining(data);
    g_timeout_add(1000, G_SOURCE_FUNC(calculate_remaining), data);
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *start_button;
    GtkWidget *stop_button;
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

    timer_label = gtk_label_new("");
    gtk_widget_add_css_class(GTK_WIDGET(timer_label), "timer-label");

    gtk_window_set_child(GTK_WINDOW(window), grid);
    start_button = gtk_button_new_with_label("Start");
    stop_button = gtk_button_new_with_label("Stop");
    reset_button = gtk_button_new_with_label("Reset");

    g_signal_connect(start_button, "clicked", G_CALLBACK(start_timer),
                     timer_label);
    gtk_grid_attach(GTK_GRID(grid), start_button, 9, 25, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), stop_button, 10, 25, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), reset_button, 11, 25, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), timer_label, 0, 0, 20, 20);

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
