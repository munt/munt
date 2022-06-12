# Patches for DOSBox to add mt32emu MIDI device

- [dosbox-0.74-3-mt32-patch.diff](https://github.com/munt/munt/blob/master/DOSBox-mt32-patch/dosbox-0.74-3-mt32-patch.diff) -
diff file to be applied to official DOSBox release v.0.74-3 source distribution.

- [dosbox-SVN-r4479-mt32-patch.diff](https://github.com/munt/munt/blob/master/DOSBox-mt32-patch/dosbox-SVN-r4479-mt32-patch.diff) -
diff file to be applied to official DOSBox sources SVN r4479 (and up, hopefully).
It uses a bit different and clear approach introduced since SVN r3836.

# How to build a patched version of DOSBox

This describes the steps necessary to produce a DOSBox binary with built-in MT-32 emulation using the GNU toolchain.
Note, builds with Microsoft Visual Studio or Xcode are not covered here. On Windows, MSYS can be used.

1. Ensure that the _mt32emu_ library is built and installed in the system. Typically, the library headers should appear
   under the `/usr/local/include` directory and the library binary itself should be in `/usr/local/lib`. Usual steps

       cd <munt source directory>/mt32emu
       cmake -DCMAKE_BUILD_TYPE:STRING=Release .
       make
       [sudo] make install

   should do the job. Note, this sequence will produce a shared library that will be required further on for DOSBox to run.
   In order to link the _mt32emu_ library statically, add option `-Dlibmt32emu_SHARED:BOOL=OFF` to the `cmake` command.
   Additionally, option `CMAKE_INSTALL_PREFIX` and/or `DESTDIR` variable can be used to adjust the installation directory.

2. Apply the patch file that corresponds to the DOSBox version being compiled, like this:

       cd <DOSBox source directory>
       patch -p1 < <munt source directory>/DOSBox-mt32-patch/dosbox-0.74-3-mt32-patch.diff

3. Proceed with `autogen.sh` and `configure`, as is normally done to build DOSBox from sources.
   In case the _mt32emu_ library was installed into a non-system directory in step #1, a couple of extra parameters should be
   added when running the `configure` script:

       ./configure CPPFLAGS=-I<path-to-mt32emu-include-dir> LIBS=-L<path-to-mt32emu-library-dir>

   For example, if the _mt32emu_ library was installed with `DESTDIR=/tmp/mt32emu/stage` and `CMAKE_INSTALL_PREFIX=/usr`, then:

       ./configure CPPFLAGS=-I/tmp/mt32emu/stage/usr/include LIBS=-L/tmp/mt32emu/stage/usr/lib

   will configure the DOSBox build process properly.

4. Adjust DOSBox configuration to pick up the ROM files and fine-tune mt32emu settings.
   To get the complete list of supported configuration options, generate a new DOSBox configuration file, e.g. with command

       config -writeconf dosbox-mt32.conf

   The new configuration file `dosbox-mt32.conf` also contains descriptions of the related options in the **midi** section and valid value ranges.
