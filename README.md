# GTK Global menu test

Test application for global menus on Wayland.

**Warning** the software in this repository is meant for demonstration purposes only and relies on functionality not present in any available Wayland compositor. Please do not submit bug reports to other projects if you experience issues. See the related PRs: <br>
https://gitlab.freedesktop.org/wlroots/wlr-protocols/-/merge_requests/131 <br>
https://gitlab.freedesktop.org/wlroots/wlroots/-/merge_requests/4986 <br>
https://github.com/WayfireWM/wayfire/pull/2576

This app demonstrates potential functionality proposed for the `wlr-foreign-toplevel-management` protocol (included in this repository) that would allow a compositor to relay information about global menus to a desktop component (e.g. a panel or dock). It is meant as a minimal example and will simple present menus of compatible apps in its main window. Both the `org.gtk.Menus` and the `com.canonical.dbusmenu` interfaces are supported.

### Requirements

 - Compositor implementation of the proposed protocol extension; use [this](https://gitlab.freedesktop.org/dkondor1/wlroots/-/tree/foreign_toplevel_appmenu2?ref_type=heads) or [this](https://gitlab.freedesktop.org/dkondor1/wlroots/-/tree/foreign_toplevel_appmenu) branch for wlroots and [this](https://github.com/dkondor/wayfire/tree/global_menu2) branch of Wayfire.
 - Libraries and development files for GLib, Gtk 3.0 and GDK
 - Libraries and development files for Wayland (`wayland-client`) and the `wayland-scanner` program
 - Libraries and development files for dbusmenu-gtk3 (on Ubuntu, this means the `libdbusmenu-gtk3-dev` package)
 - `appmenu-gtk3-module`

### Compiling

Use the standard Meson way:
```
meson setup build
ninja -C build
```

### Running

Start from the build folder (it will not be installed):
```
build/gtk_global_menu_test
```

The app-id of the last active app is displayed, and if it supports global menus, its menu can be shown by clicking on the "Show menu" button. Note: in some case, the active app is not correctly detected and you might need to switch away and back to it for things to work.

### Making apps work

So far, I've tested it with GTK3 and Qt5 apps, specifically with Gedit, Inkscape and Kate (versions available in Ubuntu 24.04). The following are required to make apps actually export their menus:

1. A menu "registrar" needs to be running -- this is a program that implements the `com.canonical.AppMenu.Registrar` DBus interface. This actually plays no role on Wayland, but seems to be required to be present to "trigger" exporting menus. A possible implementation is available [here](https://github.com/Cairo-Dock/cairo-dock-plug-ins/blob/master/Global-Menu/registrar/appmenu-registrar.py) (note: it is sufficient to download and run this one file, no need to clone the whole repository).

2. For GTK3 apps, you need to set the `GTK_MODULES` environment variable to include `appmenu-gtk-module`

3. For Qt apps, you need to set the `XDG_CURRENT_DESKTOP` environment variable to start with `KDE`


## License

The protocol file (`wlr-foreign-toplevel-management-unstable-v1.xml`) can be used under the license terms included in it. All other content in this repository is released into the public domain.



