Name:           dde-app-services
Version:        1.0.25
Release:        1
Summary:        dde-app-services provides a service collection of DDE applications, including dconfig-center.
License:        LGPL-3.0-or-later
URL:            https://github.com/linuxdeepin/dde-app-services
Source0:        %{name}-%{version}.orig.tar.xz

BuildRequires: cmake
BuildRequires: qt5-devel
BuildRequires: dtkcore-devel
BuildRequires: dtkgui-devel
BuildRequires: dtkwidget-devel
BuildRequires: gtest-devel

%description
deepin desktop-environment - dde-dconfig-daemon module
 dde-dconfig-daemon provids dbus service for reading and writing DSG configuration.

%prep
%setup -q -n %{name}-%{version}

%build
export PATH=%{_qt5_bindir}:$PATH
sed -i "s|^cmake_minimum_required.*|cmake_minimum_required(VERSION 3.0)|" $(find . -name "CMakeLists.txt")
mkdir build && pushd build
%cmake -DCMAKE_BUILD_TYPE=Release ../  -DDTK_VERSION=5 -DVERSION=%{version}
%make_build
popd

%install
%make_install -C build INSTALL_ROOT="%buildroot"

%pre
getent group dde-dconfig-daemon >/dev/null || groupadd -r dde-dconfig-daemon
getent passwd dde-dconfig-daemon >/dev/null || \
    useradd -r -g dde-dconfig-daemon -d %{_sharedstatedir}/dde-dconfig-daemon\
    -s /sbin/nologin \
    -c "User of dde-dconfig-daemon" dde-dconfig-daemon
exit 0

%files
%{_bindir}/dde-dconfig
%{_bindir}/dde-dconfig-daemon
%{_bindir}/dde-dconfig-editor
%attr(755,root,root) %{_bindir}/dde-dconfig

%{_datadir}/bash-completion/completions/*
%{_datadir}/zsh/vendor-completions/*
%{_datadir}/dbus-1/system.d/*
%{_datadir}/dbus-1/sinterfaces/*
%{_datadir}/dbus-1/system-services/*
%{_datadir}/dde-dconfig/translations/*
%{_datadir}/dde-dconfig-editor/translations/*

%changelog
* Fri Jan 19 2024 YeShanShan <yeshanshan@deepin.org> - 1.0.25
- Update to 1.0.25
