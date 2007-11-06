MAJOR_VERSION = 0
MINOR_VERSION = 0
MICRO_VERSION = 1

LITE_BEAPI_VERSION = $(MAJOR_VERSION).$(MINOR_VERSION).$(MICRO_VERSION)

HEADERS_SRC_DIR = $(shell pwd)/headers

SUPPORT_KIT =				\
	be/SupportKit.h			\
	be/support/Errors.h		\
	be/support/SupportDefs.h	\
	be/support/ClassInfo.h		\
	be/support/Debug.h		\
	be/support/Locker.h		\
	be/support/List.h		\
	be/support/String.h

APP_KIT =				\
	be/AppKit.h			\
	be/app/AppDefs.h		\
	be/app/Handler.h		\
	be/app/Looper.h			\
	be/app/Application.h		\
	be/app/Message.h		\
	be/app/Messenger.h		\
	be/app/Invoker.h

INTERFACE_KIT =				\
	be/InterfaceKit.h		\
	be/interface/GraphicsDefs.h	\
	be/interface/InterfaceDefs.h	\
	be/interface/Point.h		\
	be/interface/Rect.h		\
	be/interface/Region.h		\
	be/interface/Font.h		\
	be/interface/View.h		\
	be/interface/Bitmap.h		\
	be/interface/ScrollBar.h	\
	be/interface/ScrollView.h	\
	be/interface/Window.h		\
	be/interface/ListItem.h		\
	be/interface/ListView.h		\
	be/interface/OutlineListView.h

HEADERS = be/BeBuild.h be/Be.h $(SUPPORT_KIT) $(APP_KIT) $(INTERFACE_KIT)
HEADERS_DIRS = be/support be/app be/interface be

MKDIR ?= mkdir -p
RMDIR ?= rmdir
RM ?= rm -f
CAT ?= cat
CHMOD ?= chmod

INSTALL ?= install
INSTALL_DATA ?= $(INSTALL) -m 644

PREFIX ?= /usr
DESTDIR ?=

all:
	@echo "***"
	@echo "*** run \"make install PREFIX=/usr\" please."
	@echo "***"

install_headers:
	@for f in $(HEADERS_DIRS); do \
		test -d $(DESTDIR)$(PREFIX)/include/Lite\ BeAPI/$$f || \
		echo "Creating directory \"$$f\"" && \
		$(MKDIR) $(DESTDIR)$(PREFIX)/include/Lite\ BeAPI/$$f > /dev/null 2>&1 || exit 1; \
	done
	@for f in $(HEADERS); do \
		echo "Installing \"$$f\"" && \
		$(INSTALL_DATA) "$(HEADERS_SRC_DIR)/$$f" "$(DESTDIR)$(PREFIX)/include/Lite BeAPI/$$f" > /dev/null 2>&1 || \
		exit 1; \
	done

install_config:
	@echo "Installing \"lite-beapi-config\""
	@printf "\
#!/bin/sh\n\
\n\
prefix=$(PREFIX)\n\
libdir=$(PREFIX)/lib\n\
includedir=$(PREFIX)/include\n\
lite_beapi_version=\"$(LITE_BEAPI_VERSION)\"\n\
lite_beapi_config_output=\n\
lite_beapi_headers_dirs=\"$(HEADERS_DIRS)\"\n\
\n\
while test \$$# -gt 0; do\n\
	case \"\$$1\" in\n\
	--cflags)\n\
		lite_beapi_config_output=\"\$$lite_beapi_config_output \`etkxx-config --cflags\` -I \$$includedir/Lite\\\ BeAPI\"\n\
		for f in \$$lite_beapi_headers_dirs; do \\\\\n\
			lite_beapi_config_output=\"\$$lite_beapi_config_output -I \$$includedir/Lite\ BeAPI/\$$f\"; \\\\\n\
		done\n\
		;;\n\
\n\
	--libs)\n\
		lite_beapi_config_output=\"\$$lite_beapi_config_output \`etkxx-config --libs\`\"\n\
		;;\n\
	--version)\n\
		echo \$$lite_beapi_version\n\
		exit 0\n\
		;;\n\
	--atleast-version)\n\
		if test \"x\$$2\" = \"x\"; then\n\
			exit 1\n\
		fi\n\
\n\
		atleast_major_version=\`echo \$$2 | sed 's/\([0-9]*\).\([0-9]*\).\([0-9]*\)/\\\1/'\`\n\
		atleast_minor_version=\`echo \$$2 | sed 's/\([0-9]*\).\([0-9]*\).\([0-9]*\)/\\\2/'\`\n\
		atleast_micro_version=\`echo \$$2 | sed 's/\([0-9]*\).\([0-9]*\).\([0-9]*\)/\\\3/'\`\n\
\n\
		lite_beapi_major_version=\`echo \$$lite_beapi_version | sed 's/\([0-9]*\).\([0-9]*\).\([0-9]*\)/\\\1/'\`\n\
		lite_beapi_minor_version=\`echo \$$lite_beapi_version | sed 's/\([0-9]*\).\([0-9]*\).\([0-9]*\)/\\\2/'\`\n\
		lite_beapi_micro_version=\`echo \$$lite_beapi_version | sed 's/\([0-9]*\).\([0-9]*\).\([0-9]*\)/\\\3/'\`\n\
\n\
		if test \$$lite_beapi_major_version -lt \$$atleast_major_version; then\n\
			exit 1\n\
		elif test \$$lite_beapi_major_version -gt \$$atleast_major_version; then\n\
			exit 0\n\
		fi\n\
\n\
		if test \$$lite_beapi_minor_version -lt \$$atleast_minor_version; then\n\
			exit 1\n\
		elif test \$$lite_beapi_minor_version -gt \$$atleast_minor_version; then\n\
			exit 0\n\
		fi\n\
\n\
		if test \$$lite_beapi_micro_version -lt \$$atleast_micro_version; then\n\
			exit 1\n\
		fi\n\
\n\
		exit 0\n\
		;;\n\
	*)\n\
		echo \"Usage: lite-beapi-config [--version] [--atleast-version x.x.x] [--cflags] [--libs]\"\n\
		exit 1\n\
		;;\n\
	esac\n\
	shift\n\
done\n\
\n\
if test \"x\$$lite_beapi_config_output\" != \"x\"; then\n\
	echo \$$lite_beapi_config_output\n\
fi\n\
" > $(DESTDIR)$(PREFIX)/bin/lite-beapi-config
	@$(CHMOD) +x $(DESTDIR)$(PREFIX)/bin/lite-beapi-config

install: install_headers install_config

uninstall:
	@for f in $(HEADERS); do \
		echo "Removing \"$$f\"" && \
		$(RM) "$(DESTDIR)$(PREFIX)/include/Lite BeAPI/$$f" > /dev/null 2>&1; \
	done
	@-for f in $(HEADERS_DIRS); do \
		test ! -d $(DESTDIR)$(PREFIX)/include/Lite\ BeAPI/$$f || \
		echo "Removing directory \"$$f\"" && \
		$(RMDIR) $(DESTDIR)$(PREFIX)/include/Lite\ BeAPI/$$f > /dev/null 2>&1 || true; \
	done
	@-$(RMDIR) $(DESTDIR)$(PREFIX)/include/Lite\ BeAPI > /dev/null 2>&1 || true
	@echo "Removing \"lite-beapi-config\""
	@-$(RM) $(DESTDIR)$(PREFIX)/bin/lite-beapi-config > /dev/null 2>&1

