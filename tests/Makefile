ifeq ($(WIN32),1)
EXEC = wine cmd /c "set PATH=..\\build;%PATH% &&"
EXEEXT = .exe
SOPRE =
SOEXT = .dll
CC = i686-w64-mingw32-gcc
CXX = i686-w64-mingw32-g++
STRIP = i686-w64-mingw32-strip
else
	EXEEXT =
	SOPRE = lib
	UNAME = $(shell uname)
	ifeq ($(UNAME),Darwin)
		LIB_PATH = DYLD_LIBRARY_PATH=../build
		SOEXT = .dylib
                ifdef MACOSX_ARCH
                        ARCH_FLAGS += -arch $(MACOSX_ARCH)
                else
                        ARCH_FLAGS += -arch i386 -arch x86_64
                endif
	else
		LIB_PATH = LD_LIBRARY_PATH=../build
		SOEXT = .so
		ARCH_FLAGS =
	endif
	EXEC = $(LIB_PATH)
	CC = gcc
	CXX = g++
	STRIP = strip
endif

test: c_api_test$(EXEEXT) cpp_api_test$(EXEEXT)
	@echo
	@echo TESTING IF CONVERTER WORKS CORRECTLY...
	@echo
	@for f in *.odt; do \
		f=`basename $$f .odt`; \
		for e in odt ods odp rtf doc xls ppt docx xlsx pptx; do \
			echo TESTING $$f.$$e; \
			if ! $(EXEC) ../build/doctotext $$f.$$e | tr -d '\r' | diff -u - $$f.$$e.out; then \
				exit 1; \
			fi; \
		done; \
	done
	@for f in hebrew_*.rtf japanese.xls embedded_spreadsheet.doc encoding_in_table.doc crash_test.xls special_para_cmds.rtf \
			header_footer.doc; do \
		echo TESTING $$f; \
		if ! $(EXEC) ../build/doctotext $$f | tr -d '\r' | diff -u - $$f.out; then \
			exit 1; \
		fi; \
	done
	@echo
	@echo TESTING SPEED OF CONVERTER...
	@echo
	@for f in *.gz; do \
		f=`basename $$f .gz`; \
		echo TESTING $$f; \
		gunzip -c $$f.gz > $$f; \
		time $(EXEC) ../build/doctotext $$f > /dev/null; \
		rm $$f; \
	done
	@echo
	@echo TESTING LOGS REDIRECTION...
	@echo
	for e in odt ods odp rtf doc xls ppt docx xlsx pptx; do \
		$(EXEC) ../build/doctotext --verbose --log-file=log.txt 1.$$e > /dev/null; \
		if ! cat log.txt | tr -d '\r' | diff -u - 1.$$e.log; then \
			exit 1; \
		fi; \
	done
	rm log.txt
	@echo
	@echo TESTING LIBRARY C API...
	@echo
	if ! $(EXEC) ./c_api_test 1.odt | tr -d '\r' | diff -u - 1.odt.out; then \
		exit 1; \
	fi
	@echo
	@echo TESTING LIBRARY C++ API...
	@echo
	if ! $(EXEC) ./cpp_api_test 1.odt | tr -d '\r' | diff -u - 1.odt.out; then \
		exit 1; \
	fi
	@echo
	@echo CONVERTER TESTS COMPLETE.
	@echo

update_tests:
	@echo
	@echo UPDATING CONVERTER TESTS...
	@echo
	@for f in *.odt; do \
		f=`basename $$f .odt`; \
		for e in odt ods odp rtf doc xls ppt docx xlsx pptx; do \
			echo PROCESSING $$f.$$e; \
			$(EXEC) ../build/doctotext $$f.$$e > $$f.$$e.out; \
		done; \
	done
	@for f in hebrew_*.rtf japanese.xls embedded_spreadsheet.doc encoding_in_table.doc crash_test.xls special_para_cmds.rtf \
			header_footer.doc; do \
		echo PROCESSING $$f; \
		$(EXEC) ../build/doctotext $$f > $$f.out; \
	done
	for e in odt ods odp rtf doc xls ppt docx xlsx pptx; do \
		$(EXEC) ../build/doctotext --verbose --log-file=1.$$e.log 1.$$e > /dev/null; \
	done
	@echo
	@echo TESTS UPDATED.
	@echo

c_api_test$(EXEEXT): c_api_test.c ../build/doctotext_c_api.h ../build/$(SOPRE)doctotext$(SOEXT)
	$(LIB_PATH) $(CC) -o c_api_test$(EXEEXT) c_api_test.c -L../build/ -ldoctotext $(ARCH_FLAGS)
	$(STRIP) c_api_test$(EXEEXT)

ifeq ($(WIN32),1)
LDFLAGS = -static-libstdc++
else
LDFLAGS =
endif

cpp_api_test$(EXEEXT): cpp_api_test.cpp ../build/plain_text_extractor.h ../build/formatting_style.h ../build/$(SOPRE)doctotext$(SOEXT)
	$(LIB_PATH) $(CXX) -o cpp_api_test$(EXEEXT) cpp_api_test.cpp -L../build/ -ldoctotext $(LDFLAGS) $(ARCH_FLAGS)
	$(STRIP) cpp_api_test$(EXEEXT)

clean:
	rm -f c_api_test c_api_test.exe cpp_api_test cpp_api_test.exe
