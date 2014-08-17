#!/bin/bash
set -e

# Configure where we can find things here
export ANDROID_NDK_ROOT=/android_ndk
export ANDROID_SDK_ROOT=/android_sdk
export QT5_ANDROID=/Qt5.3.0/5.3

# Platform architecture
export ARCH=${1-x86}

if [ "$ARCH" = "arm" ] ;
then
	QT_ARCH="armv7"
	BUILDCHAIN=arm-linux-androideabi
	export TARGET_ARCH_ABI=armeabi-v7a
elif [ "$ARCH" = "x86" ] ;
then
	QT_ARCH=$ARCH
	BUILDCHAIN=i686-linux-android
	export TARGET_ARCH_ABI=x86
fi

export QT5_ANDROID_BIN=${QT5_ANDROID}/android_${QT_ARCH}/bin

pushd crossbuild

# Initialise cross toolchain
if [ ! -e ndk-$ARCH ] ; then
	$ANDROID_NDK_ROOT/build/tools/make-standalone-toolchain.sh --arch=$ARCH --install-dir=ndk-$ARCH --platform=android-19
fi

# Declare necessary variables for cross compilation
export BUILDROOT=$PWD
export PATH=${BUILDROOT}/ndk-$ARCH/bin:$PATH
export PREFIX=${BUILDROOT}/ndk-$ARCH/sysroot/usr
export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig
export CC=${BUILDCHAIN}-gcc
export CXX=${BUILDCHAIN}-g++
export AR=${BUILDCHAIN}-ar

# Download libdivecomputer, libusb and libftdi submodule
if [ ! -e libdivecomputer/configure.ac ] || [ ! -e libusb/configure.ac ] \
	|| [ ! -e libftdi/CMakeLists.txt ] || [ ! -e subsurface/subsurface.pro ] ; then
	git submodule init
	git submodule update
fi

# Prepare for building
mkdir -vp build

pushd build

if [ ! -e sqlite-autoconf-3080200.tar.gz ] ; then
	wget http://www.sqlite.org/2013/sqlite-autoconf-3080200.tar.gz
fi
if [ ! -e sqlite-autoconf-3080200 ] ; then
	tar -zxf sqlite-autoconf-3080200.tar.gz
fi
if [ ! -e $PKG_CONFIG_PATH/sqlite3.pc ] ; then
	mkdir -p sqlite-build-$ARCH
	pushd sqlite-build-$ARCH
	../sqlite-autoconf-3080200/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared
	make -j8
	make install
	popd
fi

if [ ! -e libxml2-2.9.1.tar.gz ] ; then
	wget ftp://xmlsoft.org/libxml2/libxml2-2.9.1.tar.gz
fi
if [ ! -e libxml2-2.9.1 ] ; then
	tar -zxf libxml2-2.9.1.tar.gz
fi
if [ ! -e $PKG_CONFIG_PATH/libxml-2.0.pc ] ; then
	mkdir -p libxml2-build-$ARCH
	pushd libxml2-build-$ARCH
	../libxml2-2.9.1/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --without-python --without-iconv --enable-static --disable-shared
	perl -pi -e 's/runtest\$\(EXEEXT\)//' Makefile
	perl -pi -e 's/testrecurse\$\(EXEEXT\)//' Makefile
	make -j8
	make install
	popd
fi

if [ ! -e libxslt-1.1.28.tar.gz ] ; then
	wget ftp://xmlsoft.org/libxml2/libxslt-1.1.28.tar.gz
fi
if [ ! -e libxslt-1.1.28 ] ; then
	tar -zxf libxslt-1.1.28.tar.gz
	cp libxml2-2.9.1/config.sub libxslt-1.1.28
fi
if [ ! -e $PKG_CONFIG_PATH/libxslt.pc ] ; then
	mkdir -p libxslt-build-$ARCH
	pushd libxslt-build-$ARCH
	../libxslt-1.1.28/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --with-libxml-prefix=${PREFIX} --without-python --without-crypto --enable-static --disable-shared
	make
	make install
	popd
fi

# libz.a not available in android arm ? We shall cross compile it.
if [ ! -e zlib-1.2.8.tar.gz ] ; then
	wget http://cznic.dl.sourceforge.net/project/libpng/zlib/1.2.8/zlib-1.2.8.tar.gz
fi
if [ ! -e zlib-1.2.8 ] ; then
	tar -zxf zlib-1.2.8.tar.gz
fi
if [ ! -e $PKG_CONFIG_PATH/zlib.pc ] ; then
	pushd zlib-1.2.8
	./configure --prefix=${PREFIX}
	make
	make install
	popd
