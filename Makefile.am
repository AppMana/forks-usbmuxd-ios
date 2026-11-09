AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4
check-local:
	$(CC) $(CPPFLAGS) $(CFLAGS) -DHAVE_CLOCK_GETTIME -ffunction-sections -fdata-sections -I$(srcdir)/src $(srcdir)/tests/abort-connect.c $(srcdir)/src/utils.c $(LDFLAGS) -Wl,--gc-sections -pthread -o test-abort-connect
	./test-abort-connect
	$(CC) $(CPPFLAGS) $(CFLAGS) $(libusb_CFLAGS) -DHAVE_CLOCK_GETTIME -ffunction-sections -fdata-sections -I$(srcdir)/src $(srcdir)/tests/usb-disconnect.c $(srcdir)/src/utils.c $(LDFLAGS) -Wl,--gc-sections -pthread -o test-usb-disconnect
	./test-usb-disconnect

CLEANFILES = test-abort-connect test-usb-disconnect
SUBDIRS = src $(UDEV_SUB) $(SYSTEMD_SUB) docs

EXTRA_DIST = \
	tests/abort-connect.c \
	tests/usb-disconnect.c \
	docs \
	COPYING.GPLv2 \
	COPYING.GPLv3 \
	README.md

DISTCHECK_CONFIGURE_FLAGS = \
	--with-udevrulesdir=$$dc_install_base/$(udevrulesdir) \
	--with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir)
