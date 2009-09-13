# ----------------------------------------------------------------
# Supported rpmbuild options:
#     --without opt     Build with DEBUG settings.
# ----------------------------------------------------------------

# Read: If neither macro exists, then add the default definition.
%{!?_with_opt: %{!?_without_opt: %define _with_opt --with-opt}}

# Read: It's an error if both or neither required options exist.
%{?_with_opt: %{?_without_opt: %{error: both _with_opt and _without_opt}}}

Summary: utopfs filesystem
Name: utopfs
Version: 0.3.devel
Release: 1%{?dist}
License: Something
Group: System Environment/Base
URL: http://www.utopfs.com/
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
The utopfs filesystem uses digest based block references to persist
filesystem data in non-traditional block devices. Since the block
references do not specify specific storage locations they are more
flexible with respect to mirroring, snapshots, cloud storage, storage
hierarchies etc.

BuildRequires: ace-devel
BuildRequires: gcc-c++
BuildRequires: libs3-devel
BuildRequires: python-devel
BuildRequires: python-py python-setuptools
BuildRequires: protobuf protobuf-devel protobuf-compiler protobuf-debuginfo
BuildRequires: fuse-devel
BuildRequires: openssl-devel
BuildRequires: db4-cxx db4-devel

# ---------------- fuse-utopfs

%package -n fuse-utopfs
Summary: FUSE-Filesystem for utopfs
Group: System Environment/Base
Requires: ace libs3

%description -n fuse-utopfs
This is a FUSE-filesystem client which mounts a utopfs filesystem.

# ----------------------------------------------------------------
# Common Stuff
# ----------------------------------------------------------------

%if %{?_with_opt:0}%{!?_with_opt:1}
%define BUILD   DEBUG
%define OBJDIR  Linux.DBGOBJ
%endif

%if %{?_without_opt:0}%{!?_without_opt:1}
%define BUILD   RELEASE
%define OBJDIR  Linux.OPTOBJ
%endif

%prep
%setup -q -n %{name}

%build
make %{?_smp_mflags} BUILD=%{BUILD}

%install
rm -rf $RPM_BUILD_ROOT

# Make a new build root.
mkdir -p $RPM_BUILD_ROOT

# Install libraries.
mkdir -p $RPM_BUILD_ROOT%{_libdir}
install \
  libutp/src/%{OBJDIR}/libUTPFS-utp.so \
$RPM_BUILD_ROOT%{_libdir}

# Install modules.
mkdir -p $RPM_BUILD_ROOT%{_libdir}
install \
  modules/DefaultLogger/%{OBJDIR}/UTPFS-DFLG.so \
  modules/FSBlockStore/%{OBJDIR}/UTPFS-FSBS.so \
  modules/BDBBlockStore/%{OBJDIR}/UTPFS-BDBBS.so \
  modules/S3BlockStore/%{OBJDIR}/UTPFS-S3BS.so \
  modules/VBlockStore/%{OBJDIR}/UTPFS-VBS.so \
  modules/UTFileSystem/%{OBJDIR}/UTPFS-UTFS.so \
$RPM_BUILD_ROOT%{_libdir}

# Make symbolic links with "lib" prefix.
mkdir -p $RPM_BUILD_ROOT%{_libdir}
pushd $RPM_BUILD_ROOT%{_libdir}
  ln -sf UTPFS-DFLG.so libUTPFS-DFLG.so
  ln -sf UTPFS-FSBS.so libUTPFS-FSBS.so
  ln -sf UTPFS-BDBBS.so libUTPFS-BDBBS.so
  ln -sf UTPFS-S3BS.so libUTPFS-S3BS.so
  ln -sf UTPFS-VBS.so libUTPFS-VBS.so
  ln -sf UTPFS-UTFS.so libUTPFS-UTFS.so
popd

# Install binaries.
mkdir -p $RPM_BUILD_ROOT%{_bindir}
install \
  utopfs/%{OBJDIR}/utopfs \
  utpcmd/%{OBJDIR}/utp \
$RPM_BUILD_ROOT%{_bindir}

# Install configuration
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig/utopfs
install \
  utopfs/Linux.INSTCFG/utopfs.conf \
  utpcmd/Linux.INSTCFG/utp.conf \
$RPM_BUILD_ROOT%{_sysconfdir}/sysconfig/utopfs

%clean
rm -rf $RPM_BUILD_ROOT

# ---------------- fuse-utopfs

%pre -n fuse-utopfs

%post -n fuse-utopfs
/sbin/ldconfig

%preun -n fuse-utopfs

%postun -n fuse-utopfs
/sbin/ldconfig

%files -n fuse-utopfs
%defattr(-,root,root,-)
%doc

%{_libdir}/libUTPFS-utp.so

%{_libdir}/UTPFS-DFLG.so
%{_libdir}/UTPFS-FSBS.so
%{_libdir}/UTPFS-BDBBS.so
%{_libdir}/UTPFS-S3BS.so
%{_libdir}/UTPFS-VBS.so
%{_libdir}/UTPFS-UTFS.so

%{_libdir}/libUTPFS-DFLG.so
%{_libdir}/libUTPFS-FSBS.so
%{_libdir}/libUTPFS-BDBBS.so
%{_libdir}/libUTPFS-S3BS.so
%{_libdir}/libUTPFS-VBS.so
%{_libdir}/libUTPFS-UTFS.so

%{_bindir}/utopfs
%{_bindir}/utp

%dir %{_sysconfdir}/sysconfig/utopfs
%config(noreplace) %{_sysconfdir}/sysconfig/utopfs/utopfs.conf
%config(noreplace) %{_sysconfdir}/sysconfig/utopfs/utp.conf

%doc README.txt

%changelog
* Tue Jun 16 2009 Ken Sedgwick <ksedgwic@lap3.bonsai.com> - 
- Initial build.