fi

if [ ! -e libzip-0.11.2.tar.gz ] ; then
	wget http://www.nih.at/libzip/libzip-0.11.2.tar.gz
fi
if [ ! -e libzip-0.11.2 ] ; then
	tar -zxf libzip-0.11.2.tar.gz
fi
if [ ! -e $PKG_CONFIG_PATH/libzip.pc ] ; then
	mkdir -p libzip-build-$ARCH
	pushd libzip-build-$ARCH
	../libzip-0.11.2/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared
	make
	make install
	popd
fi

if [ ! -e libgit2-0.20.0.tar.gz ] ; then
	wget -O libgit2-0.20.0.tar.gz https://github.com/libgit2/libgit2/archive/v0.20.0.tar.gz
fi
if [ ! -e libgit2-0.20.0 ] ; then
	tar -zxf libgit2-0.20.0.tar.gz
fi
if [ ! -e $PKG_CONFIG_PATH/libgit2.pc ] ; then
	mkdir -p libgit2-build-$ARCH
	pushd libgit2-build-$ARCH
	# -DCMAKE_CXX_COMPILER=arm-linux-androideabi-g++
	cmake -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_VERSION=Android -DCMAKE_C_COMPILER=${CC} -DCMAKE_FIND_ROOT_PATH=${PREFIX} -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY -DANDROID=ON -DSHA1_TYPE=builtin -DBUILD_CLAR=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${PREFIX} ../libgit2-0.20.0/
	make
	make install
	popd
fi

if [ ! -e libexif-0.6.21.tar.bz2 ] ; then
	wget http://kaz.dl.sourceforge.net/project/libexif/libexif/0.6.21/libexif-0.6.21.tar.bz2
fi
if [ ! -e libexif-0.6.21 ] ; then
	tar -xvf libexif-0.6.21.tar.bz2
fi
if [ ! -e $PKG_CONFIG_PATH/libexif.pc ] ; then
	mkdir -p libexif-build-$ARCH
	pushd libexif-build-$ARCH
	../libexif-0.6.21/configure  --host=i686-unknown-linux-gnu --prefix=${PREFIX} --enable-static --disable-shared
	make
	make install
	popd
fi

popd

# Build libusb
if [ ! -e libusb/configure ] ; then
	pushd libusb
	autoreconf --install
	popd
fi

mkdir -p build/libusb-build-$ARCH
pushd build/libusb-build-$ARCH
if [ ! -e Makefile ] ; then
	../../libusb/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared --disable-udev
fi
make
make install
popd

# Build libftdi
mkdir -p build/libftdi-build-$ARCH
pushd build/libftdi-build-$ARCH
if [ ! -e Makefile ] ; then
	cmake -DCMAKE_C_COMPILER=${CC} -DCMAKE_INSTALL_PREFIX=${PREFIX} -DCMAKE_PREFIX_PATH=${PREFIX} -DBUILD_SHARED_LIBS=OFF -DSTATICLIBS=ON -DPYTHON_BINDINGS=OFF -DDOCUMENTATION=OFF -DFTDIPP=OFF ../../libftdi
fi
make
make install
popd

# Build libdivecomputer
if [ ! -e libdivecomputer/configure ] ; then
	pushd libdivecomputer
	autoreconf -i
	popd
fi

mkdir -p build/libdivecomputer-build-$ARCH
pushd build/libdivecomputer-build-$ARCH
if [ ! -e Makefile ] ; then
	../../libdivecomputer/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared --enable-logging
fi
make
make install
popd

mkdir -p build/subsurface-build-$ARCH
pushd build/subsurface-build-$ARCH
if [ ! -e Makefile ] ; then
	$QT5_ANDROID_BIN/qmake ../../subsurface QT_CONFIG=+pkg-config CONFIG+=android_jni
fi
make -j4
make install INSTALL_ROOT=${BUILDROOT}/..

popd # from crossbuild

if [[ $? == 0 ]] ; then
echo "Finished building requisites."
fi

# Build native libraries
$ANDROID_NDK_ROOT/ndk-build -B

# Update application if build.xml is not present
if [ ! -e build.xml ] ; then
	android update project -p  .
fi

# Build the project in debug mode and install on the connected device
ant debug install

# Run the application on the emulator
adb shell am start -a android.intent.action.MAIN -n org.subsurface.downloader/.Main

# Show the logcat
adb logcat -v threadtime | grep --color=auto -e 'org.subsurface.downloader' \
	-e 'stderr' -e 'stdout' -e `adb shell ps | grep org.subsurface.downloader | cut -c 11-14`

