%define release 1
%define prefix  /usr
%define name	stardict-tools
%define version 2.4.8


Summary: 	StarDict-Editor
Name:		%{name}
Version:    %{version}
Release:	%{release}
License: 	GPL
Vendor:		GNOME
URL: 		http://stardict-4.sourceforge.net
Group: 		Applications/System
Source0:	%{name}-%{version}.tar.bz2
Packager:       Hu Zheng <huzheng_001@163.com>
BuildRoot:	%{_builddir}/%{name}-%{version}-root

BuildRequires: gtk2 >= 2.6.0


Requires: gtk2 >= 2.6.0

Docdir:         %{prefix}/share/doc

%description
A simple tool for StarDict.

%prep
%setup

%build

%configure
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-, root, root)
%{_bindir}/stardict-editor


%post

%postun

