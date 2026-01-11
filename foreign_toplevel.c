/*
 * foreign_toplevel.c -- interface to the wlr-foreign-toplevel protocol 
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


// needed for strdup
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"
#include <foreign_toplevel.h>
#include <wayland-client.h>
#include <gdk/gdk.h>
#include <gdk/gdkwayland.h>


typedef struct zwlr_foreign_toplevel_handle_v1 wfthandle;

/* struct to hold information for one instance of the manager */
struct toplevel_manager {
	struct zwlr_foreign_toplevel_manager_v1* manager;
	struct wl_list toplevels;
	void (*callback)(void* data, struct toplevel_manager* gr);
	void* data;
	struct toplevel* active;
	int init_done;
	char* self;
};


/* struct to hold information about one toplevel */
struct toplevel {
	wfthandle* handle;
	wfthandle* parent;
	struct toplevel_manager* gr;
	int init_done;
	struct wl_list link;
	
	struct toplevel_properties props;
};


#ifndef G_GNUC_UNUSED
#define G_GNUC_UNUSED __attribute__((unused))
#endif

/* callbacks */

static void title_cb(G_GNUC_UNUSED void* data, G_GNUC_UNUSED wfthandle* handle,
		G_GNUC_UNUSED const char* title) {
	/* don't care */
}

static void appid_cb(void* data, G_GNUC_UNUSED wfthandle* handle, const char* app_id) {
	if(!(app_id && data)) return;
	struct toplevel* tl = (struct toplevel*)data;
	free(tl->props.app_id);
	tl->props.app_id = strdup(app_id);
}

static void output_enter_cb(G_GNUC_UNUSED void* data, G_GNUC_UNUSED wfthandle* handle,
		G_GNUC_UNUSED struct wl_output* output) {
	/* don't care */
}
static void output_leave_cb(G_GNUC_UNUSED void* data, G_GNUC_UNUSED wfthandle* handle,
		G_GNUC_UNUSED struct wl_output* output) {
	/* don't care */
}

static void state_cb(void* data, G_GNUC_UNUSED wfthandle* handle, struct wl_array* state) {
	if(!(data && state)) return;
	struct toplevel* tl = (struct toplevel*)data;
	struct toplevel_manager* gr = tl->gr;
	if(!gr) return;
	int activated = 0;
	int i;
	uint32_t* stdata = (uint32_t*)state->data;
	for(i = 0; i*sizeof(uint32_t) < state->size; i++) {
		if(stdata[i] == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED) {
            activated = 1;
            break;
		}
	}
	if(activated) {
		int orig_init_done = tl->init_done;
		struct toplevel* new_active = NULL;
		while(tl) {
			new_active = tl;
			if(!tl->parent) break;
			tl = zwlr_foreign_toplevel_handle_v1_get_user_data(tl->parent);
		}
		if(new_active != gr->active) {
			if(!gr->self || strcmp(gr->self, new_active->props.app_id)) {
				gr->active = new_active;
				if(orig_init_done && gr->callback) gr->callback(gr->data, gr);
			}
		}
		
		/*
		fprintf(stdout, "Activated app with the following DBus properties:\n");
		fprintf(stdout, "application_id: %s\n",          new_active->gtk_application_id);
		fprintf(stdout, "app_menu_path: %s\n",           new_active->app_menu_path);
		fprintf(stdout, "menubar_path: %s\n",            new_active->menubar_path);
		fprintf(stdout, "window_object_path: %s\n",      new_active->window_object_path);
		fprintf(stdout, "application_object_path: %s\n", new_active->application_object_path);
		fprintf(stdout, "unique_bus_name: %s\n",         new_active->unique_bus_name);
		
		fprintf(stdout, "kde_service_name: %s\n",        new_active->kde_service_name);
		fprintf(stdout, "kde_object_path: %s\n",         new_active->kde_object_path); */
	}
}

static void done_cb(void* data, G_GNUC_UNUSED wfthandle* handle) {
	if(!data) return;
	struct toplevel* tl = (struct toplevel*)data;
	tl->init_done = 1;
}

