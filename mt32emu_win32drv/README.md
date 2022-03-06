# MT32Emu WinMM Driver

## Driver installation, update and removal

### drvsetup - a quick driver maintenance tool

Run `drvsetup.exe install` to perform first-time installation or upgrade.
This will copy `mt32emu.dll` into `C:\WINDOWS\SYSTEM32` (and `C:\WINDOWS\SYSWOW64`
on x64 systems) directory and register the virtual synth MIDI device
in the Plug-n-Play manager. Use the main UI-enabled application to configure
driver settings.

_NOTE_: Despite working fast, the `drvsetup.exe` tool does not ensure that
the virtual synth MIDI device is registered in the Plug-n-Play manager completely.
This manifests itself in missing certain driver properties when looking at the device
entry in the Windows Device Management. However, this tool attempts to carefully
resolve possible conflicts between installed MIDI drivers, in cases when the "official"
Windows class installer gives up leaving the device registered in the system
__but not actually working__ because of a failure to create the usermode MIDI driver alias.
Therefore, using the `drvsetup.exe` tool is the recommended way for driver maintenance.

To uninstall the driver just run `drvsetup.exe uninstall`.

In case the driver suddenly stops working (possibly due to conflicts with the other MIDI
drivers installed in the system), the command `drvsetup.exe repair` can be used.
Additionally, this command helps in case the full-blown installation fails to create
the usermode MIDI driver alias due to conflicts. See the [Troubleshooting](#Troubleshooting)
section below for more details.

### infinstall - an automated driver maintenance tool using the INF file

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

In contrast to `drvsetup.exe`, there are two binaries `infinstall.exe` and `infinstall_x64.exe`
for 32-bit and 64-bit systems respectively, which is due to the limitations of the Windows
device class installer.

Use the command `infinstall.exe install -sign` to install or update the driver.
During the process, a temporary self-signed certificate will be generated and installed
to the system trust store in case the system policy permits. If you want to avoid
this step, omit the `-sign` flag and simply run `infinstall.exe install`, although
to succeed, this requires preparation as described in step #1 of the section below.

To uninstall the driver, the command `infinstall.exe uninstall` can be used.

### Manual driver maintenance using standard Hardware wizard

You can also use the fail-safe approach "Add Hardware wizard" if you like.
The information file mt32emu.inf is provided and works on 32-bit and 64-bit systems.

#### First-time installation

 1) Disable the Windows driver signing requirement which is enforced in Vista
    and above. There are various ways to accomplish it, the simplest seems
    to be as follows.
    - Reboot the system into the recovery mode. For this, on Windows 10, right-
      click the Start Menu button (or press Windows-X) and Shift-click
      the Restart menu item.
    - Go to Troubleshoot / Advanced Options / Startup Settings and click
      the Restart button.
    - When prompted, press F7 key (or numeric 7 key) to boot into
      the "unsafe" mode.
 2) Unpack the distribution archive. Remember the location of this directory.
 3) On Windows XP, open the Control Panel and start the "Add Hardware" wizard.
    On later Windows versions, manually start "hdwwiz.exe" or "hdwwiz.cpl" from
    the Run dialog or from the command line.
 4) If prompted, tell the wizard that the device is connected but it shouldn't
    try to search the driver via the Windows Update for it.
 5) Proceed to the selection of the hardware type. Select "Sound, video and
    game controllers".
 6) When a list of available drivers appears, click "Have Disk...".
 7) In the window that pops up, click "Browse..." and choose the directory to
    which you unpacked the mt32emu.inf and mt32emu.dll files. Click "OK".
 8) "MT-32 Synth Emulator" should have appeared selected in a list.
    Click "Next" twice.
 9) If a window pops up complaining about the lack of Windows Logo testing
    and/or digital signature, click "Install Anyway..." or similar.
10) The driver should now have been installed; click "Finish".
11) If a dialog box recommends that you reboot, go ahead if you enjoy that
    sort of thing, but it shouldn't be necessary for a fresh installation.

#### Upgrading

 1) Click on "Start", then "Run...".
 2) Type "devmgmt.msc" and press enter.
 3) Click on the "+" Next to "Sound, Video and Game Controllers".
 4) Right-click on "MT-32 Synth Emulator" and select "Update Driver".
 5) If asked whether you'd like to check Windows Update, select "No, not this time" and click "Next".
 6) Select "Select software from a list" and click "Next".
 7) Select "Don't search, let me choose from a list" and click "Next".
 8) Click "Browse...".
 9) Choose the address of the unzipped stuff and click "OK".
10) Click "Next".
11) Click "Continue installation" if asked about Windows Logo stuff.
12) If a dialog pops up asking for the ROM files, navigate *again* to the directory to show it where the ROMs are and click OK.
13) If asked about change in languages, confirm that you want the file replaced.
14) You likely need to reboot the computer for the change to take effect.

#### Uninstalling

 1) Click on "Start", then "Run...".
 2) Type "devmgmt.msc" and press enter.
 3) Click on the "+" Next to "Sound, Video and Game Controllers".
 4) Right-click on "MT-32 Synth Emulator" and select "Uninstall Device".
 5) In the next dialog, select checkbox "Delete the driver software for this device"
    in order to remove a copy of driver installation files from the system store.
 6) Confirm the action by clicking the "Uninstall" button.

## Driver operation

To begin playing back MIDI through the emulator, you need to configure
the corresponding MIDI device in your MIDI application. In order to set
the emulator as the default MIDI device in the system you may use Control
Panel or a MIDI switcher like
[Putzlowitschs Vista-MIDIMapper](https://putzlowitsch.de/2007/08/07/sichtwechsel/)
or the one from [CoolSoft](https://coolsoft.altervista.org/en/midimapper).

The driver tries to find out whether the main UI-enabled application is running, and if
it is, the driver directs all the incoming MIDI messages to the main application
for processing. If the application is not running, the driver operates in self-contained
mode. In the latter case, the configuration you set using the main UI-enabled application
will also apply to the internal synth engine where relevant.

If you start the main synth application and / or change the emulation parameters you need
to either restart MIDI playback or reopen your MIDI application for the new
parameters to take effect and for the driver to make an attempt to communicate
with the main application.

## Troubleshooting

Unfortunately, Windows MIDI applications keep relying on the old WinMME API,
which isn't flexible enough for the today's demand. For instance, a total of 10
usermode MIDI drivers can be recognised by a WinMME MIDI application. As a result,
MIDI drivers installed in the system (and the Windows itself) may "fight" with each
other for the available legacy MIDI slots where a driver can be registered.

Sometimes, it happens that Windows assigns all the available slots to kernel
mode drivers, for example due to attaching a USB MIDI cable, etc, so that
user-mode drivers like mt32emu appear out of luck. In this case, the driver
is no longer recognised by MIDI applications, although remains in the system.
To recover from that, the driver can be re-installed, or in a quicker way,
command `drvsetup.exe repair` can be executed.

## License

Copyright (C) 2003, 2004, 2005, 2011 Dean Beeler, Jerome Fisher<br>
Copyright (C) 2011-2022 Dean Beeler, Jerome Fisher, Sergey V. Mikayev

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

## Trademark disclaimer

Roland is a trademark of Roland Corp. All other brand and product names are
trademarks or registered trademarks of their respective holder. Use of
trademarks is for informational purposes only and does not imply endorsement by
or affiliation with the holder.
