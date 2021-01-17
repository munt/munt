# New ports collection makefile for:	mt32emu-qt
# Date created:		2016-10-23
# Whom:			sergm <sergm@muntemu.org>
#
# $FreeBSD$
#

PORTNAME=	mt32emu-qt
PORTVERSION=	1.8.0
CATEGORIES=	audio
MASTER_SITES=	https://github.com/munt/munt/archive/
FETCH_ARGS=	-a
DISTFILES=	mt32emu_qt_1_8_0.tar.gz
WRKSRC=		$(WRKDIR)/munt-mt32emu_qt_1_8_0/mt32emu_qt
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
		share/doc/munt/mt32emu-qt/AUTHORS.txt \
		share/doc/munt/mt32emu-qt/COPYING.txt \
		share/doc/munt/mt32emu-qt/NEWS.txt \
		share/doc/munt/mt32emu-qt/README.md \
		share/doc/munt/mt32emu-qt/TODO.txt

PLIST_DIRS=	share/doc/munt/mt32emu-qt \
		share/doc/munt

.include <bsd.port.mk>
