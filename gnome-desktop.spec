%define __spec_install_post /usr/lib/rpm/brp-compress
Summary:          The gnome desktop programs for the GNOME GUI desktop environment.
Name:             gnome-desktop
Version:          2.2.0.1
Release:          1
License:          LGPL
Group:            System Environment/Base
Source:           %{name}-%{version}.tar.gz
BuildRoot:        %{_tmppath}/%{name}-%{version}-root
URL:              http://www.gnome.org
BuildRequires:    pkgconfig >= 0.8
Requires:         gtk2 >= 2.1.2
Requires:         libgnomeui >= 2.1.0
Requires:         gnome-vfs2 >= @GNOME_VFS_MODULE_REQUIRED@
Requires:         libgnomecanvas >= 2.0.0
BuildRequires:    gtk2-devel >= 2.1.2
BuildRequires:    libgnomeui-devel >= 2.1.0
BuildRequires:    gnome-vfs2-devel >= @GNOME_VFS_MODULE_REQUIRED@
BuildRequires:    libgnomecanvas-devel >= 2.0.0


%description
GNOME (GNU Network Object Model Environment) is a user-friendly
set of applications and desktop tools to be used in conjunction with a
window manager for the X Window System.  GNOME is similar in purpose and
scope to CDE and KDE, but GNOME is based completely on free
software.  The gnome-core package includes the basic programs and
libraries that are needed to install GNOME.

GNOME Desktop provides the core icons and libraries for the gnome
desktop.

%package devel
Summary:          GNOME desktop core libraries, includes, and more.
Group:            Development/Libraries
Requires:         %name = %version
Requires:         pkgconfig >= 0.8
Requires:         gtk2 >= 2.1.2
Requires:         gtk2-devel >= 2.1.2
Requires:         libgnomeui >= 2.1.0
Requires:         libgnomeui-devel >= 2.1.0
Requires:         gnome-vfs2 >= @GNOME_VFS_MODULE_REQUIRED@
Requires:         gnome-vfs2-devel >= @GNOME_VFS_MODULE_REQUIRED@
Requires:         libgnomecanvas >= 2.0.0
Requires:         libgnomecanvas-devel >= 2.0.0


%description devel
Panel libraries and header files for creating GNOME panels.

%prep
%setup -q

%build
%configure
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT


export GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1

%makeinstall

unset GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL

%find_lang %{name}-2.0

%clean
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files -f %{name}-2.0.lang
%doc AUTHORS COPYING ChangeLog NEWS README
%defattr (-, root, root)
%{_bindir}/*
%{_mandir}/man1/*
%{_libdir}/*.so.*
%{_datadir}/gnome/help/*
%{_datadir}/gnome/vfolders
%{_datadir}/gnome-about/*
%{_datadir}/pixmaps/*
%{_mandir}/man1/*
%dir %{_datadir}/gnome

%files devel
%{_includedir}/gnome-desktop-2.0
%{_libdir}/*a
%{_libdir}/*so
%{_libdir}/pkgconfig/*


%changelog
* Tue Mar 05 2002 Chris Chabot <chabotc@reviewboard.com>
- Fixed remaining formating issues
- converted to .spec.in

* Sun Feb 15 2002 Chris Chabot <chabotc@reviewboard.com>
- initial spec file for gnome-desktop
