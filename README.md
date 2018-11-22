# Readme for desh

[![copr status](https://copr.fedorainfracloud.org/coprs/injinj/gold/package/desh/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/injinj/gold/package/desh/)

## desh

This is an experimental shell that I use.  It is a forked version of the es job
control branch, described below.  I changed the the name to desh to because it
is currently has shell features which are not compatible with es.

The main thing that is changed between desh and es is that the readline library
is no longer used.  Another command line editor called linecook is used
instead.  The linecook interface with the user has similarities with readline,
but it is not api compatible with it.

The linecook library is the primary reason desh exists.  I am using desh to
test and advance features in linecook.  The major reason that I chose this
particular branch of this code base is because it has job control, which means
that ctrl-z will suspend the currently executing job.

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

1. [es](https://wryun.github.io/es-shell/) -- this is the home page for es, it
   describes the lineage and links to several academic papers.

2. [XS](https://github.com/TieDyedDevil/XS) -- this is the fork of es which
   incorporates the Boehm garbage collector.  The GC parts of es is one of the
   major pieces of it's architecture.

The rpm install from
[copr](https://copr.fedorainfracloud.org/coprs/injinj/gold/) or "make dist_rpm"
will put an ini file into [/etc/deshrc](script/deshrc) which should cause the
shell to look like this: [desh example](doc/desh_example.png).  For the key
bindings, hit F1 or ctrl-alt-k, F4 or ctrl-alt-l to clear, pgup/pgdown or
alt-k,alt-j to page the view: [desh key](doc/desh_key.png).

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
