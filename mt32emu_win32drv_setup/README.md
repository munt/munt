drvsetup
========
A helper tool capable of installing / upgrading the Windows driver _mt32emu_win32drv_ and registering
in the system (in some non-standard way which may fail occasionally).

infinstall
==========
Another helper tool `infinstall.exe` is intended to make simple the standard tedious way
of the _mt32emu_win32drv_ MIDI Windows driver installation using the `.inf` file.
It eliminates the need to use all those boring wizards intended for manual installation
of non-PnP devices on Windows yet provides an option to overcome the default Windows driver
signing policy to facilitate installation of unsigned user mode drivers.
For the latter to achieve, we use a part of [libwdi](https://github.com/pbatard/libwdi/)
library following an approach similar to the one described in
[libwdi wiki](https://github.com/pbatard/libwdi/wiki/Certification-Practice-Statement)
with a notable difference that the made up certificate is deleted instantly as soon as
the installation finishes and it isn't hanging around in the trusted system store.
