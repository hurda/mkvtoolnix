Building MKVToolNix 7.8.0 for Windows
=====================================

There is currently only one supported way to build MKVToolNix for
Windows: on Linux using a mingw cross compiler. It is known that you
can also build it on Windows itself with the mingw gcc compiler, but
that's not supported officially as I don't have such a setup myself.

Earlier versions could still be built with Microsoft's Visual Studio /
Visual C++ programs, and those steps were described here as
well. However, current MKVToolNix versions require many features of
the new C++11 standard which haven't been supported by Microsoft's
compilers for a long time. Additionally the author doesn't use Visual
C++ himself and couldn't provide project files for it.

# 1. Building with a mingw cross compiler

## 1.1. Preparations

You will need:

- a mingw cross compiler
- roughly 4 GB of free space available

Luckily there's the [mingw-cross-env project]
(http://mingw-cross-env.nongnu.org/) that provides an easy-to-use way
of setting up the cross-compiler and all required libraries.

mxe is a fast-changing project. In order to provide a stable basis for
compilation author maintains his own fork. That fork also includes a
couple of changes that cause libraries to be compiled only with the
features required by MKVToolNix saving compilation time and deployment
space. In order to retrieve that fork you need `git`. Then to the
following:

    git clone https://github.com/mbunkus/mxe $HOME/mingw-cross-env

The rest of this guide assumes that you've unpacked mingw-cross-env
into the directory `$HOME/mingw-cross-env`.

## 1.2. Automatic build script

MKVToolNix contains a script that can download, compile and install
all required libraries into the directory `$HOME/mingw-cross-env`.

If the script does not work or you want to do everything yourself
you'll find instructions for manual compilation in section 1.3.

### 1.2.1. Script configuration

The script is called `winbuild/setup_cross_compilation_env.sh`. It
contains the following variables that can be adjusted to fit your
needs:

    ARCHITECTURE=64

The architecture (64bit vs 32bit) that the binaries will be built
for. The majority of users to day run a 64bit Windows, therefore 64 is
the default. If you run a 32bit version of Windows then change this to
32.

    INSTALL_DIR=$HOME/mingw-cross-env

Base installation directory

    PARALLEL=

Number of processes to execute in parallel. Will be set to the number
of cores available if left empty.

### 1.2.2. Execution

From the MKVToolNix source directory run:

    ./winbuild/setup_cross_compilation_env.sh

If everything works fine you'll end up with a configured MKVToolNix
source tree. You just have to run `drake` afterwards.

## 1.3. Manual installation

First you will need the mingw-cross-env build scripts. Get them by
downloading them (see section 1.1. above) and unpacking them into
`$HOME/mingw-cross-env`.

Next, build the required libraries (change `MXE_TARGETS` to
`i686-w64-mingw32.static` if you need a 32bit build instead of a 64bit
one, and increase `JOBS` if you have more than one core):

    cd $HOME/mingw-cross-env
    make MXE_TARGETS=x86_64-w64-mingw32.static JOBS=2 \
      gettext libiconv zlib expat boost curl file flac lzo ogg pthreads \
      vorbis wxwidgets qtbase qttranslations

Append the installation directory to your `PATH` variable:

    export PATH=$PATH:$HOME/mingw-cross-env/usr/bin
    hash -r

Finally, configure MKVToolNix (the `host=…` spec must match the
`MXE_TARGETS` spec from above):

    cd $HOME/path/to/mkvtoolnix-source
    host=x86_64-w64-mingw32.static
    qtbin=$HOME/mingw-cross-env/usr/${host}/qt5/bin
    ./configure \
      --host=${host} \
      --enable-qt --enable-static-qt --with-mkvtoolnix-gui \
      --with-moc=${qtbin}/moc --with-uic=${qtbin}/uic --with-rcc=${qtbin}/rcc \
      --with-boost=$HOME/mingw-cross-env/usr/${host}

If everything works then build it:

    ./drake

You're done.
