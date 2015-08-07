Name:       libmm-wfd
Summary:    Multimedia Framework Wifi-Display Library
Version:    0.2.182
Release:    0
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Requires(post):  /sbin/ldconfig
Requires(postun):  /sbin/ldconfig
BuildRequires: pkgconfig(mm-common)
BuildRequires: pkgconfig(gstreamer-1.0)
BuildRequires: pkgconfig(gstreamer-plugins-base-1.0)
BuildRequires: pkgconfig(gstreamer-video-1.0)
BuildRequires: pkgconfig(gstreamer-app-1.0)
BuildRequires: pkgconfig(iniparser)
BuildRequires: pkgconfig(capi-network-wifi-direct)
BuildRequires: pkgconfig(mm-scmirroring-common)
BuildRequires: pkgconfig(dlog)

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
export CFLAGS+=" -Wextra -Wno-array-bounds"
export CFLAGS+=" -Wno-ignored-qualifiers -Wno-unused-parameter -Wshadow"
export CFLAGS+=" -Wwrite-strings -Wswitch-default"

./autogen.sh

CFLAGS+=" -DMMFW_DEBUG_MODE -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" "; export CFLAGS
LDFLAGS+="-Wl,--rpath=%{_prefix}/lib -Wl,--hash-style=both -Wl,--as-needed"; export LDFLAGS

# always enable sdk build. This option should go away
#  --enable-wfd-sink-uibc
%configure --disable-static

# Call make instruction with smp support
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/dbus-1/services/
mkdir -p %{buildroot}/%{_datadir}/license
cp -rf %{_builddir}/%{name}-%{version}/LICENSE.APLv2.0 %{buildroot}%{_datadir}/license/%{name}

mkdir -p %{buildroot}/usr/etc
cp -rf config/mmfw_wfd_sink.ini %{buildroot}/usr/etc/mmfw_wfd_sink.ini

%clean
rm -rf %{buildroot}

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_datadir}/license/%{name}
/usr/etc/mmfw_wfd_sink.ini
%{_libdir}/*.so.*
%manifest libmm-wfd.manifest

%files devel
%defattr(-,root,root,-)
%{_libdir}/*.so
%{_includedir}/mmf/mm_wfd_sink.h
%{_libdir}/pkgconfig/*

#%files factory
#%defattr(-,root,root,-)
#%{_includedir}/mmf/mm_player_factory.h
