AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -std=c++11 -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
AM_LDFLAGS = -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-rpath=/usr/local/lib/homegear
LIBS += -Wl,-Bdynamic -lhomegear-node -lhomegear-base

libdir = $(localstatedir)/lib/homegear/node-blue/nodes/serial

lib_LTLIBRARIES = serial-port.la serial-in.la serial-out.la

serial_port_la_SOURCES = serial-port/Factory.cpp serial-port/MyNode.cpp
serial_port_la_LDFLAGS =-module -avoid-version -shared

serial_in_la_SOURCES = serial-in/Factory.cpp serial-in/MyNode.cpp
serial_in_la_LDFLAGS =-module -avoid-version -shared

serial_out_la_SOURCES = serial-out/Factory.cpp serial-out/MyNode.cpp
serial_out_la_LDFLAGS =-module -avoid-version -shared

serial_ladir = $(libdir)
serial_la_DATA = serial-port/serial-port.hni serial-in/serial-in.hni serial-out/serial-out.hni
locale_en_usdir = $(libdir)/locales/en-US
locale_en_us_DATA = serial-port/locales/en-US/serial-port serial-in/locales/en-US/serial-in serial-out/locales/en-US/serial-out
locale_de_dedir = $(libdir)/locales/de-DE
locale_de_de_DATA = serial-port/locales/de-DE/serial-port serial-in/locales/de-DE/serial-in serial-out/locales/de-DE/serial-out

install-exec-hook:
	rm -f $(DESTDIR)$(libdir)/serial-port.la
	rm -f $(DESTDIR)$(libdir)/serial-in.la
	rm -f $(DESTDIR)$(libdir)/serial-out.la
