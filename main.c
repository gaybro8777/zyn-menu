#include <gtk/gtk.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#include <osc_server.h>
#include <osc_client.h>

typedef enum
{
	UP,
	DOWN
} direction;

typedef struct
{
	GtkListStore *w_banks_store;
	GtkTreeView *w_banks_tree_view;
	GtkListStore *w_presets_store;
	GtkLabel *w_current_preset;
	GtkTreeView *w_presets_tree_view;
	GtkNotebook *w_notebook;
	GtkButton *w_record_button;

	guint return_button_held;
	int must_prompt_exit;

	guint arrow_held;
	guint begin_slide; //timeout before starting slide
	int must_slide_choices;
	direction slide_direction;

	int current_bank_id;
} app_widgets;

int message_box(const char *text, const char *caption)
{
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, text);

	gtk_window_set_title(GTK_WINDOW(dialog), caption);
	gint result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));

	switch (result)
	{
	case GTK_RESPONSE_CANCEL:
		return 0;
	case GTK_RESPONSE_OK:
		return 1;
	default:
		return 0;
	}
}

void close_application()
{
	printf("Quitting...\n");

	quit_zyn();

	sleep(4);

	gtk_main_quit();
}

void raise_volume()
{
	system("volume up");
}

void lower_volume()
{
	system("volume down");
}

void mute_volume()
{
	system("volume mute");
}

void move(direction dir, app_widgets *widgets)
{
	GtkTreeIter iter;
	GtkTreeSelection *tsel;
	GtkTreeModel *model;
	GtkTreeView *view;

	if (gtk_notebook_get_current_page(widgets->w_notebook) == 1)
	{
		view = widgets->w_presets_tree_view;
	}
	else
	{
		view = widgets->w_banks_tree_view;
	}

	model = gtk_tree_view_get_model(view);

	tsel = gtk_tree_view_get_selection(view);

	gtk_tree_selection_get_selected(tsel, &model, &iter);

	GtkTreePath *path = gtk_tree_model_get_path(model, &iter);

	if (dir == DOWN)
		gtk_tree_path_next(path);
	else
		gtk_tree_path_prev(path);

	gtk_tree_view_set_cursor(view, path, NULL, FALSE);
}

int slide(gpointer data)
{
	app_widgets *widgets = (app_widgets *)data;

	if (!widgets->must_slide_choices)
		return FALSE;

	move(widgets->slide_direction, widgets);

	return TRUE;
}

int begin_slide(gpointer data)
{
	app_widgets *widgets = (app_widgets *)data;

	widgets->must_slide_choices = TRUE;

	if (!widgets->arrow_held)
		widgets->arrow_held = g_timeout_add(120, slide, widgets);
	
	widgets->begin_slide = FALSE;

	return FALSE;
}

void down_press(GtkWidget *widget, GdkEvent *event, app_widgets *widgets)
{
	if (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS)
	{
		return;
	}

	move(DOWN, widgets);
	widgets->slide_direction = DOWN;

	widgets->begin_slide = g_timeout_add(200, begin_slide, widgets);
}

void down_release(GtkWidget *widget, GdkEvent *event, app_widgets *widgets)
{
	widgets->must_slide_choices = FALSE;

	if(widgets->arrow_held)
		g_source_remove(widgets->arrow_held);
	if(widgets->begin_slide)
		g_source_remove(widgets->begin_slide);

	widgets->arrow_held = FALSE;
}

void up_press(GtkWidget *widget, GdkEvent *event, app_widgets *widgets)
{
	if (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS)
	{
		return;
	}

	move(UP, widgets);

	widgets->slide_direction = UP;

	widgets->begin_slide = g_timeout_add(200, begin_slide, widgets);
}

void up_release(GtkWidget *widget, GdkEvent *event, app_widgets *widgets)
{
	widgets->must_slide_choices = FALSE;

	if(widgets->arrow_held)
		g_source_remove(widgets->arrow_held);
	if(widgets->begin_slide)
		g_source_remove(widgets->begin_slide);

	widgets->arrow_held = FALSE;
}

void record_clicked(GtkWidget *widget, app_widgets *widgets)
{
}

void enter_clicked(GtkWidget *widget, app_widgets *widgets)
{
	GtkTreeSelection *tsel = gtk_tree_view_get_selection(widgets->w_banks_tree_view);
	GtkTreeSelection *tsel_songs = gtk_tree_view_get_selection(widgets->w_presets_tree_view);

	GtkTreeModel *tm_songs;

	GtkTreeIter iter;
	GtkTreeModel *tm;
	GtkNotebook *nb = GTK_NOTEBOOK(gtk_widget_get_ancestor(GTK_WIDGET(widgets->w_banks_tree_view), GTK_TYPE_NOTEBOOK));

	if (gtk_notebook_get_current_page(nb) == 1)
	{
		if (gtk_tree_selection_get_selected(tsel_songs, &tm_songs, &iter))
		{
			gchararray preset_path;
			gchararray preset_name;

			tm = gtk_tree_view_get_model(widgets->w_presets_tree_view);

			gtk_tree_model_get(tm, &iter, 0, &preset_name, -1);
			gtk_tree_model_get(tm, &iter, 1, &preset_path, -1);

			gtk_label_set_text(widgets->w_current_preset, preset_name);
			load_preset(preset_path);
			
			return;
		}
	}

	int bank_id;

	if (gtk_tree_selection_get_selected(tsel, &tm, &iter))
	{
		gtk_tree_model_get(tm, &iter, 1, &bank_id, -1);
	}

	gtk_notebook_next_page(nb);

	if (widgets->current_bank_id != bank_id)
	{
		gtk_list_store_clear(widgets->w_presets_store);
		list_presets(bank_id);
	}

	widgets->current_bank_id = bank_id;
}

