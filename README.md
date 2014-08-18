Subsurface Android Downloader
===============================

This application is meant to download data from divecomputers on android,
process the data and save the resulting xml file on the external storage.

It uses libdivecomputer library to communicate with divecomputer and subsurface
library for loading, processing and saving of dives. These saved dives can be
later be imported to subsurface application.
This application is a part of the overall plan to bring subsurface to android
and was undertaken as a part of Google Summer of Code '14.

>Application Developer	- Venkatesh Shukla	<venkatesh.shukla.eee11@iitbhu.ac.in>

>Project Mentor		- Anton Lundin		<glance@acc.umu.se>

>Organization		- Subsurface		https://subsurface.hohndel.org

###Dependencies

This project needs the following dependencies in order to be built.

1. Android SDK Tools - Make sure the SDK is installed and updated. It can be found
   [here](http://developer.android.com/sdk/installing/index.html?pkg=tools).
   Make sure sdk/platform-tools are in the path. This way adb can be used from
   anywhere.

2. Android NDK - Subsurface is a C/C++ based application and in order to cross
   compile it on android, we need the build tools of android ndk. Find the ndk
   [here](http://developer.android.com/tools/sdk/ndk/index.html).

3. Qt for Android - Subsurface cross compilation would require presence of Qt
   for android. Grab it from [here](http://qt-project.org/downloads).

4. Ant - For android application building and installation.

5. Make - For building purposes.

6. Git - Git submodules are required.

###Build Instructions

Follow these steps to build the project

1. Make sure the above mentioned dependencies are installed.

2. Edit the build.sh files to provide the path to android sdk, ndk and Qt
   directories.

3. On the terminal, navigate to the project directory and execute the following
   commands
   ```
   bash build.sh arch
   ```
   where the arch can be `arm` or `x86` as per your target architecture.

4. The script will download the following dependencies of subsurface and cross
   compile them.
   1. libsqlite3
   2. libz
   3. libzip
   4. libxslt
   5. libxml2
   6. libgit2
   7. libusb-1.0
   8. libftdi1
   9. libdivecomputer

5. After the above subsurface dependencies are made, subsurface is cross
   compiled without qt making a libsubsurface_jni.a library.

6. Using ndk-build the above mentioned library and JNI interface is compiled and
   android application is built.

7. If an emulator/device is attached, the application will be installed, run and
   its logs shown on the terminal.

Note - Steps 4 5 6 and 7 are automated by the script and not required to be
explicitly run.

###Libraries Used

The application, as it deals with USB on android, required some tweaking with
various libraries in order to work. These are listed below.

* [Libusb](http://www.libusb.org/) - tweaked for android

Libusb is an excellant library used for USB communication. To enable its
functioning on android, it needs some modifications. The modifications take care
of android USB permissions and give back libusb_device_handle when a int fd of
the USB device is passed. These tweaks can be found in the android-fd branch
[here](https://github.com/venkateshshukla/libusb).

* [Libftdi](http://www.intra2net.com/en/developer/libftdi/) - tweaked for android

For talking with FTDI devices, a sizeable chunk of which are based on FTDI
chips libftdi library is utilised. Under the hood, it uses libusb to talk with
USB attached FTDI devices. As we are using tweaked libusb which expects USB file
descriptor, libftdi also needs some modifications to pass the file descriptor.
These tweaks can be seen in android-fd branch 
[here] (https://github.com/venkateshshukla/libftdi)

* [Libdivecomputer](http://libdivecomputer.org/) - tweaked for android

Libdivecomputer is used for extraction of data from various divecomputers. It is
a cross platform library and utilises tty interface for communication with the
divecomputers. As tty devices are not present in android, in order to
communicate with divecomputers, we will need chipset specific scripts.
Implementation for ftdi based devices has been done. It uses libftdi and passes
USB file descriptor as needed by android. The tweaks can be seen in android
branch [here](https://github.com/venkateshshukla/libdiveoc0mputer).

* [Subsurface](http://subsurface.hohndel.org) - Built without Qt for android.

Subsurface is the core of this application. This application does only a very
minute subset of the overall subsurface functionality. It processes the data
extracted from divecomputers and gives a portable xml file which can be used
later on with desktop application. For building on android without qt and for
passing fd on android, certain modifications has been made. These tweaks can be
seen in android-jni branch [here](https://github.com/venkateshshukla/subsurface)

###Application Usage

This application is a very simple one and emulates the Subsurface download from
divecomputer functionality.

The following are the steps to download your dives on android using subsurface
android downloader.

a. Connect your divecomputer with the android using OTG cable. On attaching the
divecomputer, a popup appears asking for permission to use the device. Grant the
permission. It would also be favourable to tick the checkbox so that subsurface
android is opened always when a divecomputer is attached.

b. Choose the divecomputer vendor and product from the dropdown list. Also
choose the name to be given to the imported xml file.

Note - Please note that the application would first load the xml file before
downloading data from divecomputer. This is similar to Subsurface functionality.
If no such file is present, a new xml file is created.

c. If you wish to save the libdivecomputer log files, tick the checkbox and
input the logfile name.

d. If you wish to download the dump of the divecomputer, tick the dump checkbox
and input the required dump name.

e. If you want to force the download of dives even though the dives may already
be present in the xml file, tick the force download checkbox.

f. Similary if you wish that downloaded dives be preferred than existing, tick
the preferred checkbox.

g. Click on OK button. A dialog showing attached USB devices are shown. The
dialog will also show the VID and PID of the devices. Choose the one matching
your divecomputer. If you do not have permission for the usage of the attached
USB device, a dialogbox asing for the permission can be seen. Grant the
permission to continue.


h. The download will begin and would be marked by an indefinitely rotating
circle on the actiobar. In case of any errors during load, download or save of
the files, an error dialogbox is shown. On successful import, a download toast
can be seen.

i. The saved dives are generally saved in sdcard/Dives folder. If you wish to
change the directory of file saving, you can do so in the Settings accessed by
menu icon present in the actionbar. The given name is attached to location of
sdcard.

###Supported Divecomputers

At present only FTDI based divecomputers are supported. A non exhaustive list is
given below.

1. Suunto devices
2. Suunto Smart Interface devices
3. Oceanic devices
4. Cressi Leonardo
5. Heinrichs Weikamp

In case your divecomputer is not present above, please test if you have a FTDI
chipset based divecomputer by executing the following dialog on linux.

```
$ lsusb
```
Result similar to this is seen on my system.

```
003 Device 074: ID 0403:6001 Future Technology Devices International, Ltd
FT232 USB-Serial (UART) IC
```

If the VID of your divecomputer comes out to be 0x0403, you are in luck. Please
go forward and try this application. If not, don't lose heart. Support is coming
soon.

###TODO

Following other features can be included in the application.

1. Progress bar indicator showing the dive import progress as is present in
   subsurface application.

2. Saving of favourite divecomputers.

3. Listing and showing the imported dives and logs.

4. Support for more divecomputer chipsets.

###Bugs and Issues

The application has not yet been checked for downloading device dumps. There
might be some issues with it.

Some of the major issues are given here :

1. Sometimes race condition is seen due to which the indeterminate state of the
   application continues indefinitely. Neither does the application exits, nor
   does the download finishes,

2. Files of the same name are always overwritten. (except for xml files which
   are loaded first)

These bugs and their status can be seen [here](https://github.com/venkateshshukla/subsurface-android-downloader/).
Please report any more bugs found during the usage and feature request on the
above link.

For other queries and information, drop an email to the developer at
venkatesh.shukla.eee11@iitbhu.ac.in .

