#include "glib.h"
#include "glibconfig.h"
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
gint64 ms, target, remaining_total_s, remaining_m, remaining_s, sourceID = 0;
gboolean isFirstClick = 1, isFocusMode = 1, isSettingsOpen = 0;

typedef struct {
    GtkWidget *timerLabel;
    GtkWidget *playButton;
    GtkWidget *resetButton;
    GtkWidget *settingsGrid;
    GtkWidget *grid;
    GtkWidget *focusSpinButton;
    GtkWidget *breakSpinButton;
    gint64 FOCUS_M;
    gint64 BREAK_M;
    GtkWidget *overlay;
    GtkMediaStream *alarm;
    GKeyFile *configFile;
} userData;
static void reset_timer(gpointer user_data) {
    isFirstClick = 1;
    if (sourceID != 0) {
        g_source_remove(sourceID);
        sourceID = 0;
    }
    userData *uD = user_data;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(uD->playButton), 0);
    gtk_button_set_label(GTK_BUTTON(uD->playButton), "►");

    gchar *new_label =
        g_strdup_printf("%02ld:00", (isFocusMode ? uD->FOCUS_M : uD->BREAK_M));

    gtk_label_set_text(GTK_LABEL(uD->timerLabel), new_label);
}


static void set_focus_time(gpointer user_data) {
    userData *uD = user_data;
    gint64 new_focus_m =
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(uD->focusSpinButton));

    uD->FOCUS_M = new_focus_m;
    g_key_file_set_integer(uD->configFile, "USER SETTINGS", "FOCUS_M",
                           new_focus_m);
}

static void set_break_time(gpointer user_data) {
    userData *uD = user_data;
    gint64 new_break_m =
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(uD->breakSpinButton));
    uD->BREAK_M = new_break_m;
    g_key_file_set_integer(uD->configFile, "USER SETTINGS", "BREAK_M",
                           new_break_m);

}

static void apply_settings(GtkButton *apply_settings_button,
                           gpointer user_data) {

    userData *uD = user_data;
    gtk_overlay_set_clip_overlay(GTK_OVERLAY(uD->overlay),
                                 GTK_WIDGET(uD->settingsGrid), true);
    set_focus_time(user_data);
    set_break_time(user_data);
    g_key_file_save_to_file(uD->configFile, "./config.ini", NULL);
    if (sourceID == 0)
        reset_timer(user_data);
}
static void close_settings(GtkButton *close_settings_button,
                           gpointer user_data) {
    userData *uD = user_data;
    gtk_widget_set_visible(uD->settingsGrid, false);
    gtk_widget_set_sensitive(GTK_WIDGET(uD->grid), true);
}

static void open_settings(GtkButton *settings_button, gpointer user_data) {
    userData *uD = user_data;
    gtk_widget_set_visible(uD->settingsGrid, true);
    gtk_widget_set_sensitive(GTK_WIDGET(uD->grid), false);
}

static void switch_to_focus_mode(GtkButton *focus_button, gpointer user_data) {
    if (isFocusMode != 1)
        isFocusMode = 1;
    reset_timer(user_data);
}

static void switch_to_break_mode(GtkButton *break_button, gpointer user_data) {
    if (isFocusMode != 0)
        isFocusMode = 0;
    reset_timer(user_data);
}

static void reset_onClick(GtkButton *reset_button, gpointer user_data) {
    reset_timer(user_data);
}

static gboolean calculate_remaining(gpointer data) {

    userData *uD = data;

    ms = g_get_monotonic_time();

    remaining_total_s = (target - ms + 999999) / 1000000;
    remaining_m = remaining_total_s / 60;
    remaining_s = remaining_total_s % 60;

    gchar *lbl = g_strdup_printf("%02ld:%02ld", remaining_m, remaining_s);
    gtk_label_set_text(GTK_LABEL(uD->timerLabel), lbl);
    if (remaining_total_s <= 0) {
        gtk_media_stream_set_playing(GTK_MEDIA_STREAM(uD->alarm), 1);
        gtk_button_set_label(GTK_BUTTON(uD->playButton), "►");
        return 0;
    }
    return 1;
}

