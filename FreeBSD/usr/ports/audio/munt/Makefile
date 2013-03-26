# New ports collection makefile for:	munt
# Date created:		2012-12-25
# Whom:			sergm <sergm@muntemu.org>
#
# $FreeBSD$
#

PORTNAME=	munt
PORTVERSION=	1.2.0
CATEGORIES=	audio
MASTER_SITES=	https://github.com/munt/munt/archive/
FETCH_ARGS=	-a
DISTFILES=	munt_1_2_0.tar.gz

MAINTAINER=	sergm@muntemu.org
COMMENT=	Software synthesiser emulating pre-GM MIDI devices such as Roland MT-32, CM-32L, CM-64 and LAPC-I

USE_CMAKE=	yes
USE_QT4=	gui imageformats qmake_build moc_build rcc_build uic_build

PLIST_FILES=	bin/mt32emu-smf2wav \
		bin/mt32emu-qt \
		lib/libmt32emu.a \
		include/mt32emu/File.h \
		include/mt32emu/FileStream.h \
		include/mt32emu/LA32Ramp.h \
		include/mt32emu/Part.h \
		include/mt32emu/Partial.h \
		include/mt32emu/Poly.h \
		include/mt32emu/ROMInfo.h \
		include/mt32emu/Structures.h \
		include/mt32emu/Synth.h \
		include/mt32emu/Tables.h \
		include/mt32emu/TVA.h \
		include/mt32emu/TVF.h \
		include/mt32emu/TVP.h \
		include/mt32emu/mt32emu.h
PLIST_DIRS=	include/mt32emu

CMAKE_BUILD_TYPE?=	Release

post-extract:
		mv $(WRKDIR)/munt-munt_1_2_0 $(WRKSRC)

do-configure:
		cd $(WRKSRC); \
		cmake -D CMAKE_BUILD_TYPE:STRING=$(CMAKE_BUILD_TYPE) .

post-install:
		${STRIP_CMD} ${PREFIX}/bin/mt32emu-smf2wav
		${STRIP_CMD} ${PREFIX}/bin/mt32emu-qt

.include <bsd.port.mk>
