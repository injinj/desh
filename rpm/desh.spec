Name:		desh
Version:	999.999
Release:	99999%{?dist}
Summary:	An experimental shell

Group:		System Environment/Shells
License:	BSD
URL:		https://github.com/injinj/%{name}
Source0:	%{name}-%{version}-99999.tar.gz
BuildRoot:	${_tmppath}
Prefix:	        /usr
BuildRequires:  gcc-c++
BuildRequires:  chrpath
BuildRequires:  byacc
BuildRequires:  linecook
BuildRequires:  libdecnumber
BuildRequires:  pcre2-devel
Requires:       pcre2
Requires:       linecook
Requires:       libdecnumber
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
An experimental shell derived from es and rc.

%prep
%setup -q


%define _unpackaged_files_terminate_build 0
%define _missing_doc_files_terminate_build 0
%define _missing_build_ids_terminate_build 0
%define _include_gdb_index 1

%build
make build_dir=./usr %{?_smp_mflags} dist_bins
cp -a ./include ./usr/include
mkdir -p ./usr/share/doc/%{name}
cp -a script/%{name}rc script/esrc.haahr ./usr/share/doc/%{name}/
cp -a README.md doc/CHANGES doc/es.1 ./usr/share/doc/%{name}/

%install
rm -rf %{buildroot}
mkdir -p  %{buildroot}

# in builddir
cp -a * %{buildroot}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
/usr/bin/*
/usr/lib64/*
/usr/include/*
/usr/share/doc/*

%post
echo "${RPM_INSTALL_PREFIX}/lib64" > /etc/ld.so.conf.d/%{name}.conf
cp -a "${RPM_INSTALL_PREFIX}/share/doc/%{name}/%{name}rc" /etc/%{name}rc
if [ -d "${RPM_INSTALL_PREFIX}/share/man/man1" ] ; then
  ln -s -f "${RPM_INSTALL_PREFIX}/share/doc/%{name}/es.1" "${RPM_INSTALL_PREFIX}/share/man/man1/%{name}.1"
fi
if [ $1 -eq 1 ] ; then
  if [ ! -f %{_sysconfdir}/shells ] ; then
    echo "%{_bindir}/%{name}" > %{_sysconfdir}/shells
    echo "${RPM_INSTALL_PREFIX}/bin/%{name}" >> %{_sysconfdir}/shells
  else
    grep -q "^%{_bindir}/%{name}$" %{_sysconfdir}/shells || echo "%{_bindir}/%{name}" >> %{_sysconfdir}/shells
    grep -q "^${RPM_INSTALL_PREFIX}/bin/%{name}$" %{_sysconfdir}/shells || echo "${RPM_INSTALL_PREFIX}/bin/%{name}" >> %{_sysconfdir}/shells
  fi
fi
/sbin/ldconfig

%postun
# if uninstalling
if [ $1 -eq 0 ] ; then
  rm -f /etc/ld.so.conf.d/%{name}.conf
  rm -f /etc/%{name}rc
  if [ -h "${RPM_INSTALL_PREFIX}/share/man/man1/%{name}.1" ] ; then
    rm -f "${RPM_INSTALL_PREFIX}/share/man/man1/%{name}.1"
  fi
  rm -f /usr/share/man/man1/%{name}.1
  sed -i '\!^%{_bindir}/%{name}$!d' %{_sysconfdir}/shells
  sed -i '\!^${RPM_INSTALL_PREFIX}/bin/%{name}$!d' %{_sysconfdir}/shells
fi
/sbin/ldconfig

%changelog
* __DATE__ <gchrisanderson@gmail.com>
- Hello world
