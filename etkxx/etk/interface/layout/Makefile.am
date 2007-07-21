AUTOMAKE_OPTIONS = 1.7

DEFAULT_INCLUDES = -I$(top_builddir)

INCLUDES =	$(ETK_THREAD_CFLAGS) 			\
		$(ETK_EXTRA_CFLAGS)			\
		-DETK_COMPILATION 			\
		@ETK_DEBUG_FLAGS@

EXTRA_DIST =	Layout.h

noinst_LTLIBRARIES = libetk-interface-layout.la

libetk_interface_layout_la_SOURCES =
libetk_interface_layout_la_LIBADD =
libetk_interface_layout_la_DEPENDENCIES =

DISTCLEANFILES = Makefile.in

