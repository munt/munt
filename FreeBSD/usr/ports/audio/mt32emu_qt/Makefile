# New ports collection makefile for:	mt32emu-qt
# Date created:		2016-10-23
# Whom:			sergm <sergm@muntemu.org>
#
# $FreeBSD$
#

PORTNAME=	mt32emu-qt
PORTVERSION=	1.4.0
CATEGORIES=	audio
MASTER_SITES=	https://github.com/munt/munt/archive/
FETCH_ARGS=	-a
DISTFILES=	munt_2_0_0.tar.gz

MAINTAINER=	sergm@muntemu.org
COMMENT=	Main MT-32 emulation application using libmt32emu

USES=		cmake
#USE_QT4=	gui imageformats qmake_build moc_build rcc_build uic_build
USE_QT5=	widgets qmake_build buildtools_build

LIB_DEPENDS=	libportaudio.so.2:${PORTSDIR}/audio/portaudio libmt32emu.so.2:${PORTSDIR}/audio/libmt32emu

PLIST_FILES=	bin/mt32emu-qt \
		share/doc/munt/mt32emu-qt/AUTHORS.txt \
		share/doc/munt/mt32emu-qt/COPYING.txt \
		share/doc/munt/mt32emu-qt/NEWS.txt \
		share/doc/munt/mt32emu-qt/README.txt \
		share/doc/munt/mt32emu-qt/TODO.txt

PLIST_DIRS=	share/doc/munt/mt32emu-qt \
		share/doc/munt

CMAKE_BUILD_TYPE?=	Release

post-extract:
		mkdir $(WRKSRC)

do-configure:
		cd $(WRKSRC); \
		cmake -D CMAKE_BUILD_TYPE:STRING=$(CMAKE_BUILD_TYPE) \
			-D mt32emu-qt_WITH_ALSA_MIDI_SEQUENCER:BOOL=OFF $(WRKDIR)/munt-munt_2_0_0/mt32emu_qt

.include <bsd.port.mk>
