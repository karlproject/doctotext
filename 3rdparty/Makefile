LIBICONV_VERSION=1.11
GETTEXT_VERSION = 0.18.1.1
GLIB_VERSION=2.18.2
ZLIB_VERSION=1.2.7
LIBXML2_VERSION=2.7.7
LIBGSF_VERSION=1.14.22

ifeq ($(WIN32),1)
ARCH = win32
CONFIG_ARGS =
ARCH_FLAGS =
MAKE_ARGS =
else
	UNAME = $(shell uname)
	ifeq ($(UNAME),Darwin)
		ARCH = i386-x86_64-macosx
		ifdef MACOSX_ARCH
			ARCH_FLAGS = -arch $(MACOSX_ARCH)
		else
			ARCH_FLAGS = -arch i386 -arch x86_64
		endif
		CONFIG_ARGS = --disable-dependency-tracking
		MAKE_ARGS = LIBTOOL=glibtool CFLAGS="$(ARCH_FLAGS)" CXXFLAGS="$(ARCH_FLAGS)" LDFLAGS="$(ARCH_FLAGS)"
	else
		ARCH = x86-linux
		CONFIG_ARGS =
		ARCH_FLAGS =
		MAKE_ARGS =
	endif
endif

pwd = $(shell pwd)

ifeq ($(WIN32),1)

CC = i686-w64-mingw32-gcc
XMINGW_TOOLS = \
	CC=i686-w64-mingw32-gcc \
	CXX=i686-w64-mingw32-g++ \
	AR=i686-w64-mingw32-ar \
	RANLIB=i686-w64-mingw32-ranlib \
	WINDRES=i686-w64-mingw32-windres \
	STRIP=i686-w64-mingw32-strip \
	OBJDUMP=i686-w64-mingw32-objdump \

libiconv: libiconv-$(LIBICONV_VERSION)-$(ARCH).tar.bz2
	sha1sum -c libiconv-$(LIBICONV_VERSION)-$(ARCH).tar.bz2.sha1
	tar -xjvf libiconv-$(LIBICONV_VERSION)-$(ARCH).tar.bz2
	touch libiconv

libiconv-$(LIBICONV_VERSION)-$(ARCH).tar.bz2:
	wget http://silvercoders.com/download/3rdparty/libiconv-$(LIBICONV_VERSION)-$(ARCH).tar.bz2

gettext: gettext-$(GETTEXT_VERSION)-$(ARCH).tar.bz2
	sha1sum -c gettext-$(GETTEXT_VERSION)-$(ARCH).tar.bz2.sha1
	tar -xjvf gettext-$(GETTEXT_VERSION)-$(ARCH).tar.bz2
	touch gettext

gettext-$(GETTEXT_VERSION)-$(ARCH).tar.bz2:
	wget http://silvercoders.com/download/3rdparty/gettext-$(GETTEXT_VERSION)-$(ARCH).tar.bz2

