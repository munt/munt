# New ports collection makefile for:	mt32emu-smf2wav
# Date created:		2016-10-23
# Whom:			sergm <sergm@muntemu.org>
#
# $FreeBSD$
#

PORTNAME=	mt32emu-smf2wav
PORTVERSION=	1.6.0
CATEGORIES=	audio
MASTER_SITES=	https://github.com/munt/munt/archive/
FETCH_ARGS=	-a
DISTFILES=	mt32emu_smf2wav_1_6_0.tar.gz
WRKSRC=		$(WRKDIR)/munt-mt32emu_smf2wav_1_6_0/mt32emu_smf2wav
LICENSE=	GPLv3+
LICENSE_FILE=	${WRKSRC}/COPYING.txt

MAINTAINER=	sergm@muntemu.org
COMMENT=	Experimental program for converting SMF (Standard MIDI File) to WAVE via libmt32emu

USES=		cmake

USE_GNOME=	glib20

LIB_DEPENDS=    libmt32emu.so.2:audio/libmt32emu

PLIST_FILES=	bin/mt32emu-smf2wav \
		share/doc/munt/smf2wav/AUTHORS.txt \
		share/doc/munt/smf2wav/COPYING.txt \
		share/doc/munt/smf2wav/README.md

PLIST_DIRS=	share/doc/munt/smf2wav \
		share/doc/munt

.include <bsd.port.mk>
