Name:		desh
Version:	999.999
Release:	99999%{?dist}
Summary:	An experimental shell

Group:		System Environment/Shells
License:	BSD
URL:		https://github.com/injinj/%{name}
Source0:	%{name}-%{version}-99999.tar.gz
BuildRoot:	${_tmppath}
BuildArch:      x86_64
Prefix:	        /usr
BuildRequires:  gcc-c++
BuildRequires:  byacc
BuildRequires:  linecook
BuildRequires:  libdecnumber
Requires:       linecook
Requires:       libdecnumber

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
cp -a script/deshrc script/esrc.haahr ./usr/share/doc/%{name}/
cp -a README.md CHANGES es.1 ./usr/share/doc/%{name}/

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
echo "${RPM_INSTALL_PREFIX}/lib64" > /etc/ld.so.conf.d/desh.conf
cp -a "${RPM_INSTALL_PREFIX}/share/doc/desh/deshrc" /etc/deshrc
ln -s -f "${RPM_INSTALL_PREFIX}/share/doc/desh/es.1" /usr/share/man/man1/desh.1
ldconfig

%postun
rm -f /etc/ld.so.conf.d/desh.conf
rm -f /etc/deshrc
rm -f /usr/share/man/man1/desh.1
ldconfig

%changelog
* __DATE__ <gchrisanderson@gmail.com>
- Hello world
