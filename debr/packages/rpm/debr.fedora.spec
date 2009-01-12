%define name debr
%define version 0.9.6-pre20090112
%define release fedora
%define _topdir %(echo $PWD)/rpm

Name: %{name}
Version: %{version}
Release: %{release}
License: GPL
Group: Application/Misc
URL: http://groups.google.com/group/gebr
Packager: Bráulio Barros de Oliveira <brauliobo@gmail.com>
Source: %{name}-%{version}.tar.gz
Requires: libgebr, glib2, gtk2
BuildRoot: /tmp/%{name}-%{version}
Summary: Flow editor to seismic processing software.
%description
GeBR is an environment to seismic processing based on open-source
technologies, designed to easily assembly and run processing flows.

What GeBR does:

    * Handle projects and lines
    * Assembly and run processing flows
    * Act as an interface to many freely-available seismic-processing
      packages (Seismic Unix, Madagascar, etc)

GeBRME is the flow editor to seismic processing software.

%prep
%setup -q

%configure

%build
make -j 4

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}

%makeinstall

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_bindir}/*
%{_datadir}/%{name}/*

%changelog
