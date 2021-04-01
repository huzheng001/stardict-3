%define dicname stardict-english-czech
Name: stardict-dic-cs_CZ
Summary: Czech dictionaries for StarDict
Version: 20150213
Release: 14%{?dist}
License: GFDL
Provides: stardict-dic-cs = %{?epoch:%{epoch}:}%{version}-%{release}

URL: http://cihar.com/software/slovnik/
Source0: http://dl.cihar.com/slovnik/stable/%{dicname}-%{version}.tar.gz

BuildArch: noarch
Requires: stardict
Supplements: (stardict and langpacks-cs)

%description
Czech-English and English-Czech translation dictionaries for StarDict, a
GUI-based dictionary software.

%prep
%setup -q -c -n %{dicname}-%{version}

%build

%install
install -m 0755 -p -d ${RPM_BUILD_ROOT}%{_datadir}/stardict/dic
install -m 0644 -p  %{dicname}-%{version}/cz* ${RPM_BUILD_ROOT}%{_datadir}/stardict/dic/
install -m 0644 -p  %{dicname}-%{version}/en* ${RPM_BUILD_ROOT}%{_datadir}/stardict/dic/

%files
%doc %{dicname}-%{version}/README
%{_datadir}/stardict/dic/*

%changelog
* Wed Jan 27 2021 Fedora Release Engineering <releng@fedoraproject.org> - 20150213-14
- Rebuilt for https://fedoraproject.org/wiki/Fedora_34_Mass_Rebuild

* Wed Jul 29 2020 Fedora Release Engineering <releng@fedoraproject.org> - 20150213-13
- Rebuilt for https://fedoraproject.org/wiki/Fedora_33_Mass_Rebuild

* Fri Jan 31 2020 Fedora Release Engineering <releng@fedoraproject.org> - 20150213-12
- Rebuilt for https://fedoraproject.org/wiki/Fedora_32_Mass_Rebuild

* Sat Jul 27 2019 Fedora Release Engineering <releng@fedoraproject.org> - 20150213-11
- Rebuilt for https://fedoraproject.org/wiki/Fedora_31_Mass_Rebuild

* Sun Feb 03 2019 Fedora Release Engineering <releng@fedoraproject.org> - 20150213-10
- Rebuilt for https://fedoraproject.org/wiki/Fedora_30_Mass_Rebuild

* Sat Jul 14 2018 Fedora Release Engineering <releng@fedoraproject.org> - 20150213-9
- Rebuilt for https://fedoraproject.org/wiki/Fedora_29_Mass_Rebuild

* Fri Feb 09 2018 Fedora Release Engineering <releng@fedoraproject.org> - 20150213-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_28_Mass_Rebuild

* Thu Jul 27 2017 Fedora Release Engineering <releng@fedoraproject.org> - 20150213-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Mass_Rebuild

* Sat Feb 11 2017 Fedora Release Engineering <releng@fedoraproject.org> - 20150213-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_26_Mass_Rebuild

* Mon Feb 22 2016 Parag Nemade <pnemade AT redhat DOT com> - 20150213-5
- Add Supplements: for langpacks namimg guidelines
- Clean the specfile to follow current packaging guidelines

* Fri Feb 05 2016 Fedora Release Engineering <releng@fedoraproject.org> - 20150213-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_24_Mass_Rebuild

* Fri Jun 19 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 20150213-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_23_Mass_Rebuild

* Mon Mar 30 2015 Petr Špaček <pspacek@redhat.com> - 20150213-2
- Add virtual provide for langpack support (bug #1206899)

* Sun Feb 22 2015 Petr Špaček <pspacek@redhat.com> - 20150213-1
- New version of dictionary (bug #1187871)

* Sun Jun 08 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 20110801-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_Mass_Rebuild

* Sun Aug 04 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 20110801-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_20_Mass_Rebuild

* Fri Feb 15 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 20110801-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Sat Jul 21 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 20110801-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Sat Jan 14 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 20110801-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Tue Aug 30 2011 Petr Sklenar <psklenar@redhat.com> - 20110801-1
- New version of dictionary (bug #727200)
- Remove stardict requires (bug #727191)
- Source was changed to the http place

* Wed Feb 09 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 20100216-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Tue Feb 16 2010 Petr Sklenar <psklenar@redhat.com> - 20100216-1
- New version of dictionary

* Sun Jul 26 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 20081201-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Mon Feb 9 2009 Petr Sklenar <psklenar@redhat.com> - 20081201-2
- editing specfile - name and description

* Mon Jan 26 2009 Petr Sklenar <psklenar@redhat.com> - 20081201-1
- Initial build for Fedora

