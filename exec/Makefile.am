AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -std=c++11 -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
AM_LDFLAGS = -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-rpath=/usr/local/lib/homegear
LIBS += -Wl,-Bdynamic -lhomegear-node -lhomegear-base

libdir = $(localstatedir)/lib/homegear/node-blue/nodes/exec
lib_LTLIBRARIES = exec.la
exec_la_SOURCES = Factory.cpp Exec.cpp
exec_la_LDFLAGS =-module -avoid-version -shared
exec_ladir = $(libdir)
exec_la_DATA = exec.hni
locale_en_usdir = $(libdir)/locales/en-US
locale_en_us_DATA = locales/en-US/exec
locale_de_dedir = $(libdir)/locales/de-DE
locale_de_de_DATA = locales/de-DE/exec
install-exec-hook:
	rm -f $(DESTDIR)$(libdir)/exec.la
