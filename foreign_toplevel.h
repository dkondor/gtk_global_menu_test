/*
 * foreign_toplevel.h -- interface to the wlr-foreign-toplevel protocol 
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


#ifndef FOREIGN_TOPLEVEL_H
#define FOREIGN_TOPLEVEL_H

#ifdef __cplusplus
extern "C" {
#endif


struct toplevel_manager;

/* properties of toplevels we care about */
struct toplevel_properties {
	char* app_id;
	char *menubar_path;
	char *menubar_bus_name;
	char *window_object_path;
	char *window_bus_name;
	char *application_object_path;
	char *application_bus_name;
	
	char *kde_service_name;
	char *kde_object_path;
};

/*
 * Create a new manager and start listening to events about toplevels.
 */
struct toplevel_manager* toplevel_manager_new();

/*
 * Get info about the last activated app (if any).
 */
const struct toplevel_properties* toplevel_manager_get_active_app(struct toplevel_manager* gr);

/* 
 * Set our own app-id; apps with this ID will be ignored.
 */
void toplevel_manager_set_self(struct toplevel_manager* gr, const char* self_id);

/* 
 * Set the callback function to be called when a new toplevel is activated.
 */
void toplevel_manager_set_callback(struct toplevel_manager* gr,
		void (*callback)(void* data, struct toplevel_manager* gr), void* data);

/* Stop listening to toplevel events and free all resources associated
 * with this instance.
 */
void toplevel_manager_free(struct toplevel_manager* gr);

#ifdef __cplusplus
}
#endif

#endif