app_widgets *global_widgets;

int append_bank_main_thread(gpointer data)
{
	GtkListStore *liststore = global_widgets->w_banks_store;
	GtkTreeView *view = global_widgets->w_banks_tree_view;
	GtkTreeModel *model = gtk_tree_view_get_model(view);

	g_object_ref(model);

	GtkTreeIter iter;

	struct zyn_bank *bank = (struct zyn_bank *)data;

	gtk_list_store_append(liststore, &iter);
	gtk_list_store_set(liststore, &iter, 0, bank->name, -1);
	gtk_list_store_set(liststore, &iter, 1, bank->id, -1);

	gtk_tree_model_get_iter_first(model, &iter);
	GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
	gtk_tree_view_set_cursor(view, path, NULL, FALSE);

	g_object_unref(model);

	free(bank->name);
	free(bank->path);
	free(bank);

	return FALSE;
}

int append_preset_main_thread(gpointer data)
{
	GtkListStore *liststore = global_widgets->w_presets_store;
	GtkTreeView *view = global_widgets->w_presets_tree_view;
	GtkTreeModel *model = gtk_tree_view_get_model(view);

	g_object_ref(model);

	GtkTreeIter iter;

	struct zyn_preset *bank = (struct zyn_preset *)data;

	gtk_list_store_append(liststore, &iter);
	gtk_list_store_set(liststore, &iter, 0, bank->name, -1);
	gtk_list_store_set(liststore, &iter, 1, bank->path, -1);

	gtk_tree_model_get_iter_first(model, &iter);
	GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
	gtk_tree_view_set_cursor(view, path, NULL, FALSE);

	g_object_unref(model);

	free(bank->name);
	free(bank->path);
	free(bank);

	return FALSE;
}

void append_bank(struct zyn_bank bank)
{
	struct zyn_bank *b = malloc(sizeof(struct zyn_bank));

	size_t name_size = sizeof(char) * bank.name_length;
	size_t path_size = sizeof(char) * bank.path_length;

	b->id = bank.id;
	b->name = (char *)malloc(name_size);
	b->path = (char *)malloc(path_size);

	strncpy(b->name, bank.name, name_size);
	strncpy(b->path, bank.path, path_size);

	g_idle_add(append_bank_main_thread, b);
	g_main_context_wakeup(NULL);
}

void append_preset(struct zyn_preset preset)
{
	struct zyn_preset *b = malloc(sizeof(struct zyn_preset));

	size_t name_size = sizeof(char) * preset.name_length;
	size_t path_size = sizeof(char) * preset.path_length;

	b->id = preset.id;
	b->name = (char *)malloc(name_size);
	b->path = (char *)malloc(path_size);

	strncpy(b->name, preset.name, name_size);
	strncpy(b->path, preset.path, path_size);

	g_idle_add(append_preset_main_thread, b);
	g_main_context_wakeup(NULL);
}

int main(int argc, char *argv[])
{
	GtkBuilder *builder;
	GtkWidget *window;

	app_widgets *widgets = g_slice_new(app_widgets);

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "interface.glade", NULL);

	window = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));

	widgets->w_banks_store = GTK_LIST_STORE(
		gtk_builder_get_object(builder, "banks_store"));
	widgets->w_banks_tree_view = GTK_TREE_VIEW(
		gtk_builder_get_object(builder, "banks_tree_view"));
	widgets->w_presets_store = GTK_LIST_STORE(
		gtk_builder_get_object(builder, "presets_store"));
	widgets->w_presets_tree_view = GTK_TREE_VIEW(
		gtk_builder_get_object(builder, "presets_tree_view"));
	widgets->w_notebook = GTK_NOTEBOOK(
		gtk_builder_get_object(builder, "notebook"));
	widgets->w_current_preset = GTK_LABEL(
		gtk_builder_get_object(builder, "current_preset"));
	widgets->w_record_button = GTK_BUTTON(
		gtk_builder_get_object(builder, "record"));

	widgets->arrow_held = FALSE;
	widgets->return_button_held = FALSE;
	widgets->must_prompt_exit = FALSE;
	widgets->must_slide_choices = FALSE;

	widgets->current_bank_id = 0;

	global_widgets = widgets;

	system("./zyn.sh");
	sleep(4);

	gtk_builder_connect_signals(builder, widgets);

	g_object_unref(builder);

	run_osc_server();
	rescan_presets();

	gtk_widget_show(window);

	gtk_main();

	return 0;
}

int prompt_exit(gpointer data)
{
	app_widgets *widgets = (app_widgets *)data;

	if (!widgets->must_prompt_exit)
		return FALSE;

	widgets->must_prompt_exit = FALSE;

	if (message_box("Do you want to close zyn-menu?", "Exit") > 0)
	{
		close_application();
	}

	return FALSE;
}

void return_button_press(GtkWidget *widget, GdkEvent *event, app_widgets *widgets)
{
	widgets->must_prompt_exit = TRUE;

	widgets->return_button_held = g_timeout_add(800, prompt_exit, widgets);
}

void return_button_release(GtkWidget *widget, GdkEvent *event, app_widgets *widgets)
{
	widgets->must_prompt_exit = FALSE;

	g_source_remove(widgets->return_button_held);

	GtkNotebook *nb = GTK_NOTEBOOK(gtk_widget_get_ancestor(GTK_WIDGET(widgets->w_banks_tree_view), GTK_TYPE_NOTEBOOK));

	if (gtk_notebook_get_current_page(nb) == 0)
	{
		gtk_notebook_next_page(nb);
	}
	else
	{
		gtk_notebook_prev_page(nb);
	}
}

void on_window_main_destroy()
{
	close_application();
}
