ifeq ($(WIN32),1)
STRIP = i686-w64-mingw32-strip
ARCH = win32
EXEEXT=.exe
SOPRE =
SOEXT = .dll
else
	UNAME = $(shell uname)
	ifeq ($(UNAME),Darwin)
		STRIP = strip -x
		ifdef MACOSX_ARCH
			ARCH = $(MACOSX_ARCH)-macosx
		else
			ARCH = i386-x86_64-macosx
		endif
		ifeq ($(MACOSX_STATIC),1)
			ARCH := $(ARCH)-static
		endif
		SOEXT = .dylib
	else
		STRIP = strip
		ARCH = x86-linux
		SOEXT = .so
	endif
	EXEEXT=
	SOPRE = lib
endif

ifeq ($(DEBUG),1)
ARCH := $(ARCH)-debug
STRIP = touch
endif

build: version.h
	$(MAKE) -C 3rdparty wv2
	$(MAKE) -C 3rdparty unzip
ifeq ($(WIN32),1)
	$(MAKE) -C 3rdparty libxml2
	$(MAKE) -C 3rdparty gettext
endif
	$(MAKE) -C src
	rm -rf build
	mkdir build
ifeq ($(WIN32),1)
	cp 3rdparty/glib/bin/lib{glib,gio,gmodule,gobject}-2.0-0.dll build
	cp 3rdparty/libgsf/bin/libgsf-1-114.dll build
	cp 3rdparty/libiconv/bin/libiconv-2.dll build
	cp 3rdparty/gettext/bin/libintl-8.dll build
	cp 3rdparty/libxml2/bin/libxml2-2.dll build
	cp 3rdparty/zlib/bin/zlib1.dll build
	for f in $(subst :, ,$(PATH)); do \
		if test -f "$$f/libgcc_s_sjlj-1.dll"; then \
			cp "$$f/libgcc_s_sjlj-1.dll" build/; \
			break; \
		fi \
	done
	$(STRIP) build/*.dll
else
ifeq ($(UNAME),Darwin)
ifneq ($(MACOSX_STATIC),1)
	cp /opt/local/lib/libglib-2.0.0.dylib build/
	cp /opt/local/lib/libgobject-2.0.0.dylib build/
	cp /opt/local/lib/libgsf-1.114.dylib build/
	cp /opt/local/lib/libintl.8.dylib build/
	cp /opt/local/lib/libxml2.2.dylib build/
	cp /opt/local/lib/libbz2.1.dylib build/
	cp /opt/local/lib/libgio-2.0.0.dylib build/
	cp /opt/local/lib/libffi.5.dylib build/
	cp /opt/local/lib/libgmodule-2.0.0.dylib build/
	cp /opt/local/lib/libgthread-2.0.0.dylib build/
	cp /opt/local/lib/libiconv.2.dylib build/
	cp 3rdparty/wv2-0.2.3/src/.libs/libwv2.1.dylib build/
	$(STRIP) build/*.dylib
endif
	echo 'DYLD_LIBRARY_PATH=.:$$DYLD_LIBRARY_PATH ./doctotext "$$@"' > build/doctotext.sh
else
	cp 3rdparty/wv2-0.2.3/src/.libs/libwv2.so.1 build
	$(STRIP) build/*.so*
	echo 'LD_LIBRARY_PATH=.:$$LD_LIBRARY_PATH ./doctotext "$$@"' > build/doctotext.sh
endif
	chmod u+x build/doctotext.sh
endif
	cp src/doctotext$(EXEEXT) build
	cp src/$(SOPRE)doctotext$(SOEXT) build/
	cp src/plain_text_extractor.h build/
	cp src/formatting_style.h build/
	cp src/doctotext_c_api.h build/
	cp ChangeLog VERSION build
	$(MAKE) -C tests

version.h: VERSION
	echo "#define VERSION \"`cat VERSION`\"" > version.h

clean:
	rm -rf build
	rm -f version.h
	$(MAKE) -C 3rdparty clean
	$(MAKE) -C src clean
	$(MAKE) -C tests clean

snapshot: clean
	snapshot_fn=$$TMPDIR/doctotext-`date +%Y%m%d`.tar.bz2 && \
	tar -cjvf $$snapshot_fn ../doctotext --exclude .svn --exclude "*.tar.*" --exclude "*.zip" && \
	mv $$snapshot_fn .

release: clean
	release_fn=$$TMPDIR/doctotext-`cat VERSION`.tar.bz2 && \
	tar -cjvf $$release_fn ../doctotext --exclude .svn --exclude "*.tar.*" --exclude "*.zip" && \
	mv $$release_fn .

snapshot-bin: build
	mv build doctotext
	tar -cjvf doctotext-`date +%Y%m%d`-$(ARCH).tar.bz2 doctotext
	mv doctotext build

release-bin: build
	mv build doctotext
	tar -cjvf doctotext-`cat VERSION`-$(ARCH).tar.bz2 doctotext
	mv doctotext build

unzip101e/.unpacked: unzip101e.zip
	tar -xjvf unzip101e.zip
	touch unzip101e.zip/.unpacked

unzip101e.zip:
	wget http://www.winimage.com/zLibDll/unzip101e.zip

	rm -rf unzip101e