static void toplevel_free(struct toplevel *tl) {
	/* note: we can assume that this toplevel is not set as the parent
	 * of any existing toplevels at this point */
	
	zwlr_foreign_toplevel_handle_v1_destroy(tl->handle);
	
	free(tl->props.app_id);
	free(tl->props.menubar_path);
	free(tl->props.menubar_bus_name);
	free(tl->props.window_object_path);
	free(tl->props.window_bus_name);
	free(tl->props.application_object_path);
	free(tl->props.application_bus_name);
	free(tl->props.kde_service_name);	
	free(tl->props.kde_object_path);
	
	free(tl);
}

static void closed_cb(void* data, G_GNUC_UNUSED wfthandle* handle) {
	if(!data) return;
	struct toplevel* tl = (struct toplevel*)data;
	wl_list_remove(&(tl->link));
	toplevel_free(tl);
}

void parent_cb(void* data, G_GNUC_UNUSED wfthandle* handle, wfthandle* parent) {
	if(!data) return;
	struct toplevel* tl = (struct toplevel*)data;
	tl->parent = parent;
}

static void client_annotations_cb(void *data, G_GNUC_UNUSED wfthandle* handle,
		const char *interface, const char *bus_name, const char *object_path) {
	if(!data) return;
	struct toplevel* tl = (struct toplevel*)data;

	fprintf(stderr, "got client annotations: %s %s %s\n", interface, bus_name, object_path);

	// we only care if this is the application_object_path, which corresponds
	// to the org.gtk.Actions interface
	if(!strcmp(interface, "org.gtk.Actions")) {
		free(tl->props.application_object_path);
		free(tl->props.application_bus_name);
		
		if (object_path != NULL)
			tl->props.application_object_path = strdup(object_path);
		else tl->props.application_object_path = NULL;

		if (bus_name != NULL)
			tl->props.application_bus_name = strdup(bus_name);
		else tl->props.application_bus_name = NULL;
	}
}

static void surface_annotations_cb(void *data, G_GNUC_UNUSED wfthandle* handle,
		const char *interface, const char *bus_name, const char *object_path) {
	if(!data) return;
	struct toplevel* tl = (struct toplevel*)data;

	fprintf(stderr, "got surface annotations: %s %s %s\n", interface, bus_name, object_path);

	if(!strcmp(interface, "org.gtk.Actions")) {
		free(tl->props.window_object_path);
		free(tl->props.window_bus_name);

		if (object_path != NULL)
			tl->props.window_object_path = strdup(object_path);
		else tl->props.window_object_path = NULL;

		if (bus_name != NULL)
			tl->props.window_bus_name = strdup(bus_name);
		else tl->props.window_bus_name = NULL;
	}
	else if(!strcmp(interface, "org.gtk.Menus")) {
		free(tl->props.menubar_path);
		free(tl->props.menubar_bus_name);

		if (object_path != NULL)
			tl->props.menubar_path = strdup(object_path);
		else tl->props.menubar_path = NULL;

		if (bus_name != NULL)
			tl->props.menubar_bus_name = strdup(bus_name);
		else tl->props.menubar_bus_name = NULL;
	}
	else if (!strcmp(interface, "com.canonical.dbusmenu")) {
		free(tl->props.kde_object_path);
		free(tl->props.kde_service_name);

		if (object_path != NULL)
			tl->props.kde_object_path = strdup(object_path);
		else tl->props.kde_object_path = NULL;

		if (bus_name != NULL)
			tl->props.kde_service_name = strdup(bus_name);
		else tl->props.kde_service_name = NULL;
	}
}

struct zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_interface = {
    .title        = title_cb,
    .app_id       = appid_cb,
    .output_enter = output_enter_cb,
    .output_leave = output_leave_cb,
    .state        = state_cb,
    .done         = done_cb,
    .closed       = closed_cb,
    .parent       = parent_cb,
    .client_dbus_annotation  = client_annotations_cb,
    .surface_dbus_annotation = surface_annotations_cb
};

/* register new toplevel */
static void new_toplevel(void *data, G_GNUC_UNUSED struct zwlr_foreign_toplevel_manager_v1 *manager,
		wfthandle *handle) {
	if(!handle) return;
	if(!data) {
		/* if we unset the user data pointer, then we don't care anymore */
		zwlr_foreign_toplevel_handle_v1_destroy(handle);
		return;
	}
	struct toplevel_manager* gr = (struct toplevel_manager*)data;
	struct toplevel* tl = (struct toplevel*)calloc(1, sizeof(struct toplevel));
	if(!tl) {
		/* TODO: error message */
		return;
	}
	tl->handle = handle;
	tl->gr = gr;
	wl_list_insert(&(gr->toplevels), &(tl->link));
	
	/* note: we cannot do anything as long as we get app_id */
	zwlr_foreign_toplevel_handle_v1_add_listener(handle, &toplevel_handle_interface, tl);
}

