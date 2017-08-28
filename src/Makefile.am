AUTOMAKE_OPTIONS = subdir-objects

AM_CXXFLAGS = @PAUKN_CFLAGS@ @VST_CFLAGS@ -I../include
libpauknincludedir = $(includedir)
libpaukninclude_HEADERS = ../include/paukn.h

lib_LTLIBRARIES = libpaukn.la
libpaukn_la_SOURCES = dwgs.cpp filter.cpp

paukn_LDADD = libpaukn.la @VST_LIBS@
paukn_LDFLAGS = $(os_ldflags) -bundle
paukn_SOURCES = midi.cpp paukn.cpp vsteffect.cpp @VSTSDK_DIR@/audioeffect.cpp @VSTSDK_DIR@/audioeffect.h @VSTSDK_DIR@/audioeffectx.cpp @VSTSDK_DIR@/audioeffectx.h @VSTSDK_DIR@/aeffeditor.h @VSTPLUG_DIR@/aeffect.h @VSTPLUG_DIR@/aeffectx.h @VSTSDK_DIR@/vstplugmain.cpp

bin_PROGRAMS = paukn

clean: cleanapproot clean-am

cleanapproot:
	rm -rf $(top_builddir)/approot

all: $(top_builddir)/approot/Library/Audio/Plug-Ins/VST/paukn.vst $(top_builddir)/mac/vst/Info.plist $(top_builddir)/mac/vst/PkgInfo

$(top_builddir)/approot/Library/Audio/Plug-Ins/VST/paukn.vst: paukn
	mkdir -p $(top_builddir)/approot/Library/Audio/Plug-Ins/VST/paukn.vst/Contents/MacOS
	cp -f paukn $(top_builddir)/approot/Library/Audio/Plug-Ins/VST/paukn.vst/Contents/MacOS/paukn
	cp -f $(top_builddir)/mac/vst/Info.plist $(top_builddir)/approot/Library/Audio/Plug-Ins/VST/paukn.vst/Contents
	cp -f $(top_builddir)/mac/vst/PkgInfo $(top_builddir)/approot/Library/Audio/Plug-Ins/VST/paukn.vst/Contents
	mkdir -p $(top_builddir)/approot/Library/Audio/Plug-Ins/VST/paukn.vst/Contents/Resources

