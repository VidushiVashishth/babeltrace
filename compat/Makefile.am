noinst_LTLIBRARIES = libcompat.la

libcompat_la_SOURCES = compat_uuid.c \
		compat_mman.c \
		logging.c logging.h

libcompat_la_LDFLAGS = \
	$(LD_NO_AS_NEEDED)

if BABELTRACE_BUILD_WITH_MINGW
libcompat_la_LDFLAGS += -lrpcrt4
endif
