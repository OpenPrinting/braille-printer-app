ACLOCAL_AMFLAGS = -I m4

doc_DATA = \
	ABOUT-NLS \
	AUTHORS \
	COPYING \
	CHANGES.md \
	CHANGES-1.x.md \
	INSTALL \
	README.md \
	DEVELOPING.md \
	CONTRIBUTING.md

EXTRA_DIST = \
	$(doc_DATA) \
	autogen.sh \
	config.rpath \
	filter/braille/drivers/index/ubrlto4dot.c \
	filter/braille/filters/TODO.txt

# =========
# utilities
# =========

pkgutilsdir = $(bindir)
pkgutils_PROGRAMS =

# ========
# Backends
# ========
pkgbackenddir = $(CUPS_SERVERBIN)/backend
pkgbackend_PROGRAMS =

if ENABLE_BRAILLE
pkgbackend_PROGRAMS += cups-brf
endif

cups_brf_SOURCES = \
	backend/cups-brf.c

# =======
# Filters
# =======

pkgfilter_PROGRAMS =
pkgfilterdir = $(CUPS_SERVERBIN)/filter

# =======
# Drivers
# =======
pkgdriverdir = $(CUPS_DATADIR)/drv

pkgdriver_DATA =

brldrvfiles = \
	drv/generic-brf.drv \
	drv/generic-ubrl.drv \
	drv/indexv3.drv \
	drv/indexv4.drv
if ENABLE_BRAILLE
pkgdriver_DATA += $(brldrvfiles)
endif

EXTRA_DIST += $(brldrvfiles)

# =======
# Definitions for drivers
# =======
pkgppdcdir = $(CUPS_DATADIR)/ppdc

pkgppdc_DATA =

GENERATED_LIBLOUIS = \
	filter/braille/filters/liblouis3.defs \
	filter/braille/filters/liblouis4.defs
GENERATED_DEFS = \
	filter/braille/filters/liblouis1.defs \
	filter/braille/filters/liblouis2.defs \
	$(GENERATED_LIBLOUIS)

filter/braille/filters/liblouis1.defs: filter/braille/filters/liblouis1.defs.gen
	$< > $@

filter/braille/filters/liblouis2.defs: filter/braille/filters/liblouis1.defs
	sed -e "s/Braille transcription/Additional Braille transcription (2)/" \
	    -e "s/^  \\*Choice /  Choice /" \
	    -e "s/^  Choice \"HyphLocale\//  *Choice \"HyphLocale\//" \
	    -e s/LibLouis/LibLouis2/ \
	    < $< > $@

$(GENERATED_LIBLOUIS): filter/braille/filters/liblouis%.defs: filter/braille/filters/liblouis1.defs
	sed -e "s/Braille transcription/Additional Braille transcription ($*)/" \
	    -e "s/^  \\*Choice /  Choice /" \
	    -e "s/^  Choice \"None\//  *Choice \"None\//" \
	    -e s/LibLouis/LibLouis$*/ \
	    < $< > $@

brlppdcfiles = \
	filter/braille/drivers/common/media-braille.defs \
	filter/braille/drivers/index/index.defs \
	filter/braille/filters/braille.defs \
	filter/braille/filters/imagemagick.defs \
	filter/braille/filters/liblouis.defs \
	filter/braille/drivers/common/fr-braille.po

if ENABLE_BRAILLE
pkgppdc_DATA += $(brlppdcfiles)
nodist_pkgppdc_DATA = \
	$(GENERATED_DEFS)
endif

EXTRA_DIST += \
	filter/braille/filters/liblouis1.defs.gen.in \
	$(brlppdcfiles)

# =====
# MIMEs
# =====
pkgmimedir = $(CUPS_DATADIR)/mime

pkgmime_DATA =

brlmimefiles = \
	mime/braille.convs \
	mime/braille.types
if ENABLE_BRAILLE
pkgmime_DATA += $(brlmimefiles)
endif

EXTRA_DIST += \
	$(brlmimefiles)

# =================
# Braille aux files
# =================
if ENABLE_BRAILLE
pkgbrailledir = $(CUPS_DATADIR)/braille
nodist_pkgbraille_SCRIPTS = \
	filter/braille/drivers/index/indexv4.sh \
	filter/braille/drivers/index/indexv3.sh \
	filter/braille/drivers/index/index.sh \
	filter/braille/filters/cups-braille.sh
endif

# ======================
# Simple filter binaries
# ======================
genfilterscripts =
pkgfilter_SCRIPTS = $(genfilterscripts)
if ENABLE_BRAILLE
nodist_pkgfilter_SCRIPTS = \
	filter/braille/drivers/generic/brftoembosser \
	filter/braille/drivers/index/imageubrltoindexv3 \
	filter/braille/drivers/index/imageubrltoindexv4 \
	filter/braille/drivers/index/textbrftoindexv3 \
	filter/braille/filters/imagetobrf \
	filter/braille/filters/vectortopdf \
	filter/braille/filters/vectortobrf \
	filter/braille/filters/texttobrf \
	filter/braille/filters/brftopagedbrf \
	filter/braille/filters/musicxmltobrf
endif

distclean-local:
	rm -rf *.cache *~

install-exec-hook:
	$(INSTALL) -d -m 755 $(DESTDIR)$(pkgfilterdir)
	$(INSTALL) -d -m 755 $(DESTDIR)$(pkgbackenddir)
if ENABLE_BRAILLE
	$(LN_S) -f imagetobrf $(DESTDIR)$(pkgfilterdir)/imagetoubrl
	$(LN_S) -f vectortopdf $(DESTDIR)$(pkgfilterdir)/svgtopdf
	$(LN_S) -f vectortopdf $(DESTDIR)$(pkgfilterdir)/xfigtopdf
	$(LN_S) -f vectortopdf $(DESTDIR)$(pkgfilterdir)/wmftopdf
	$(LN_S) -f vectortopdf $(DESTDIR)$(pkgfilterdir)/emftopdf
	$(LN_S) -f vectortopdf $(DESTDIR)$(pkgfilterdir)/cgmtopdf
	$(LN_S) -f vectortopdf $(DESTDIR)$(pkgfilterdir)/cmxtopdf
	$(LN_S) -f vectortobrf $(DESTDIR)$(pkgfilterdir)/vectortoubrl
	$(LN_S) -f textbrftoindexv3 $(DESTDIR)$(pkgfilterdir)/textbrftoindexv4
endif

install-data-hook:
if ENABLE_BRAILLE
	chmod 700 $(DESTDIR)/$(pkgbackenddir)/cups-brf
endif


uninstall-hook:
if ENABLE_BRAILLE
	$(RM) $(DESTDIR)$(pkgfilterdir)/imagetoubrl
	$(RM) $(DESTDIR)$(pkgfilterdir)/svgtopdf
	$(RM) $(DESTDIR)$(pkgfilterdir)/xfigtopdf
	$(RM) $(DESTDIR)$(pkgfilterdir)/wmftopdf
	$(RM) $(DESTDIR)$(pkgfilterdir)/emftopdf
	$(RM) $(DESTDIR)$(pkgfilterdir)/cgmtopdf
	$(RM) $(DESTDIR)$(pkgfilterdir)/cmxtopdf
	$(RM) $(DESTDIR)$(pkgfilterdir)/vectortoubrl
	$(RM) $(DESTDIR)$(pkgfilterdir)/textbrftoindexv4
endif

SUBDIRS =