static void play_timer(GtkToggleButton *play_button, gpointer data) {

    userData *uD = data;

    if (isFirstClick) {
        target =
            g_get_monotonic_time() +
            ((isFocusMode ? uD->FOCUS_M * 60 : uD->BREAK_M * 60) * 1000000);
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
        *settings_grid, *timer_label, *focus_button, *toggle_mode_button,
        *break_button, *overlay, *focus_spin_button, *break_spin_button,
        *focus_setting_text, *break_setting_text, *close_settings_button,
        *apply_settings_button, *settings_header_bar, *settings_header_layout,
        *settings_title_label;
    GtkCssProvider *css = gtk_css_provider_new();
    GtkMediaStream *alarm;
    GKeyFile *config_file;

    gtk_css_provider_load_from_path(css, "style.css");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(), GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Focus Timer");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
    grid = gtk_grid_new();
    settings_grid = gtk_grid_new();

    overlay = gtk_overlay_new();
    gtk_window_set_child(GTK_WINDOW(window), overlay);
    gtk_overlay_set_child(GTK_OVERLAY(overlay), grid);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), settings_grid);
    gtk_widget_set_visible(settings_grid, false);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(grid, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(settings_grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(settings_grid, GTK_ALIGN_CENTER);


    config_file = g_key_file_new();
    g_key_file_load_from_file(config_file, "./config.ini", G_KEY_FILE_NONE, NULL);


    gint64 FOCUS_M = g_key_file_get_integer(config_file, "USER SETTINGS", "FOCUS_M", NULL);
    gint64 BREAK_M = g_key_file_get_integer(config_file, "USER SETTINGS", "BREAK_M", NULL);



    settings_button = gtk_button_new_with_label("⚙");
    gchar *timer_str = g_strdup_printf("%02ld:00", FOCUS_M);
    timer_label = gtk_label_new(timer_str );
    play_button = gtk_toggle_button_new_with_label("►");
    reset_button = gtk_button_new_with_label("↻");
    focus_button = gtk_button_new_with_label(" Focus");
    break_button = gtk_button_new_with_label("Break");

    alarm = gtk_media_file_new();
    gtk_media_file_set_filename(GTK_MEDIA_FILE(alarm), "./alarm.wav");

    settings_header_bar = gtk_header_bar_new();
    settings_title_label = gtk_label_new("Settings");
    gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(settings_header_bar),
                                          0);
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(settings_header_bar),
                                    settings_title_label);
    focus_spin_button = gtk_spin_button_new_with_range(1, 60, 1);
    break_spin_button = gtk_spin_button_new_with_range(1, 60, 1);
    focus_setting_text = gtk_label_new("Focus time:");
    break_setting_text = gtk_label_new("Break time:");
    apply_settings_button = gtk_button_new_with_label("Apply Settings");
    close_settings_button = gtk_button_new_with_label("X");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(settings_header_bar),
                            close_settings_button);

    gtk_widget_add_css_class(GTK_WIDGET(timer_label), "timer-label");
    gtk_widget_add_css_class(GTK_WIDGET(play_button), "play-btn");
    gtk_widget_add_css_class(GTK_WIDGET(settings_button), "action-btn");
    gtk_widget_add_css_class(GTK_WIDGET(reset_button), "action-btn");
    gtk_widget_add_css_class(GTK_WIDGET(focus_button), "action-btn");
    gtk_widget_add_css_class(GTK_WIDGET(break_button), "action-btn");
    gtk_widget_add_css_class(GTK_WIDGET(grid), "grid");
    gtk_widget_add_css_class(GTK_WIDGET(settings_grid), "settings-grid");

    gtk_widget_set_cursor_from_name(play_button, "pointer");
    gtk_widget_set_cursor_from_name(settings_button, "pointer");
    gtk_widget_set_cursor_from_name(reset_button, "pointer");
    gtk_widget_set_cursor_from_name(focus_button, "pointer");
    gtk_widget_set_cursor_from_name(break_button, "pointer");

    gtk_grid_attach(GTK_GRID(grid), settings_button, 22, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), timer_label, 0, 1, 24, 24);
    gtk_grid_attach(GTK_GRID(grid), play_button, 11, 26, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), reset_button, 0, 0, 2, 2);
    gtk_grid_attach(GTK_GRID(grid), focus_button, 8, 26, 2, 2);
    gtk_grid_attach(GTK_GRID(grid), break_button, 14, 26, 2, 2);

    // gtk_grid_attach(GTK_GRID(settings_grid), close_settings_button, 0, 0, 1,
    // 2);
    gtk_grid_attach(GTK_GRID(settings_grid), settings_header_bar, 0, 0, 20, 2);
    gtk_grid_attach(GTK_GRID(settings_grid), focus_setting_text, 5, 5, 5, 5);
    gtk_grid_attach(GTK_GRID(settings_grid), focus_spin_button, 12, 5, 2, 2);
    gtk_grid_attach(GTK_GRID(settings_grid), break_setting_text, 5, 10, 5, 5);
    gtk_grid_attach(GTK_GRID(settings_grid), break_spin_button, 12, 10, 2, 2);
    gtk_grid_attach(GTK_GRID(settings_grid), apply_settings_button, 12, 15, 5,
                    5);

    static userData ud;
    ud.timerLabel = timer_label;
    ud.playButton = play_button;
    ud.resetButton = reset_button;
    ud.settingsGrid = settings_grid;
    ud.grid = grid;
    ud.focusSpinButton = focus_spin_button;
    ud.breakSpinButton = break_spin_button;
    ud.configFile = config_file;
    ud.FOCUS_M = g_key_file_get_integer(config_file, "USER SETTINGS", "FOCUS_M", NULL);
    ud.BREAK_M = g_key_file_get_integer(config_file, "USER SETTINGS", "BREAK_M", NULL);
    ud.overlay = overlay;
    ud.alarm = alarm;

    g_signal_connect(play_button, "toggled", G_CALLBACK(play_timer), &ud);
    g_signal_connect(reset_button, "clicked", G_CALLBACK(reset_onClick), &ud);
    g_signal_connect(focus_button, "clicked", G_CALLBACK(switch_to_focus_mode),
                     &ud);
    g_signal_connect(break_button, "clicked", G_CALLBACK(switch_to_break_mode),
                     &ud);
    g_signal_connect(settings_button, "clicked", G_CALLBACK(open_settings),
                     &ud);
    g_signal_connect(close_settings_button, "clicked",
                     G_CALLBACK(close_settings), &ud);
    g_signal_connect(apply_settings_button, "clicked",
                     G_CALLBACK(apply_settings), &ud);

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
