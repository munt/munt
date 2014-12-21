# New ports collection makefile for:	munt
# Date created:		2012-12-25
# Whom:			sergm <sergm@muntemu.org>
#
# $FreeBSD$
#

PORTNAME=	munt
PORTVERSION=	1.5.0
CATEGORIES=	audio
MASTER_SITES=	https://github.com/munt/munt/archive/
FETCH_ARGS=	-a
DISTFILES=	munt_1_5_0.tar.gz

MAINTAINER=	sergm@muntemu.org
COMMENT=	Software synthesiser emulating pre-GM MIDI devices such as Roland MT-32, CM-32L, CM-64 and LAPC-I

USES=		cmake
USE_QT4=	gui imageformats qmake_build moc_build rcc_build uic_build

LIB_DEPENDS=	libportaudio.so.2:${PORTSDIR}/audio/portaudio2 libsamplerate.so.0:${PORTSDIR}/audio/libsamplerate

PLIST_FILES=	bin/mt32emu-smf2wav \
		bin/mt32emu-qt \
		lib/libmt32emu.a \
		include/mt32emu/File.h \
		include/mt32emu/FileStream.h \
		include/mt32emu/MidiStreamParser.h \
		include/mt32emu/ROMInfo.h \
		include/mt32emu/Synth.h \
		include/mt32emu/Types.h \
		include/mt32emu/mt32emu.h \
		share/doc/munt/libmt32emu/AUTHORS.txt \
		share/doc/munt/libmt32emu/COPYING.txt \
		share/doc/munt/libmt32emu/COPYING.LESSER.txt \
		share/doc/munt/libmt32emu/NEWS.txt \
		share/doc/munt/libmt32emu/README.txt \
		share/doc/munt/libmt32emu/TODO.txt \
		share/doc/munt/smf2wav/AUTHORS.txt \
		share/doc/munt/smf2wav/COPYING.txt \
		share/doc/munt/smf2wav/README.txt \
		share/doc/munt/mt32emu-qt/AUTHORS.txt \
		share/doc/munt/mt32emu-qt/COPYING.txt \
		share/doc/munt/mt32emu-qt/NEWS.txt \
		share/doc/munt/mt32emu-qt/README.txt \
		share/doc/munt/mt32emu-qt/TODO.txt

PLIST_DIRS=	include/mt32emu \
		share/doc/munt/libmt32emu \
		share/doc/munt/smf2wav \
		share/doc/munt/mt32emu-qt \
		share/doc/munt

CMAKE_BUILD_TYPE?=	Release

post-extract:
		mv $(WRKDIR)/munt-munt_1_5_0 $(WRKSRC)

do-configure:
		cd $(WRKSRC); \
		cmake -D CMAKE_BUILD_TYPE:STRING=$(CMAKE_BUILD_TYPE) \
			-D PORTAUDIO_INCLUDE_DIR:PATH=/usr/local/include/portaudio2 \
			-D PORTAUDIO_LIBRARY:FILEPATH=/usr/local/lib/portaudio2/libportaudio.so.2 \
			-D mt32emu-qt_WITH_ALSA_MIDI_DRIVER:BOOL=OFF .

.include <bsd.port.mk>
