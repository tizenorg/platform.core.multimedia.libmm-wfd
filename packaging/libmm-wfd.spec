
Name:       libmm-wfd
Summary:    Multimedia Framework Wifi-Display Library
Version:    0.2.16
Release:    4
Group:      System/Libraries
License:    Apache License 2.0
Source0:    %{name}-%{version}.tar.gz
Requires(post):  /sbin/ldconfig
Requires(postun):  /sbin/ldconfig
BuildRequires:  pkgconfig(mm-ta)
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(gstreamer-0.10)
BuildRequires:  pkgconfig(gstreamer-plugins-base-0.10)
BuildRequires:  pkgconfig(gstreamer-interfaces-0.10)
BuildRequires:  pkgconfig(gstreamer-app-0.10)
BuildRequires:  pkgconfig(iniparser)
BuildRequires:  pkgconfig(libcrypto)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(dbus-glib-1)
BuildRequires:	pkgconfig(x11)
BuildRequires:	pkgconfig(xdmcp)
BuildRequires:	pkgconfig(xext)
BuildRequires:	pkgconfig(xfixes)
BuildRequires:	pkgconfig(libdrm)
BuildRequires:	pkgconfig(dri2proto)
BuildRequires:	pkgconfig(libdri2)
BuildRequires:	pkgconfig(utilX)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(gst-rtsp-server-wfd)

BuildRoot:  %{_tmppath}/%{name}-%{version}-build

%description

%package devel
Summary:    Multimedia Framework Wifi-Display Library (DEV)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel

%package factory
Summary:    Multimedia Framework Wifi-Display Library (Factory)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description factory

%prep
%setup -q

%build

./autogen.sh

CFLAGS+=" -DMMFW_DEBUG_MODE -DGST_EXT_TIME_ANALYSIS -DAUDIO_FILTER_EFFECT -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" "; export CFLAGS
LDFLAGS+="-Wl,--rpath=%{_prefix}/lib -Wl,--hash-style=both -Wl,--as-needed"; export LDFLAGS

# always enable sdk build. This option should go away
./configure --enable-sdk --prefix=%{_prefix} --disable-static

# Call make instruction with smp support
#make %{?jobs:-j%jobs}
make

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/dbus-1/services/
install -m 755 com.samsung.wfd.server.service %{buildroot}/usr/share/dbus-1/services/
mkdir -p %{buildroot}/%{_datadir}/license
cp -rf %{_builddir}/%{name}-%{version}/LICENSE.APLv2.0 %{buildroot}%{_datadir}/license/%{name}

mkdir -p %{buildroot}/usr/etc
mkdir -p %{buildroot}/opt/etc
cp -rf config/mmfw_wfd.ini %{buildroot}/usr/etc/mmfw_wfd.ini
cp -rf config/mmfw_wfd.ini %{buildroot}/opt/etc/mmfw_wfd.ini

%clean
rm -rf %{buildroot}

%post
/sbin/ldconfig
/usr/bin/vconftool set -t int memory/wifi/miracast/source_status "0" -i -f

%postun
/sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_datadir}/dbus-1/services/com.samsung.wfd.server.service
%{_datadir}/license/%{name}
/usr/etc/mmfw_wfd.ini
/opt/etc/mmfw_wfd.ini
%{_libdir}/*.so.*
%{_bindir}/*
%manifest libmm-wfd.manifest

%files devel
%defattr(-,root,root,-)
%{_libdir}/*.so
%{_includedir}/mmf/mm_wfd.h
%{_includedir}/mmf/mm_wfd_proxy.h
%{_includedir}/mmf/wfd-stub.h
%{_includedir}/mmf/wfd-structure.h
%{_libdir}/pkgconfig/*

#%files factory
#%defattr(-,root,root,-)
#%{_includedir}/mmf/mm_player_factory.h