/* sent when toplevel management is no longer available -- this will happen after stopping */
static void toplevel_manager_finished(G_GNUC_UNUSED void *data,
		struct zwlr_foreign_toplevel_manager_v1 *manager) {
    zwlr_foreign_toplevel_manager_v1_destroy(manager);
}

static struct zwlr_foreign_toplevel_manager_v1_listener toplevel_manager_interface = {
    .toplevel = new_toplevel,
    .finished = toplevel_manager_finished,
};

static void registry_global_add_cb(void *data, struct wl_registry *registry,
		uint32_t id, const char *interface, uint32_t version) {
	struct toplevel_manager* gr = (struct toplevel_manager*)data;
	if(!strcmp(interface, zwlr_foreign_toplevel_manager_v1_interface.name)) {
		uint32_t v = zwlr_foreign_toplevel_manager_v1_interface.version;
		if(version < v) v = version;
		gr->manager = wl_registry_bind(registry, id, &zwlr_foreign_toplevel_manager_v1_interface, v);
		if(gr->manager)
			zwlr_foreign_toplevel_manager_v1_add_listener(gr->manager, &toplevel_manager_interface, gr);
		else { /* TODO: handle error */ }
	}
	gr->init_done = 0;
}

static void registry_global_remove_cb(G_GNUC_UNUSED void *data,
		G_GNUC_UNUSED struct wl_registry *registry, G_GNUC_UNUSED uint32_t id) {
	/* don't care */
}

static const struct wl_registry_listener registry_listener = {
	registry_global_add_cb,
	registry_global_remove_cb
};

struct toplevel_manager* toplevel_manager_new() {
	struct toplevel_manager* gr = (struct toplevel_manager*)calloc(1, sizeof(struct toplevel_manager));
	if(!gr) return NULL;
	
	struct wl_display* dpy = NULL;
	GdkDisplay *gdkdsp = gdk_display_get_default();
	if (GDK_IS_WAYLAND_DISPLAY (gdkdsp))
		dpy = gdk_wayland_display_get_wl_display(gdkdsp);
	
	if(!dpy) {
		fprintf(stderr, "Cannot connect to Wayland display, not running in a Wayland session?\n");
		free(gr);
		return NULL;
	}
	
	wl_list_init(&(gr->toplevels));
	
	struct wl_registry* registry = wl_display_get_registry(dpy);
	wl_registry_add_listener(registry, &registry_listener, gr);
	do {
		gr->init_done = 1;
		wl_display_roundtrip(dpy);
	}
	while(!gr->init_done);
	
	if(!gr->manager) {
		fprintf(stderr, "Could not bind wlr-foreign-toplevel interface, your compositor might not support this protocol\n");
		free(gr);
		return NULL;
	}
	return gr;
}

const struct toplevel_properties* toplevel_manager_get_active_app(struct toplevel_manager* gr) {
	if(gr && gr->active) return &(gr->active->props);
	return NULL;
}

void toplevel_manager_set_callback(struct toplevel_manager* gr,
		void (*callback)(void* data, struct toplevel_manager* gr), void* data) {
	if(gr) {
		gr->callback = callback;
		gr->data = data;
	}
}

void toplevel_manager_set_self(struct toplevel_manager* gr, const char* self_id) {
	if(!gr) return;
	free(gr->self);
	if(self_id) gr->self = strdup(self_id);
	else gr->self = NULL;
}

void toplevel_manager_free(struct toplevel_manager* gr) {
	if(!gr) return;
	/* stop listening and also free all existing toplevels */
	/* set user data to null -- this will stop adding newly reported toplevels */
	zwlr_foreign_toplevel_manager_v1_set_user_data(gr->manager, NULL);
	/* this will send the finished signal and result in destroying manager later */
	zwlr_foreign_toplevel_manager_v1_stop(gr->manager);
	gr->manager = NULL;
	free(gr->self);
	/* destroy all existing toplevel handles */
	struct toplevel* tl;
	struct toplevel* tmp;
	wl_list_for_each_safe(tl, tmp, &(gr->toplevels), link) {
		wl_list_remove(&(tl->link));
		toplevel_free(tl);
	}
}


