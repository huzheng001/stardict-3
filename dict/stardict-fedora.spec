Name:           stardict
Summary:        A powerful dictionary platform written in GTK
Version:        3.0.7
Release:        1%{?dist}
# The entire source code is GPLv3+ except
# dict/src/lib/ctype-{uca,utf8}.cpp which is GPLv2+
# dict/src/lib/ctype-uca.cpp which is GPLv2+
# dict/src/eggaccelerators.{h,cpp which is GPLv2+
# dict/stardict-plugins/stardict-wordnet-plugin/tenis.h is CPL
# refer to README
License:        GPLv3+ and GPLv2+ and CPL
URL:            http://stardict-4.sourceforge.net/
Source0:        http://downloads.sourceforge.net/%{name}-4/%{version}/%{name}-%{version}-github.tar.xz

BuildRequires: make
BuildRequires: gcc-c++
BuildRequires: /usr/bin/dos2unix
BuildRequires: desktop-file-utils
BuildRequires: enchant-devel
BuildRequires: gettext
BuildRequires: intltool
BuildRequires: gnome-doc-utils
BuildRequires: gtk3-devel
BuildRequires: libsigc++20-devel
BuildRequires: espeak-devel
BuildRequires: rarian-compat
BuildRequires: pkgconfig(libxml-2.0)
BuildRequires: perl(XML::Parser)


%description
StarDict is a cross-platform and international dictionary written in GTK.
It has powerful features such as glob-style pattern matching,
scan selection, and fuzzy matching.

%prep
%setup -q

# Remove unneeded sigc++ header files to make it sure
# that we are using system-wide libsigc++
# (and these does not work on gcc43)
find dict/src/sigc++* -name \*.h -or -name \*.cc | xargs rm -f

