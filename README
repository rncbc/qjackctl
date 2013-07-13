QjackCtl - JACK Audio Connection Kit Qt GUI Interface
-----------------------------------------------------

QjackCtl is a simple Qt application to control the JACK sound server
(http://jackaudio.org), for the Linux Audio infrastructure.

Written in C++ around the Qt4 toolkit for X11, most exclusively using
Qt Designer.

Provides a simple GUI dialog for setting several JACK server parameters,
which are properly saved between sessions, and a way control of the
status of the audio server. With time, this primordial interface has
become richer by including a enhanced patchbay and connection control
features.

Homepage: http://qjackctl.sourceforge.net

License: GNU General Public License (GPL)


Requirements
------------

The software requirements for build and runtime are listed as follows:

  Mandatory:

  - Qt4 (core, gui, xml), C++ class library and tools for
        crossplatform development and internationalization
        http://qt-project.org/

  - JACK Audio Connection Kit
        http://jackaudio.org/

  Optional (opted-in at build time):

  - ALSA, Advanced Linux Sound Architecture
        http://www.alsa-project.org/


Installation
------------

The installation procedure follows the standard for source distributions:

    ./configure [--prefix=/usr/local]
    make

and optionally as root:

    make install

This procedure will end installing the following files:

    ${prefix}/bin/qjackctl
    ${prefix}/share/pixmaps/qjackctl.png
    ${prefix}/share/applications/qjackctl.desktop
    ${prefix}/share/locale/qjackctl_*.qm
    ${prefix}/share/man/man1/qjackctl.1

Just launch ${prefix}/bin/qjackctl and you're off (hopefully).

If you're checking out from Subversion (SVN), you'll have to prepare the
configure script just before you proceed with the above instructions:

   make -f Makefile.svn


Configuration
-------------

QjackCtl holds its settings and configuration state per user, in a file
located as $HOME/.config/rncbc.org/QjackCtl.conf . Normally, there's no
need to edit this file, as it is recreated and rewritten everytime
qjackctl is run.


Bugs
----

Probably plenty still, QjackCtl maybe considered on beta stage already.
It has been locally tested since JACK release 0.98.0, with custom 2.4
kernels with low-latency, preemptible and capabilities enabling patches.
As for 2.6 kernels, the emergence of Ingo Molnar's Realtime Preemption
kernel patch it's being now recommended for your taking benefit of the
realtime and low-latency audio pleasure JACK can give.


Support
-------

QjackCtl is open source free software. For bug reports, feature
requests, discussion forums, mailling lists, or any other matter
related to the development of this piece of software, please use the
Sourceforge project page (http://sourceforge.net/projects/qjackctl).

You can also find timely and closer contact information on my personal
web site (http://www.rncbc.org).


Acknowledgments
---------------

QjackCtl's user interface layout (and the whole idea for that matter)
was partially borrowed from origoinal Lawrie Abbott's jacko project,
which was taken from wxWindow/Python into the Qt/C++ arena.

Since 2003-08-06, qjackctl has been included in the awesome Planet CCRMA
(http://ccrma-www.stanford.edu/planetccrma/software/) software collection.
Thanks a lot Fernando!

Here are some people who helped this project in one way or another,
and in fair and strict alphabetic order:

    Alexandre Prokoudine             Kasper Souren
    Andreas Persson                  Kjetil Matheussen
    Arnout Engelen                   Ken Ellinwood
    Austin Acton                     Lawrie Abbott
    Ben Powers                       Lee Revell
    Chris Cannam                     Lucas Brasilino
    Dan Nigrin                       Marc-Olivier Barre
    Dave Moore                       Mark Knecht
    Dave Phillips                    Matthias Nagorni
    Dirk Jagdmann                    Melanie
    Dominic Sacre                    Nedko Arnaudov
    Fernando Pablo Lopez-Lezcano     Orm Finnendahl
    Filipe Tomas                     Paul Davis
    Florian Schmidt                  Robert Jonsson
    Fons Adriaensen                  Robin Gareus
    Geoff Beasley                    Roland Mas
    Jack O'Quin                      Sampo Savolainen
    Jacob Meuser                     Stephane Letz
    Jesse Chappell                   Steve Harris
    Joachim Deguara                  Taybin Rutkin
    John Schneiderman                Wilfried Huss
    Jussi Laako                      Wolfgang Woehl
    Karsten Wiese

A special mention should go to the translators of QjackCtl (see TRANSLATORS).

Thanks to you all.
--
rncbc aka Rui Nuno Capela
rncbc@rncbc.org
