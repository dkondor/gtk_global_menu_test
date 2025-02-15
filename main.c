/*
 * main.c -- test program for global menus
 * 
 * Copyright 2025 Daniel Kondor <kondor.dani@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * 
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 * 
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#include <stdio.h>
#include <stdlib.h>
#include <wayland-client.h>
#include <foreign_toplevel.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libdbusmenu-gtk/menu.h>

GtkWidget *menu_btn = NULL;
GtkWidget *app_id_lbl = NULL;
GDBusConnection *bus = NULL;
GtkMenu *dbus_menu = NULL;

static void tl_cb(void*, struct toplevel_manager* gr) {
	const struct toplevel_properties *props = toplevel_manager_get_active_app(gr);
	const char* app_id = props->app_id;
	printf("Activated app: %s\n", app_id ? app_id : "(null)");
	
	gtk_label_set_label(GTK_LABEL(app_id_lbl), app_id);
	
	gtk_widget_insert_action_group(menu_btn, "app", NULL);
	gtk_widget_insert_action_group(menu_btn, "win", NULL);
	gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_btn), NULL);
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(menu_btn), NULL);
	if (dbus_menu) {
		g_object_unref(dbus_menu);
		dbus_menu = NULL;
	}
	
	if(props->menubar_path && props->menubar_bus_name &&
		((props->window_object_path && props->window_bus_name) ||
		 (props->application_object_path && props->application_bus_name))) {
		// try using the GTK menu implementation
		GDBusMenuModel *model = g_dbus_menu_model_get(bus, props->menubar_bus_name, props->menubar_path);
		if(model) {
			gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_btn), G_MENU_MODEL(model));
			g_object_unref(model);
			
			if(props->application_object_path) {
				GDBusActionGroup *grp = g_dbus_action_group_get(bus, props->application_bus_name, props->application_object_path);
				if(grp) {
					gtk_widget_insert_action_group(menu_btn, "app", G_ACTION_GROUP(grp));
					g_object_unref(grp);
				}
				else fprintf(stderr, "Error retrieving app action group!\n");
			}
			if(props->window_object_path) {
				GDBusActionGroup *grp = g_dbus_action_group_get(bus, props->window_bus_name, props->window_object_path);
				if(grp) {
					gtk_widget_insert_action_group(menu_btn, "win", G_ACTION_GROUP(grp));
					g_object_unref(grp);
				}
				else fprintf(stderr, "Error retrieving window action group!\n");
			}
		}
		else fprintf(stderr, "Error retrieving menu model!\n");
	}
	else if(props->kde_service_name && props->kde_object_path) {
		// alternatively use the KDE / com.canonical.dbusmenu implementation
		DbusmenuGtkMenu *menu = dbusmenu_gtkmenu_new(props->kde_service_name, props->kde_object_path);
		if(menu) {
			g_object_ref_sink(menu);
			gtk_menu_button_set_popup(GTK_MENU_BUTTON(menu_btn), GTK_WIDGET(menu));
			gtk_widget_show_all(GTK_WIDGET(menu));
			dbus_menu = GTK_MENU(menu);
		}
	}
}

static void clicked_cb(GtkButton* btn, gpointer) {
	/* show the menu when the button is clicked -- no idea why this is necessary */
	if(dbus_menu) gtk_menu_popup_at_widget(dbus_menu, GTK_WIDGET(btn), GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
}


#define SELF_NAME "gtk_global_menu_test"

int main() {
	gtk_init(NULL, NULL);
	
	struct toplevel_manager* gr = toplevel_manager_new();
	if(!gr) {
		fprintf(stderr, "Cannot create grabber interface!\n");
		return 1;
	}
	
	bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	if(!bus)
	{
		fprintf(stderr, "Cannot connect to DBus!\n");
		toplevel_manager_free(gr);
		return 1;
	}
	
	g_set_prgname(SELF_NAME);
	toplevel_manager_set_self(gr, SELF_NAME);
	
	GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(win), "Gtk global menu test");
	gtk_widget_set_size_request(win, 400, 300);
	
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	GtkWidget *lbl = gtk_label_new("Active app: ");
	app_id_lbl = gtk_label_new("");
	gtk_container_add(GTK_CONTAINER(hbox), lbl);
	gtk_container_add(GTK_CONTAINER(hbox), app_id_lbl);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	
	menu_btn = gtk_menu_button_new();
	gtk_button_set_label(GTK_BUTTON(menu_btn), "Show menu");
	g_signal_connect(G_OBJECT(menu_btn), "clicked", G_CALLBACK(clicked_cb), NULL);
	gtk_container_add(GTK_CONTAINER(vbox), menu_btn);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	
	g_signal_connect(G_OBJECT(win), "destroy", gtk_main_quit, NULL);
	
	gtk_widget_show_all(win);
	
	toplevel_manager_set_callback(gr, tl_cb, NULL);
	gtk_main();
	
	toplevel_manager_free(gr);
	
	return 0;
}

