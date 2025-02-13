project(
	'gtk-global-menu-test',
	'c',
	version: '0.0.1',
	license: 'public-domain',
	meson_version: '>=1.0.0',
	default_options: [
		'c_std=c11',
		'warning_level=2',
		'werror=true',
	],
)


wayland_client = dependency('wayland-client')
wayland_scanner = find_program('wayland-scanner')

wayland_scanner_code = generator(
	wayland_scanner,
	output: '@BASENAME@-protocol.c',
	arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
)

wayland_scanner_client = generator(
	wayland_scanner,
	output: '@BASENAME@-client-protocol.h',
	arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
)


# protocol needed
wayland_protocols = [
    './wlr-foreign-toplevel-management-unstable-v1.xml'
]

wayland_protos_client_src = []
wayland_protos_headers = []

foreach p : wayland_protocols
	xml = join_paths(p)
	wayland_protos_headers += wayland_scanner_client.process(xml)
	wayland_protos_client_src += wayland_scanner_code.process(xml)
endforeach

lib_protos = static_library('protos', wayland_protos_client_src + wayland_protos_headers,
	dependencies: [wayland_client]) # for the include directory

lib_protos_dep = declare_dependency(
	link_with: lib_protos,
	sources: wayland_protos_headers,
)


# GUI dependencies
glib     = dependency('glib-2.0')
gtk      = dependency('gtk+-3.0')
gdk      = dependency('gdk-3.0')
gdkwl    = dependency('gdk-wayland-3.0')
dbusmenu = dependency('dbusmenu-gtk3-0.4')


global_menu_test = executable('gtk_global_menu_test',
	['main.c', 'foreign_toplevel.c', 'foreign_toplevel.h'],
	dependencies: [wayland_client, lib_protos_dep, glib, gtk, gdk, gdkwl, dbusmenu],
	install: false)



