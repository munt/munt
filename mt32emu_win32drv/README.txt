MT32Emu WinMM Driver
********************************

First-time Installation
-----------------------

Run "drvsetup.exe install" to perform installation. This will copy mt32emu.dll
into C:\WINDOWS\SYSTEM32 (and C:\WINDOWS\SYSWOW64 on x64 systems) directory
and register the virtual synth MIDI device in the Plug-n-Play manager.
Use the main UI-enabled application to configure driver settings.

--------------------------------------------------------------------------------

You can also use the fail-safe approach "Add Hardware wizard" if you like.
The information file mt32emu.inf is provided. However, this way may appear
significantly longer. To make it easier, there is a helper tool infinstall,
that performs the steps 3-10 described below in a single run.

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

Alternatively, after the step #2, simply run "infinstall.exe install"
(on 32-bit systems) or "infinstall_x64.exe install" (on 64-bit systems)
and confirm the installation when asked.

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

The "infinstall.exe install" command can also be used to update the driver as a shortcut
instead of going through the above steps.


Troubleshooting
---------------

Unfortunately, Windows MIDI applications keep relying on the old WinMME API,
which isn't flexible enough for the today's demand. For instance, a total of 10
MIDI drivers can be recognised by a WinMME MIDI application. As a result, MIDI
drivers installed in the system (and the Windows itself) may "fight" with each
other for the available legacy MIDI slots where a driver can be registered.

Sometimes, it happens that Windows assigns all the available slots to kernel
mode drivers, for example due to attaching a USB MIDI cable, etc, so that
user-mode drivers like mt32emu appear out of luck. In this case, the driver
is no longer recognised by MIDI applications, although remains in the system.
To recover from that, the driver can be re-installed, or in a quicker way,
command "drvsetup.exe repair" can be executed.


Uninstalling
------------

Run "drvsetup.exe uninstall" to remove the driver. This will not affect any configuration
variables set using the main UI-enabled application.


License
-------

Copyright (C) 2003, 2004, 2005, 2011 Dean Beeler, Jerome Fisher
Copyright (C) 2011-2020 Dean Beeler, Jerome Fisher, Sergey V. Mikayev

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
