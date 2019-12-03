Name:           illacceptanything
Version:        v0.1.0-rms-2015.04.08
Release:        1%{?dist}
Summary:        Tool

Group:          illacceptanything
License:        WTFPL
URL:            https://github.com/illacceptanything/illacceptanything
Source0:        https://github.com/illacceptanything/illacceptanything.git
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  docker sun-java oracle-java java-1.8.0-openjdk golang
Requires:       docker sun-java oracle-java java-1.8.0-openjdk golang kde-libs xorg-x11-drv-nvidia-libs xorg-x11-drv-nvidia-kmodsrc

%description
I see you runnin'
Deceiver chased away
A long time comin'


%prep
%setup -q


%build
%configure
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc
lolnope



%changelog
* Fri Aug 30 2019 David Roble <robled@failback.org> v0.1.0-rms-2015.04.08-1
- Initial release of Fear Inoculum
