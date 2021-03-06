# Readme for desh

[![copr status](https://copr.fedorainfracloud.org/coprs/injinj/gold/package/desh/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/injinj/gold/package/desh/)

## desh

This is an experimental shell that I use.  It is a forked version of the
<b>es</b> job control branch, described below.  I changed the the name to
<b>desh</b> to because it currently has shell features which are not compatible
with <b>es</b>.

The main thing that is changed between <b>desh</b> and <b>es</b> is that the
readline library is no longer used.  Another command line editor called
linecook is used instead.  The linecook interface with the user has
similarities with readline, but it is not api compatible with it.

The linecook library is the primary reason <b>desh</b> exists.  I am using
<b>desh</b> to test and advance features in linecook.  The major reason that I
chose this particular branch of this code base is because it has job control,
which means that ctrl-z will suspend the currently executing job.

Other differences:

1. Added :gt, :lt, :ge, :ge, :eq, :ne operators, %cmp primative. These operate
   numerically if both args are numbers and lexically if not (from XS).

2. Added %sum, %diff, %mul, %div, %pow, %neg, %log, %log10, %floor, %round,
   %isinf, %isnan primitives, which operate on lists of numbers.

3. Added libdecnumber for the math behind the above numbers.  It is a IEEE
   754-2008 library forked from gcc (IBM and Intel authored).  The 128 bit
   decimal math parts are used here, which gives about 34 decimal places and
   remembers the precision of values.  There is never ambiguity of numeric
   conversions since the binary representation is equivalent to decimal
   strings, which humans like to use.

Be sure to visit these pages:

1. [es](https://wryun.github.io/es-shell/) -- this is the home page for
   <b>es</b>, it describes the lineage and links to several academic papers.

2. [XS](https://github.com/TieDyedDevil/XS) -- this is the fork of <b>es</b>
   which incorporates the Boehm garbage collector.  The GC parts of <b>es</b>
   is one of the major pieces of it's architecture.

The rpm install from
[copr](https://copr.fedorainfracloud.org/coprs/injinj/gold/) or "make dist_rpm"
will put an ini file into [/etc/deshrc](script/deshrc) which should cause the
shell to look like this: [desh example](doc/desh_example.png).  For the key
bindings, hit F1 or ctrl-alt-k, F4 or ctrl-alt-l to clear, pgup/pgdown or
alt-k,alt-j to page the view: [desh key](doc/desh_key.png).

I made a asciicast for a quick tour of the features here:

[![asciicast](https://asciinema.org/a/219909.svg)](https://asciinema.org/a/219909)

### Making desh

#### CentOS & Fedora install

This project uses a makefile so make will build everything.  It needs gcc-c++,
byacc and the git submodules.  I'm using Fedora 27 below.

```console
$ git clone git@github.com:injinj/desh.git
$ cd desh
$ git submodule update --init --recursive
$ sudo dnf install gcc-c++ make byacc chrpath pcre2-devel
$ make
$ FC27_x86_64/bin/desh
;
```

That should be it.  The target will be in a build directory called FC28_x86_64
or RH7_x86_64 or UB9_x86_64, based on the Linux distro installed.  There are
run paths inserted into the binary so that it finds the shared libraries when
run from the make directory for debugging.  These paths are removed for the rpm
install using the chrpath command.

If you run this way, source the script/deshrc file.  It has prompt styling and
completion rules that are useful:  git branch decoding, fzf integration,
terminal pwd title, and the history file.  This file is normally installed at
/etc/deshrc.

```console
$ FC27_x86_64/bin/desh
; . script/deshrc
chris@deedee:~
[4983];
```

The make dist_rpm or make dist_dpkg will create an rpm/deb based install using
the rpm/desh.spec or deb/rules file, but it needs the submodule rpm/deb
installed before the desh rpm/deb itself is built and installed.

```console
$ cd linecook
$ make dist_rpm
$ sudo rpm -i rpmbuild/RPMS/x86_64/linecook-1.1.0-14.fc28.x86_64.rpm
$ cd ../libdecnumber
$ make dist_rpm
$ sudo rpm -i rpmbuild/RPMS/x86_64/libdecnumber-3.61.0-5.fc28.x86_64.rpm
$ cd ..
$ make dist_rpm
$ sudo rpm -i rpmbuild/RPMS/x86_64/desh-1.1.0-16.fc28.x86_64.rpm
```

Uninstalling the rpms is with the -e option:

```console
$ sudo rpm -e linecook libdecnumber desh
```

#### Debian & Ubuntu install

For Debian based, there is a dist_dpkg target, which will create a debian
package.

```console
$ cd linecook
$ make dist_dpkg
$ sudo dpkg -i dpkgbuild/linecook_1.1.0-14_amd64.deb
$ cd ../libdecnumber
$ make dist_dpkg
$ sudo dpkg -i dpkgbuild/libdecnumber_3.61.0-5_amd64.deb
$ cd ..
$ make dist_dpkg
$ sudo dpkg -i dpkgbuild/desh_1.1.0-16_amd64.deb
```

Uninstalling dpkgs:

```console
$ sudo dpkg -r linecook libdecnumber desh
```

#### Ubuntu under Windows 10 install

For Ubuntu 18.04 under Windows 10, this build works for me:

```console
$ sudo apt-get update
$ sudo apt-get install make g++ gcc devscripts libpcre2-dev chrpath
$ sudo apt-get install debhelper bison
$ sudo update-alternatives --set fakeroot /usr/bin/fakeroot-tcp
$ git clone https://github.com/injinj/desh
$ cd desh
$ git submodule update --init --recursive
$ cd libdecnumber
$ make dist_dpkg
$ sudo dpkg -i dpkgbuild/libdecnumber_3.61.0-5_amd64.deb
$ cd ../linecook
$ make dist_dpkg
$ sudo dpkg -i dpkgbuild/linecook_1.3.0-29_amd64.deb
$ cd ..
$ make dist_dpkg
$ sudo dpkg -i dpkgbuild/desh_1.3.0-25_amd64.deb
```

The version numbers used in all of the above examples will (and have) changed.

The weird part of the Windows Ubuntu build is the fakeroot-tcp requirement.
This is because Windows does not yet implement SYSV IPC message queues, which
the standard fakeroot uses.

I've also built CentOS 7, Fedora 27, Fedora 28, Fedora 29, Debian 9 x86_64, so
I know at least these will work,  CentOS 6 does not work without adding utf-32
glibc support for linecook.  That shouldn't be terribly difficult, but I don't
have a reason for that port.

#### Solaris 11 install

Notes for the SunOS 5.11 port.  I used gcc-7 and I have /usr/gnu/bin in my
PATH.  Set CC=gcc, cc is not an alias for gcc. I needed to make pcre2, the
standard package does not include libpcre2-32.  That requires gnu autotools and
friends.  I also needed rpath to remove the library run paths.  The libraries
live in /usr/lib/64, the runtime linker uses /usr/lib for 32 bit libraries.

```console
$ git clone https://github.com/luvit/pcre2
$ cd pcre2
$ configure --enable-pcre2-16 --enable-pcre2-32 --prefix=/usr
$ make
$ make install  # as su
# manually mv things from /usr/lib into /usr/lib/64 and change the pkgconfig
$ wget http://www.linker-aliens.org/blogs/ali/entry/changing_elf_runpaths/resource/rpath.tgz
$ tar xvzf rpath.tgz
$ cd rpath ; make
$ cp rpath /usr/bin/rpath             # as su
$ cp cp rpath.1 /usr/share/man/man1/  # as su
$ git clone https://github.com/injinj/desh
$ cd desh
$ git submodule update --init --recursive
$ cd libdecnumber
$ make
$ make install_prefix=/usr install_lib_suffix=/64 install  # as su
$ cd ../linecook
$ make
$ make install_prefix=/usr install_lib_suffix=/64 install  # as su
$ cd ..
$ make
$ make install_prefix=/usr install_lib_suffix=/64 install  # as su
$ cp /usr/share/doc/desh/deshrc /etc/deshrc
```

#### Apple Mac install

Notes for the Mac port.  I used clang from `xcode-select --install` and
installed pcre2, linecook, libdecnumber to '/usr/local':

```console
$ curl -O https://ftp.pcre.org/pub/pcre/pcre2-10.35.tar.gz
$ tar xvzf pcre2-10.35.tar.gz
$ cd pcre2-10.35
$ sh configure --enable-pcre2-16 --enable-pcre2-32
$ make
$ sudo make install
$ git clone https://github.com/raitechnology/libdecnumber
$ cd libdecnumber
$ make ; sudo make install
$ cd ..
$ git clone https://github.com/injinj/linecook
$ cd linecook
$ make ; sudo make install
$ cd ..
$ git clone https://github.com/injinj/desh
$ cd desh
$ make ; sudo make install
$ sudo cp -a /usr/local/share/doc/desh/deshrc /etc/deshrc
$ sudo sh -c 'echo /usr/local/bin/desh >> /etc/shells'
$ chsh -s /usr/local/bin/desh
```

Make sure to set the TERM env var to one with colors, where 'tput colors'
returns >= 8.  With xterm, I add this to my <b>~/.Xresources</b>:

```
XTerm*termName: xterm-256color
```

The default TERM var is xterm, even though it is capable of displaying colors.

## es 0.9

This is the README file for es, version 0.9-beta1

I include the original README file below.  See the CHANGES file for 
a list of changes to the shell.  See the INSTALL file for instructions 
for installing es.  In all other ways, I believe that this file is up
to date.

Soren Dayton
csdayton@cs.uchicago.edu

## Original es readme

(the links are mostly dead)

Es is an extensible shell.  The language was derived from the Plan 9
shell, rc, and was influenced by functional programming languages,
such as Scheme, and the Tcl embeddable programming language.  This
implementation is derived from Byron Rakitzis's public domain
implementation of rc.

WARNING: This is an experimental release of es.  Some aspects of this
	 release are unstable.  If you aren't feeling adventurous, you
	 may want to use version 0.84.

The Makefile should just work; if it doesn't, please let us know.  The
first few lines include some comments about what might be problematic.
Please see config.h for any conditional flags you may have to set on
the command line to make es compile.  An ANSI C compiler is
practically required.

See the file CHANGES for recent changes to the shell.  For details on
how to use es, please see the man page, which is unfortunately a bit
out of date.  The file initial.es, which is used to build the initial
memory state of the es interpreter, can be read to better understand
how pieces of the shell interact.

For some background into our motivation for writing es, see our Winter
1993 Usenix paper, ``Es: a shell with higher-order functions,''
available by anonymous ftp as

	ftp.sys.utoronto.ca:/pub/es/usenix-w93.ps.Z

The paper corresponds to a slightly older version of the shell;  see
the file ERRATA for changes which affect parts of the paper.

An old version of Paul's .esrc (es startup) file is provided as an
example as esrc.haahr; correctness is not guaranteed.  A simple
debugger for es scripts, esdebug, is also included; this is very
untested and should be considered little more than a sketch of a few
ideas.

A simple history mechanism for this shell (conceptually derived from
the Research Eighth Edition Unix =(1) commands) is available in

	ftp.sys.utoronto.ca:/pub/es/history.tar.Z

Es supports the use of GNU readline (or compatible) libraries for
interactive command-line editing.  Readline may be obtained from any
of the many GNU project distribution sites, such as prep.ai.mit.edu.
Simi Turner wrote a much smaller readline clone called editline which
Rich Salz has been maintaining; a copy of it is available as

	ftp.sys.utoronto.ca:/pub/es/editline.tar.Z

Please report any problems to us at

	Paul Haahr <haahr@adobe.com>
	Byron Rakitzis <byron@netapp.com>

There is a mailing list for discussing es.  Send mail to

	es-request@hawkwind.utcs.toronto.edu

to join the list.  The list itself is <sub id="ml">[1](#ml)</sup>

	es@hawkwind.utcs.toronto.edu

Es is in the public domain.  We hold no copyrights or patents on
the source code, and do not place any restrictions on its distribution.
We would appreciate it if any distributions do credit the authors.

Enjoy!

-- Paul Haahr & Byron Rakitzis

<b id="ml">1</b>: I believe the mailing list has migrated to a [google
  group](https://groups.google.com/forum/#!forum/es-shell)
