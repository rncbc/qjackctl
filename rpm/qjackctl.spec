#
# spec file for package qjackctl
#
# Copyright (C) 2003-2024, rncbc aka Rui Nuno Capela. All rights reserved.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.
#
# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

%define name    qjackctl
%define version 0.9.13
%define release 57.1

%define _prefix	/usr

%if %{defined fedora}
%define debug_package %{nil}
%endif

%if 0%{?fedora_version} >= 34 || 0%{?suse_version} > 1500 || ( 0%{?sle_version} == 150200 && 0%{?is_opensuse} )
%define qt_major_version  6
%else
%define qt_major_version  5
%endif

Summary:	JACK Audio Connection Kit Qt GUI Interface
Name:		%{name}
Version:	%{version}
Release:	%{release}
License:	GPL-2.0+
Group:		Productivity/Multimedia/Sound/Utilities
Source0:	%{name}-%{version}.tar.gz
URL:		http://qjackctl.sourceforge.net/
Packager:	rncbc.org

BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-buildroot

BuildRequires:	coreutils
BuildRequires:	pkgconfig
BuildRequires:	glibc-devel
BuildRequires:	cmake >= 3.15
%if 0%{?sle_version} >= 150200 && 0%{?is_opensuse}
BuildRequires:	gcc10 >= 10
BuildRequires:	gcc10-c++ >= 10
%define _GCC	/usr/bin/gcc-10
%define _GXX	/usr/bin/g++-10
%else
BuildRequires:	gcc >= 10
BuildRequires:	gcc-c++ >= 10
%define _GCC	/usr/bin/gcc
%define _GXX	/usr/bin/g++
%endif
%if 0%{qt_major_version} == 6
%if 0%{?sle_version} == 150200 && 0%{?is_opensuse}
BuildRequires:	qtbase6.6-static >= 6.6
BuildRequires:	qttools6.6-static
BuildRequires:	qttranslations6.6-static
BuildRequires:	qtsvg6.6-static
%else
BuildRequires:	cmake(Qt6LinguistTools)
BuildRequires:	pkgconfig(Qt6Core)
BuildRequires:	pkgconfig(Qt6Gui)
BuildRequires:	pkgconfig(Qt6Widgets)
BuildRequires:	pkgconfig(Qt6Svg)
BuildRequires:	pkgconfig(Qt6Xml)
BuildRequires:	pkgconfig(Qt6Network)
%endif
%else
BuildRequires:	cmake(Qt5LinguistTools)
BuildRequires:	pkgconfig(Qt5Core)
BuildRequires:	pkgconfig(Qt5Gui)
BuildRequires:	pkgconfig(Qt5Widgets)
BuildRequires:	pkgconfig(Qt5Svg)
BuildRequires:	pkgconfig(Qt5Xml)
BuildRequires:	pkgconfig(Qt5Network)
BuildRequires:	pkgconfig(Qt5DBus)
%endif
%if %{defined fedora}
BuildRequires:	jack-audio-connection-kit-devel
%else
BuildRequires:	pkgconfig(jack)
%endif
BuildRequires:	pkgconfig(alsa)

%description
JACK Audio Connection Kit - Qt GUI Interface: A simple Qt application
to control the JACK server. Written in C++ around the Qt framework
for X11, most exclusively using Qt Designer. Provides a simple GUI
dialog for setting several JACK server parameters, which are properly
saved between sessions, and a way control of the status of the audio
server. With time, this primordial interface has become richer by 
including a enhanced patchbay and connection control features.

%prep
%setup -q

%build
%if 0%{?sle_version} == 150200 && 0%{?is_opensuse}
source /opt/qt6.6-static/bin/qt6.6-static-env.sh
%endif
CXX=%{_GXX} CC=%{_GCC} \
cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} -Wno-dev -B build
cmake --build build %{?_smp_mflags}

%install
DESTDIR="%{buildroot}" \
cmake --install build

%clean
[ -d "%{buildroot}" -a "%{buildroot}" != "/" ] && %__rm -rf "%{buildroot}"

