# New ports collection makefile for:	mt32emu-qt
# Date created:		2016-10-23
# Whom:			sergm <sergm@muntemu.org>
#
# $FreeBSD$
#

PORTNAME=	mt32emu-qt
PORTVERSION=	1.9.0
CATEGORIES=	audio
MASTER_SITES=	https://github.com/munt/munt/archive/
FETCH_ARGS=	-a
DISTFILES=	mt32emu_qt_1_9_0.tar.gz
WRKSRC=		$(WRKDIR)/munt-mt32emu_qt_1_9_0/mt32emu_qt
LICENSE=        GPLv3+
LICENSE_FILE=   ${WRKSRC}/COPYING.txt

MAINTAINER=	sergm@muntemu.org
COMMENT=	Main MT-32 emulation application using libmt32emu

USES=		cmake qt:5
USE_QT=		widgets qmake_build buildtools_build

LIB_DEPENDS=	libportaudio.so.2:audio/portaudio \
		libjack.so.0:audio/jack \
		libmt32emu.so.2:audio/libmt32emu

PLIST_FILES=	bin/mt32emu-qt \
		share/applications/mt32emu-qt.desktop \
		share/icons/hicolor/16x16/apps/munt.png \
		share/icons/hicolor/24x24/apps/munt.png \
		share/icons/hicolor/32x32/apps/munt.png \
		share/icons/hicolor/48x48/apps/munt.png \
		share/icons/hicolor/64x64/apps/munt.png \
		share/icons/hicolor/72x72/apps/munt.png \
		share/icons/hicolor/96x96/apps/munt.png \
		share/icons/hicolor/128x128/apps/munt.png \
		share/icons/hicolor/256x256/apps/munt.png \
		share/doc/munt/mt32emu-qt/AUTHORS.txt \
		share/doc/munt/mt32emu-qt/COPYING.txt \
		share/doc/munt/mt32emu-qt/NEWS.txt \
		share/doc/munt/mt32emu-qt/README.md \
		share/doc/munt/mt32emu-qt/TODO.txt

PLIST_DIRS=	share/doc/munt/mt32emu-qt \
		share/doc/munt

.include <bsd.port.mk>
