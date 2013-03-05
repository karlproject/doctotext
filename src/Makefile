CXXFLAGS = -O2 -I../3rdparty/wv2-0.2.3/src -I../3rdparty/unzip101e
LDFLAGS = ../3rdparty/unzip101e/ioapi.o ../3rdparty/unzip101e/unzip.o

COMMON_OBJECTS = doc_parser.o rtf_parser.o odf_ooxml_parser.o misc.o xml_fixer.o doctotext_unzip.o plain_text_extractor.o ppt_parser.o \
	xls_parser.o
OBJECTS = doctotext.o $(COMMON_OBJECTS)
LIB_OBJECTS = doctotext_c_api.o $(COMMON_OBJECTS)
ALL_OBJECTS = doctotext.o doctotext_c_api.o $(COMMON_OBJECTS)

HEADERS = ../version.h doc_parser.h doctotext_c_api.h rtf_parser.h odf_ooxml_parser.h misc.h xml_fixer.h doctotext_unzip.h ppt_parser.h \
	xls_parser.h

ifeq ($(WIN32),1)
STRIP = i686-w64-mingw32-strip
CXX = i686-w64-mingw32-g++
EXEEXT = .exe
SOPRE =
SOEXT = .dll
CXXFLAGS += -I../3rdparty/glib/include/glib-2.0 -I../3rdparty/glib/lib/glib-2.0/include -I../3rdparty/zlib/include -I../3rdparty/libgsf/include/libgsf-1 -I../3rdparty/libxml2/include/libxml2
LDFLAGS += -static-libstdc++ \
	../3rdparty/unzip101e/iowin32.o \
	-L../3rdparty/wv2-0.2.3/src/.libs -lwv2 \
	-L../3rdparty/libgsf/lib -lgsf-1 \
	-L../3rdparty/glib/lib -lgobject-2.0 -lglib-2.0 \
	-L../3rdparty/libiconv/lib -liconv \
	-L../3rdparty/libxml2/lib -llibxml2 \
	-L../3rdparty/zlib/lib -lz
SHARED = -shared
else
CXX = g++
EXEEXT =
CXXFLAGS += `pkg-config glib-2.0 --cflags` `pkg-config libxml-2.0 --cflags` `pkg-config libgsf-1 --cflags`
ifeq ($(MACOSX_STATIC),1)
	LDFLAGS += ../3rdparty/wv2-0.2.3/src/.libs/libwv2.a /opt/local/lib/libglib-2.0.a /opt/local/lib/libintl.a /opt/local/lib/libgsf-1.a /opt/local/lib/libgobject-2.0.a /opt/local/lib/libffi.a /opt/local/lib/libgthread-2.0.a /opt/local/lib/libbz2.a /opt/local/lib/libiconv.a /opt/local/lib/libxml2.a /opt/local/lib/libz.a -framework CoreServices
else
	CXXFLAGS += -fPIC
	LDFLAGS += -L../3rdparty/wv2-0.2.3/src/.libs -lwv2 `pkg-config glib-2.0 --libs` `pkg-config libxml-2.0 --libs` -lz
endif
	SOPRE = lib
	UNAME = $(shell uname)
	ifeq ($(UNAME),Darwin)
		STRIP = strip -x
		SOEXT = .dylib
		SHARED = -dynamiclib
		ifdef MACOSX_ARCH
			CXXFLAGS += -arch $(MACOSX_ARCH)
			LDFLAGS += -arch $(MACOSX_ARCH)
		else
			CXXFLAGS += -arch i386 -arch x86_64
			LDFLAGS += -arch i386 -arch x86_64
		endif
	else
		STRIP = strip
		SOEXT = .so
		SHARED = -shared
	endif
endif

ifeq ($(DEBUG),1)
CXXFLAGS += -g -O0
STRIP = touch
endif

all: doctotext$(EXEEXT) $(SOPRE)doctotext$(SOEXT)

doctotext$(EXEEXT): $(OBJECTS)
	$(CXX) -o doctotext$(EXEEXT) $(OBJECTS) $(LDFLAGS)
	$(STRIP) doctotext$(EXEEXT)

$(SOPRE)doctotext$(SOEXT): $(LIB_OBJECTS)
	$(CXX) $(SHARED) -o $(SOPRE)doctotext$(SOEXT) $(LIB_OBJECTS) $(LDFLAGS)
	$(STRIP) $(SOPRE)doctotext$(SOEXT)

$(ALL_OBJECTS): %.o: %.cpp $(HEADERS)
	$(CXX) -c $< $(CXXFLAGS) 

clean:
	rm -f *.o doctotext doctotext.exe libdoctotext.{so,dylib} doctotext.dll
