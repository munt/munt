MT32Emu WinMM Driver
********************************

First-time Installation
-----------------------

Run "drvsetup.exe install" to perform installation. This will copy mt32emu.dll into
C:\WINDOWS\SYSTEM32 (C:\WINDOWS\SYSWOW64 for x64 systems) directory.
Use the main UI-enabled application to configure driver settings.

--------------------------------------------------------------------------------

You can also use the fail-safe approach "Add Hardware wizard" if you like.
The information file mt32emu.inf is provided. However, this way works
on x86 Windows only.

 1) Unpack mt32emu.inf and mt32emu.dll. Remember the location of this directory.
 2) Open Control Panel.
 3) Double-click on "Add Hardware".
 4) Click "Next" until you come to a message asking you whether you have
    already installed the hardware.
 5) Select the "Yes" option and click "Next".
 6) A list of installed hardware will appear. Scroll to the bottom of the list
    and select the last entry, which should be something like "New Hardware".
    Click "Next".
 7) Select "Choose hardware manually from a list" and click "Next".
 8) Select "Sound, Video and Game Controllers" and click "Next".
 9) Click "Have Disk...".
10) In the window that pops up, click "Browse..." and choose the directory to
    which you unpacked the mt32emu.inf and mt32emu.dll files. Click "OK".
11) If a window pops up complaining about the lack of Windows Logo testing,
    click "Install Anyway" or similar.
12) "MT-32 Synth Emulator" should have appeared selected in a list.
    Click "Next" twice.
13) The driver *still* isn't Windows Logo tested, so click "Install Anyway" if
    necessary.
14) The driver should now have been installed; click "Finish".
15) A dialog box will recommend that you reboot. Go ahead if you enjoy that
    sort of thing, but it shouldn't be necessary for a fresh installation.

--------------------------------------------------------------------------------

To begin playing back MIDI through the emulator, you need to configure
the corresponding MIDI device in your MIDI application. In order to set
the emulator as the default MIDI device in the system you may use Control
Panel or a MIDI switcher like Putzlowitschs Vista-MIDIMapper.

The driver tries to find out whether the main UI-enabled application is running, and if
it is, the driver directs all the incoming MIDI messages to the main application
for processing. If the application is not running, the driver operates in self-contained
mode. In the latter case, the configuration you set using the main UI-enabled application
will also apply to the internal synth engine where relevant.

If you start the main synth application and / or change the emulation parameters you need
to either restart MIDI playback or reopen your MIDI application for the new
parameters to take effect and for the driver to make an attempt to communicate
with the main application.


Upgrading
---------

Run "drvsetup.exe install" to upgrade the driver. The configuration you set using
the main UI-enabled application will apply to the internal synth engine as well.

                             OR

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
14) You almost certainly *will* need to reboot the computer for the change to take effect.


Uninstalling
------------

Run "drvsetup.exe uninstall" to remove the driver. This will not affect any configuration
variables set using the main UI-enabled application.


License
-------

Copyright (C) 2003, 2004, 2005, 2011 Dean Beeler, Jerome Fisher
Copyright (C) 2011-2017 Dean Beeler, Jerome Fisher, Sergey V. Mikayev

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


Trademark disclaimer
--------------------

Roland is a trademark of Roland Corp. All other brand and product names are
trademarks or registered trademarks of their respective holder. Use of
trademarks is for informational purposes only and does not imply endorsement by
or affiliation with the holder.