glib: glib-$(GLIB_VERSION)-$(ARCH).tar.bz2
	sha1sum -c glib-$(GLIB_VERSION)-$(ARCH).tar.bz2.sha1
	tar -xjvf glib-$(GLIB_VERSION)-$(ARCH).tar.bz2
	rm glib/lib/*.la
	touch glib

glib-$(GLIB_VERSION)-$(ARCH).tar.bz2:
	wget http://silvercoders.com/download/3rdparty/glib-$(GLIB_VERSION)-$(ARCH).tar.bz2

zlib: zlib-$(ZLIB_VERSION)-$(ARCH).tar.bz2
	sha1sum -c zlib-$(ZLIB_VERSION)-$(ARCH).tar.bz2.sha1
	tar -xjvf zlib-$(ZLIB_VERSION)-$(ARCH).tar.bz2
	touch zlib

zlib-$(ZLIB_VERSION)-$(ARCH).tar.bz2:
	wget http://silvercoders.com/download/3rdparty/zlib-$(ZLIB_VERSION)-$(ARCH).tar.bz2

libxml2: libxml2-$(LIBXML2_VERSION)-$(ARCH).tar.bz2
	sha1sum -c libxml2-$(LIBXML2_VERSION)-$(ARCH).tar.bz2.sha1
	tar -xjvf libxml2-$(LIBXML2_VERSION)-$(ARCH).tar.bz2
	touch libxml2

libxml2-$(LIBXML2_VERSION)-$(ARCH).tar.bz2:
	wget http://silvercoders.com/download/3rdparty/libxml2-$(LIBXML2_VERSION)-$(ARCH).tar.bz2

libgsf: libgsf-$(LIBGSF_VERSION)-$(ARCH).tar.bz2
	sha1sum -c libgsf-$(LIBGSF_VERSION)-$(ARCH).tar.bz2.sha1
	tar -xjvf libgsf-$(LIBGSF_VERSION)-$(ARCH).tar.bz2
	rm libgsf/lib/*.la
	touch libgsf

libgsf-$(LIBGSF_VERSION)-$(ARCH).tar.bz2:
	wget http://silvercoders.com/download/3rdparty/libgsf-$(LIBGSF_VERSION)-$(ARCH).tar.bz2

endif

wv2: wv2-0.2.3/src/.libs/libwv2.a

ifeq ($(WIN32),1)
wv2_pkg = PKG_CONFIG_SYSROOT_DIR=!disabled
wv2_host = --host=mingw32 $(XMINGW_TOOLS)
wv2_deps = libiconv libgsf glib
wv2_iconv = --with-libiconv=$(pwd)/libiconv
wv2_cxxflags = CXXFLAGS="-I$(pwd)/libgsf/include/libgsf-1 -I$(pwd)/glib/include/glib-2.0 -I$(pwd)/glib/lib/glib-2.0/include -I$(pwd)/libiconv/include"
#wv2_ldflags = LDFLAGS="-L$(pwd)/zlib/lib -L$(pwd)/glib/lib -L$(pwd)/libxml2/lib "
wv2_ldflags = LDFLAGS="-L$(pwd)/libgsf/lib -L$(pwd)/glib/lib -L$(pwd)/libxml2/lib"
else
wv2_pkg =
wv2_host =
wv2_deps =
wv2_iconv =
wv2_cxxflags =
wv2_ldflags =
endif

wv2-0.2.3/src/.libs/libwv2.a: $(wv2_deps) wv2-0.2.3/.unpacked
ifeq ($(WIN32),1)
	cd wv2-0.2.3 && \
	if ! test -f .patched; then \
		patch -i ../wv2_configure_no_try_run.diff; \
		patch -d src -i ../../wv2_ustring_expotential_growth.patch; \
		patch -p0 -i ../wv2_parentstyle_upechpx_fix.patch; \
		sed -i 's/#include <algorithm>/#include <algorithm>\n#include <string.h>/' src/styles.cpp; \
		sed -i "s|#include <glib/giochannel.h>|#include <glib.h>|" src/olestream.h; \
		touch .patched; \
	fi
else
	cd wv2-0.2.3 && \
	if ! test -f .patched; then \
		patch -d src -i ../../wv2_ustring_expotential_growth.patch; \
		patch -p0 -i ../wv2_parentstyle_upechpx_fix.patch; \
		sed -i 's/#include <algorithm>/#include <algorithm>\n#include <string.h>/' src/styles.cpp; \
		sed -i 's|#include <glib/giochannel.h>|#include <glib.h>|' src/olestream.h; \
		touch .patched; \
	fi
endif
	cd wv2-0.2.3 && \
	$(wv2_pkg) ./configure $(wv2_host) \
		--enable-static $(CONFIG_ARGS) \
		$(wv2_iconv) $(wv2_cxxflags) $(wv2_ldflags) && \
	$(MAKE) $(MAKE_ARGS) $(wv2_cxxflags)

wv2-0.2.3/.unpacked: wv2-0.2.3.tar.bz2
	tar -xjvf wv2-0.2.3.tar.bz2
	touch wv2-0.2.3/.unpacked

wv2-0.2.3.tar.bz2:
	wget http://surfnet.dl.sourceforge.net/sourceforge/wvware/wv2-0.2.3.tar.bz2

unzip: unzip101e/unzip.o


ifeq ($(WIN32),1)
unzip101e/unzip.o: zlib unzip101e/.unpacked
	cd unzip101e && $(MAKE) CC=$(CC) CFLAGS=-I../zlib/include unzip.o ioapi.o iowin32.o
else
unzip101e/unzip.o: unzip101e/.unpacked
	cd unzip101e && $(MAKE) CC=$(CC) CFLAGS="$(ARCH_FLAGS)" unzip.o ioapi.o
endif

unzip101e/.unpacked: unzip101e.zip
	unzip -d unzip101e unzip101e.zip
	touch unzip101e/.unpacked

unzip101e.zip:
	wget http://www.winimage.com/zLibDll/unzip101e.zip

clean:
	rm -rf libiconv
	rm -rf gettext
	rm -rf glib
	rm -rf zlib
	rm -rf libxml2
	rm -rf libgsf
	rm -rf unzip101e
	rm -rf wv2-0.2.3