%files
%defattr(-,root,root)
%license LICENSE
%doc README TRANSLATORS ChangeLog
#dir %{_datadir}/applications
%dir %{_datadir}/icons/hicolor
%dir %{_datadir}/icons/hicolor/32x32
%dir %{_datadir}/icons/hicolor/32x32/apps
%dir %{_datadir}/icons/hicolor/scalable
%dir %{_datadir}/icons/hicolor/scalable/apps
%dir %{_datadir}/%{name}
%dir %{_datadir}/%{name}/translations
%dir %{_datadir}/metainfo
#dir %{_datadir}/man
#dir %{_datadir}/man/man1
#dir %{_datadir}/man/fr
#dir %{_datadir}/man/fr/man1
%{_bindir}/%{name}
%{_datadir}/applications/org.rncbc.%{name}.desktop
%{_datadir}/icons/hicolor/32x32/apps/org.rncbc.%{name}.png
%{_datadir}/icons/hicolor/scalable/apps/org.rncbc.%{name}.svg
%{_datadir}/%{name}/translations/%{name}_*.qm
%{_datadir}/metainfo/org.rncbc.%{name}.metainfo.xml
%{_datadir}/man/man1/%{name}.1.gz
%{_datadir}/man/fr/man1/%{name}.1.gz

%changelog
* Wed Jan 24 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.9.13
- A Winter'24 Release.
* Sat Sep  9 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.9.12
- An End-of-Summer'23 Release.
* Thu Jun  1 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.9.11
- A Spring'23 Release.
* Thu Mar 23 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.9.10
- An Early-Spring'23 Release.
* Wed Dec 28 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.9.9
- An End-of-Year'22 Release.
* Mon Oct  3 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.9.8
- An Early-Autumn'22 Release.
* Sat Apr  2 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.9.7
- A Spring'22 Release.
* Sun Jan  9 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.9.6
- A Winter'22 Release.
* Tue Oct  5 2021 Rui Nuno Capela <rncbc@rncbc.org> 0.9.5
- Autumn'21 release.
* Sun Jul  4 2021 Rui Nuno Capela <rncbc@rncbc.org> 0.9.4
- Early-Summer'21 release.
* Tue May 11 2021 Rui Nuno Capela <rncbc@rncbc.org> 0.9.3
- Spring'21 release.
* Sun Mar 14 2021 Rui Nuno Capela <rncbc@rncbc.org> 0.9.2
- End-of-Winter'21 release.
* Sun Feb  7 2021 Rui Nuno Capela <rncbc@rncbc.org> 0.9.1
- Winter'21 release.
* Thu Dec 17 2020 Rui Nuno Capela <rncbc@rncbc.org> 0.9.0
- Winter'20 release.
* Fri Jul 31 2020 Rui Nuno Capela <rncbc@rncbc.org> 0.6.3
- Summer'20 release.
* Tue Mar 24 2020 Rui Nuno Capela <rncbc@rncbc.org> 0.6.2
- Spring'20 release.
* Sun Dec 22 2019 Rui Nuno Capela <rncbc@rncbc.org> 0.6.1
- Winter'19 release.
* Thu Oct 17 2019 Rui Nuno Capela <rncbc@rncbc.org> 0.6.0
- Autumn'19 release.
* Fri Jul 12 2019 Rui Nuno Capela <rncbc@rncbc.org> 0.5.9
- Summer'19 release.
* Sat May 25 2019 Rui Nuno Capela <rncbc@rncbc.org> 0.5.8
- Spring'19 release.
* Thu Apr 11 2019 Rui Nuno Capela <rncbc@rncbc.org> 0.5.7
- Spring-Break'19 release.
* Mon Mar 11 2019 Rui Nuno Capela <rncbc@rncbc.org> 0.5.6
- Pre-LAC2019 release frenzy.
* Fri Nov 23 2018 Rui Nuno Capela <rncbc@rncbc.org> 0.5.5
- Black-Friday'18 release.
* Mon Sep 24 2018 Rui Nuno Capela <rncbc@rncbc.org> 0.5.4
- Early Autumn'18 release.
* Sun Jul 22 2018 Rui Nuno Capela <rncbc@rncbc.org> 0.5.3
- Summer'18 release.
* Sun May 27 2018 Rui Nuno Capela <rncbc@rncbc.org> 0.5.2
- Pre-LAC2018 release frenzy hotfix.
* Mon May 21 2018 Rui Nuno Capela <rncbc@rncbc.org> 0.5.1
- Pre-LAC2018 release frenzy.
* Sat Dec 16 2017 Rui Nuno Capela <rncbc@rncbc.org> 0.5.0
- End of Autumn'17 release.
* Thu Apr 27 2017 Rui Nuno Capela <rncbc@rncbc.org> 0.4.5
- Pre-LAC2017 release frenzy.
* Mon Nov 14 2016 Rui Nuno Capela <rncbc@rncbc.org> 0.4.4
- A Fall'16 release.
* Wed Sep 14 2016 Rui Nuno Capela <rncbc@rncbc.org> 0.4.3
- End of Summer'16 release.
* Tue Apr  5 2016 Rui Nuno Capela <rncbc@rncbc.org> 0.4.2
- Spring'16 release frenzy.
* Wed Oct 28 2015 Rui Nuno Capela <rncbc@rncbc.org> 0.4.1
- A Fall'15 release.
* Wed Jul 15 2015 Rui Nuno Capela <rncbc@rncbc.org> 0.4.0
- Summer'15 release frenzy.
* Wed Mar 25 2015 Rui Nuno Capela <rncbc@rncbc.org> 0.3.13
- Pre-LAC2015 release frenzy.
* Sun Oct 19 2014 Rui Nuno Capela <rncbc@rncbc.org> 0.3.12
- JACK Pretty-names aliasing.
* Tue Dec 31 2013 Rui Nuno Capela <rncbc@rncbc.org> 0.3.11
- A fifth of a Jubilee release.
* Tue Apr  2 2013 Rui Nuno Capela <rncbc@rncbc.org> 0.3.10
- The singing swan rehersal.
* Fri May 18 2012 Rui Nuno Capela <rncbc@rncbc.org> 0.3.9
- The last of the remnants.
* Fri Jul  1 2011 Rui Nuno Capela <rncbc@rncbc.org> 0.3.8
- JACK Session versioning. 
* Tue Nov 30 2010 Rui Nuno Capela <rncbc@rncbc.org> 0.3.7
- JACK Session managerism. 
* Mon May 17 2010 Rui Nuno Capela <rncbc@rncbc.org>
- Standard desktop icon fixing. 
* Mon Jan 11 2010 Rui Nuno Capela <rncbc@rncbc.org> 0.3.6
- Full D-Busification!
* Mon Jan 11 2010 Rui Nuno Capela <rncbc@rncbc.org>
- Man page added.
* Wed Sep 30 2009 Rui Nuno Capela <rncbc@rncbc.org> 0.3.5
- Slipped away!
* Fri Dec 05 2008 Rui Nuno Capela <rncbc@rncbc.org> 0.3.4
- Patchbay snapshot revamp.
* Sat Jun 07 2008 Rui Nuno Capela <rncbc@rncbc.org> 0.3.3
- Patchbay JACK-MIDI, file logging and X11 uniqueness.
* Thu Dec 20 2007 Rui Nuno Capela <rncbc@rncbc.org> 0.3.2
- Patchbay heads-up with season greetings. 
* Thu Jul 19 2007 Rui Nuno Capela <rncbc@rncbc.org>
- System-tray tooltip icon crash fix.
* Wed Jul 18 2007 Rui Nuno Capela <rncbc@rncbc.org> 0.3.1
- Shallowed bug-fix release..
* Tue Jul 10 2007 Rui Nuno Capela <rncbc@rncbc.org> 0.3.0
- Qt4 migration was complete.
* Mon Jun 25 2007 Rui Nuno Capela <rncbc@rncbc.org>
- Application icon is now installed to (prefix)/share/pixmaps.
- Declared fundamental build and run-time requirements.
- Destination install directory prefix is now in spec.
- Spec is now a bit more openSUSE compliant.
* Wed May 31 2006 Rui Nuno Capela <rncbc@rncbc.org>
- Changed copyright to license attribute
* Wed Aug 24 2005 Rui Nuno Capela <rncbc@rncbc.org>
- Created initial qjackctl.spec