chmod 644 COPYING dict/doc/*
dos2unix dict/doc/stardict-textual-dict.rnc

%build
# gnome and gucharmap are gtk3 now.
# dictdotcn is outdated.
# no 'make install' for tools
# Festival in Fedora linux have some problem and it will cause festival plugin
# crash, so we need to disable it presently. -- README
%configure --disable-gnome-support \
           --disable-scrollkeeper \
           --disable-dictdotcn \
           --disable-tools \
           --disable-festival
make -k %{_smp_mflags}

%install
make DESTDIR=$RPM_BUILD_ROOT INSTALL="install -p" install

desktop-file-install --delete-original \
%if 0%{?fedora} && 0%{?fedora} < 1
  --vendor fedora \
%endif
  --dir $RPM_BUILD_ROOT%{_datadir}/applications \
  $RPM_BUILD_ROOT%{_datadir}/applications/%{name}.desktop

rm -f `find $RPM_BUILD_ROOT%{_libdir}/stardict/plugins -name "*.la"`

# remove useless files in dict/doc
rm dict/doc/{Makefile*,Readme.mac,README_windows.txt}

%find_lang %{name}


%files -f %{name}.lang
%{_bindir}/stardict
%{_datadir}/applications/*.desktop
%{_datadir}/stardict
%{_libdir}/stardict
%{_datadir}/pixmaps/stardict.png
%{_datadir}/omf/*
%{_mandir}/man1/*
# co-own
%dir %{_datadir}/gnome/help/
%doc %{_datadir}/gnome/help/stardict
%doc AUTHORS COPYING ChangeLog README dict/doc/*


%changelog
* Wed Jan 27 2021 Fedora Release Engineering <releng@fedoraproject.org> - 3.0.6-16
- Rebuilt for https://fedoraproject.org/wiki/Fedora_34_Mass_Rebuild

* Wed Jul 29 2020 Fedora Release Engineering <releng@fedoraproject.org> - 3.0.6-15
- Rebuilt for https://fedoraproject.org/wiki/Fedora_33_Mass_Rebuild

* Fri Jan 31 2020 Fedora Release Engineering <releng@fedoraproject.org> - 3.0.6-14
- Rebuilt for https://fedoraproject.org/wiki/Fedora_32_Mass_Rebuild

* Sat Jul 27 2019 Fedora Release Engineering <releng@fedoraproject.org> - 3.0.6-13
- Rebuilt for https://fedoraproject.org/wiki/Fedora_31_Mass_Rebuild

* Sun Feb 03 2019 Fedora Release Engineering <releng@fedoraproject.org> - 3.0.6-12
- Rebuilt for https://fedoraproject.org/wiki/Fedora_30_Mass_Rebuild

* Sat Jul 14 2018 Fedora Release Engineering <releng@fedoraproject.org> - 3.0.6-11
- Rebuilt for https://fedoraproject.org/wiki/Fedora_29_Mass_Rebuild

* Sat Mar 10 2018 Robin Lee <cheeselee@fedoraproject.org> - 3.0.6-10
- BR gcc-c++ for http://fedoraproject.org/wiki/Changes/Remove_GCC_from_BuildRoot

* Fri Feb 09 2018 Fedora Release Engineering <releng@fedoraproject.org> - 3.0.6-9
- Rebuilt for https://fedoraproject.org/wiki/Fedora_28_Mass_Rebuild

* Thu Aug 03 2017 Fedora Release Engineering <releng@fedoraproject.org> - 3.0.6-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Binutils_Mass_Rebuild

* Thu Jul 27 2017 Fedora Release Engineering <releng@fedoraproject.org> - 3.0.6-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Mass_Rebuild

* Mon May 15 2017 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.0.6-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_26_27_Mass_Rebuild

* Sat Feb 11 2017 Fedora Release Engineering <releng@fedoraproject.org> - 3.0.6-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_26_Mass_Rebuild

* Fri Feb 05 2016 Fedora Release Engineering <releng@fedoraproject.org> - 3.0.6-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_24_Mass_Rebuild

* Fri Jun 19 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.0.6-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_23_Mass_Rebuild

* Sat May 02 2015 Kalev Lember <kalevlember@gmail.com> - 3.0.6-2
- Rebuilt for GCC 5 C++11 ABI change

* Fri Feb 06 2015 Anish Patil <apatil@redhat.com> - 3.0.6-1
- Upstream has released new sources

* Mon Aug 18 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.0.5-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_22_Mass_Rebuild

* Sat Jun 28 2014 Robin Lee <cheeselee@fedoraproject.org> - 3.0.5-1
- Update to 3.0.5, drop upstreamed patches
- Don't run autoreconf
- License revised from 'GPLv3' to 'GPLv3+ and GPLv2+ and CPL'

* Sun Jun 08 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.0.4-9
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_Mass_Rebuild

* Wed Dec  4 2013 Robin Lee <cheeselee@fedoraproject.org> - 3.0.4-8
- Fix build with -Werror=format-security (BZ#1037338)

* Sun Aug 04 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.0.4-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_20_Mass_Rebuild

* Mon Apr  8 2013 Robin Lee <cheeselee@fedoraproject.org> - 3.0.4-6
- Re-add config.sub and config.guess to recognize aarch64 (#926572)
- BR: /usr/bin/gnome-autogen.sh and GConf2, for running autoreconf
- BR: dos2unix, to fix end-of-line

* Sun Feb 24 2013 Toshio Kuratomi <toshio@fedoraproject.org> - 3.0.4-5
- Remove --vendor from desktop-file-install on F19+. https://fedorahosted.org/fesco/ticket/1077

* Fri Feb 15 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.0.4-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Wed Jan 16 2013 Robin Lee <cheeselee@fedoraproject.org> - 3.0.4-3
- Compile with --disable-gnome-support since GNOME 2 is obsoleted
- Re-enable the espeak plugin
- Run upstream autogen.sh
- Other cleanup

* Wed Jan 16 2013 Parag Nemade <paragn AT fedoraproject DOT org> - 3.0.4-2
- Apply the configure patch inside dict and lib directory

* Wed Jan 16 2013 Parag Nemade <paragn AT fedoraproject DOT org> - 3.0.4-1
- update to 3.0.4
- thanks to Robin Lee for configure patch

* Sat Jul 21 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.0.2-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Thu Apr 19 2012 Peter Robinson <pbrobinson@fedoraproject.org> - 3.0.2-5
- Add patch for glib2 single includes

* Sat Jan 14 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.0.2-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Tue Dec 06 2011 Adam Jackson <ajax@redhat.com> - 3.0.2-3
- Rebuild for new libpng

* Fri Jun 17 2011 Jens Petersen <petersen@redhat.com> - 3.0.2-2
- turn off netdict by default again

* Thu Jun 16 2011 Jens Petersen <petersen@redhat.com> - 3.0.2-1
- update to 3.0.2
- remove obsoleted patches
- clean up .spec file and dependencies
- update schema scriptlets
- BR gnome-doc-utils

* Thu May 19 2011 Karsten Hopp <karsten@redhat.com> 3.0.1-23.1
- add patch from Jiri Skala to fix gcc46 build failure (#704248)

* Wed Feb 09 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.0.1-23
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Thu Oct 14 2010 Ding-Yi Chen <dchen at redhat.com> - 3.0.1-22
- Fixed Bz #641955: remove gucharmap dependency as gucharmap use gtk3, but stardict still use gtk2.

* Sun Dec 27 2009 Caius 'kaio' Chance <k at kaio.me> - 3.0.1-21
- rebuilt

* Sun Dec 27 2009 Caius 'kaio' Chance <k at kaio.me> - 3.0.1-20
- Disable netdict by default and set warning for security.

* Thu Dec 17 2009 Caius 'kaio' Chance <k at kaio.me> - 3.0.1-19
- Resolves: rhbz#475904: Disabled espeak for instance as espeak has problems when it is built with pulseaudio.

* Sun Jul 26 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.0.1-18
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Sat Apr 25 2009 Milos Jakubicek <xjakub@fi.muni.cz> - 3.0.1-17
- Fix FTBFS: added stardict-3.0.1.gcc44.patch

* Wed Feb 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.0.1-16
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Tue Dec  2 2008 Michael Schwendt <mschwendt@fedoraproject.org> - 3.0.1-15
- BR libtool and run libtoolize to fix build with libtool2.
- Add preun scriptlet for GConf2 uninstall rule.
- Build with SMP make flags.
- Install with -p.

* Fri Aug 29 2008 Michael Schwendt <mschwendt@fedoraproject.org> - 3.0.1-14.fc10
- Include /etc/stardict directory

* Mon Aug 04 2008 Caius Chance <cchance@redhat.com> - 3.0.1-13.fc10
- Resolves: rhbz#441209
  - Enable appropriate locale based dictionaries in user's first time usage.

* Thu Jul 17 2008 Caius Chance <cchance@redhat.com> - 3.0.1-12.fc10
- Resolves: rhbz#455685 (Broken dependency of bonobo-activitation.)

* Mon Jul 14 2008 Caius Chance <cchance@redhat.com> - 3.0.1-11.1.fc10
- Enable gucharmap-2.

* Mon Jul 14 2008 Caius Chance <cchance@redhat.com> - 3.0.1-11.fc10
- Disable gucharmap for incompatibility with gucharmap-2.
- Refactorized Requires and BuildRequires tags.

* Thu Jul 10 2008 Caius Chance <cchance@redhat.com> - 3.0.1-10.fc10
- Rebuilt for gucharmap updation 2.22.1 and pkgconfig .ac name change.

* Mon Jun 30 2008 Caius Chance <cchance@redhat.com> - 3.0.1-9.fc10
- Fixed broken dependencies with gucharmap.

* Fri Feb 29 2008 Hu Zheng <zhu@redhat.com> - 3.0.1-8
- Forget commit first.

* Fri Feb 29 2008 Hu Zheng <zhu@redhat.com> - 3.0.1-7
- Add trayicon transparent patch.

* Thu Feb 28 2008 Hu Zheng <zhu@redhat.com> - 3.0.1-6
- OK

* Wed Feb 27 2008 Mamoru Tasaka <mtasaka@ioa.s.u-tokyo.ac.jp> - 3.0.1-5
- More gcc43 fix

* Wed Feb 27 2008 Hu Zheng <zhu@redhat.com> - 3.0.1-4
- small fix.

* Tue Feb 26 2008 Hu Zheng <zhu@redhat.com> - 3.0.1-3
- Gcc-4.3 compile fix.

* Wed Feb 20 2008 Fedora Release Engineering <rel-eng@fedoraproject.org> - 3.0.1-2
- Autorebuild for GCC 4.3

* Thu Nov 08 2007 Hu Zheng <zhu@redhat.com> - 3.0.1-1
- Update to upstream.

* Mon Sep 03 2007 Hu Zheng <zhu@redhat.com> - 3.0.0-4
- Small fix on spec file. Use desktop-file-install.

* Thu Aug 23 2007 Hu Zheng <zhu@redhat.com> - 3.0.0-3
- Add floatwin and espeak patch.

* Mon Aug 13 2007 Hu Zheng <zhu@redhat.com> - 3.0.0-1
- Update to upstream.

* Fri Jun 22 2007 Hu Zheng <zhu@redhat.com> - 2.4.8-3
- Add dic and treedict directory.

* Tue Jan 16 2007 Mayank Jain <majain@redhat.com>
- Removed gnome support from the spec file (--disable-gnome-support) for bug 213850
- Commented the gnome related directives.

* Fri Jan 12 2007 Mayank Jain <majain@redhat.com>
- Added perl-XML-Parser as BuildRequires

* Thu Jan 11 2007 Mayank Jain <majain@redhat.com>
* Thu Jan 11 2007 Mayank Jain <majain@redhat.com>
- Updated to version 2.4.8
- Removed invalid patch - stardict-2.4.5-invalid-cplusplus.patch
- Reset release number to 2.4.8-1
- Added dist-tag to the release version directive

* Tue Jul 18 2006 Jesse Keating <jkeating@redhat.com> - 2.4.5-5
- rebuild
- add gettext as br

* Mon Jun 12 2006 Mayank Jain <majain@redhat.com>
- Updated package description

* Mon Jun 05 2006 Jesse Keating <jkeating@redhat.com>
- Added missing BuildRequires scrollkeeper
- Added Requires(post) and (postun) accordingly

* Tue Apr 18 2006 Mayank Jain <majain@redhat.com>
- Corrected spelling mistakes in the description section, RH bug #161777

* Fri Feb 10 2006 Jesse Keating <jkeating@redhat.com> - 2.4.5-2.2
- bump again for double-long bug on ppc(64)

* Tue Feb 07 2006 Jesse Keating <jkeating@redhat.com> - 2.4.5-2.1
- rebuilt for new gcc4.1 snapshot and glibc changes

* Fri Jan 13 2006 Leon Ho <llch@redhat.com> 2.4.5-2
- added in patch to fix #176890

* Fri Dec 09 2005 Jesse Keating <jkeating@redhat.com>
- rebuilt

* Mon Sep 19 2005 Leon Ho <llch@redhat.com> 2.4.5-1
- Upgraded to 2.4.5

* Thu Mar 17 2005 Leon Ho <llch@redhat.com> 2.4.4-3
- rebuilt

* Thu Feb 03 2005 Leon Ho <llch@redhat.com> 2.4.4-2
- Reupdate the spec file
- Upgrade to 2.4.4

* Sun Nov 28 2004 Hu Zheng  <huzheng_001@163.com> 2.4.4
- Public release of StarDict 2.4.4.

* Thu Feb 19 2004 Hu Zheng <huzheng_001@163.com> 2.4.3
- Public release of StarDict 2.4.3.

* Sat Nov 15 2003 Hu Zheng <huzheng_001@163.com> 2.4.2
- Public release of StarDict 2.4.2.

* Sun Sep 28 2003 Hu Zheng <huzheng_001@163.com> 2.4.1
- Public release of StarDict 2.4.1.

* Thu Aug 28 2003 Hu Zheng <huzheng_001@163.com> 2.4.0
- Public release of StarDict 2.4.0.

* Sat Jun 28 2003 Hu Zheng <huzheng_001@163.com> 2.2.1
- Public release of StarDict 2.2.1.

* Sun Jun 01 2003 Hu Zheng <huzheng_001@163.com> 2.2.0
- Public release of StarDict 2.2.0.

* Sun May 18 2003 Hu Zheng <huzheng_001@163.com> 2.1.0
- Public release of StarDict 2.1.0.

* Fri May 02 2003 Hu Zheng <huzheng_001@163.com> 2.0.0
- Public release of StarDict 2.0.0.

* Wed Apr  9 2003 Hu Zheng <huzheng_001@163.com> 2.0.0-pre2
- Second public preview release of StarDict 2.

* Sun Mar 30 2003 Hu Zheng <huzheng_001@163.com> 2.0.0-pre1
- First public preview release of StarDict 2.
