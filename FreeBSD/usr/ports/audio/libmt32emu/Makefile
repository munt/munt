# New ports collection makefile for:	libmt32emu
# Date created:		2016-10-23
# Whom:			sergm <sergm@muntemu.org>
#
# $FreeBSD$
#

PORTNAME=	libmt32emu
PORTVERSION=	2.5.1
CATEGORIES=	audio
MASTER_SITES=	https://github.com/munt/munt/archive/
FETCH_ARGS=	-a
DISTFILES=	libmt32emu_2_5_1.tar.gz
WRKSRC=		$(WRKDIR)/munt-libmt32emu_2_5_1/mt32emu
LICENSE=	LGPL21+
LICENSE_FILE=	${WRKSRC}/COPYING.LESSER.txt

MAINTAINER=	sergm@muntemu.org
COMMENT=	C/C++ library to emulate (approximately) the Roland MT-32, CM-32L and LAPC-I synthesiser modules

USE_LDCONFIG=	yes
USES=		cmake

CMAKE_ON=	libmt32emu_WITH_SYMBOL_VERSIONING libmt32emu_WITH_VERSION_TAGGING
CMAKE_ARGS=	-Dlibmt32emu_PKGCONFIG_INSTALL_PREFIX:PATH=libdata

PLIST_FILES=	lib/libmt32emu.so \
		lib/libmt32emu.so.2 \
		lib/libmt32emu.so.2.5.1 \
		libdata/pkgconfig/mt32emu.pc \
		include/mt32emu.h \
		include/mt32emu/mt32emu.h \
		include/mt32emu/config.h \
		include/mt32emu/globals.h \
		include/mt32emu/Enumerations.h \
		include/mt32emu/File.h \
		include/mt32emu/FileStream.h \
		include/mt32emu/MidiStreamParser.h \
		include/mt32emu/ROMInfo.h \
		include/mt32emu/SampleRateConverter.h \
		include/mt32emu/Synth.h \
		include/mt32emu/Types.h \
		include/mt32emu/VersionTagging.h \
		include/mt32emu/c_interface/c_interface.h \
		include/mt32emu/c_interface/c_types.h \
		include/mt32emu/c_interface/cpp_interface.h \
		share/doc/munt/libmt32emu/AUTHORS.txt \
		share/doc/munt/libmt32emu/COPYING.txt \
		share/doc/munt/libmt32emu/COPYING.LESSER.txt \
		share/doc/munt/libmt32emu/NEWS.txt \
		share/doc/munt/libmt32emu/README.md \
		share/doc/munt/libmt32emu/TODO.txt

PLIST_DIRS=	include/mt32emu/c_interface \
		include/mt32emu \
		share/doc/munt/libmt32emu \
		share/doc/munt

.include <bsd.port.mk>
