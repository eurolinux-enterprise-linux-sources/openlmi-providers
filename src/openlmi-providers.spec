%global logfile %{_localstatedir}/log/openlmi-install.log
%global required_konkret_ver 0.9.0-2

Name:           openlmi-providers
Version:        0.4.1
Release:        1%{?dist}
Summary:        Set of basic CIM providers

License:        LGPLv2+
URL:            http://fedorahosted.org/openlmi/
Source0:        http://fedorahosted.org/released/openlmi-providers/%{name}-%{version}.tar.gz

# Upstream name has been changed from cura-providers to openlmi-providers
Provides:       cura-providers = %{version}-%{release}
Obsoletes:      cura-providers < 0.0.10-1

BuildRequires:  cmake
BuildRequires:  konkretcmpi-devel >= %{required_konkret_ver}
BuildRequires:  sblim-cmpi-devel
BuildRequires:  cim-schema
# For openlmi-fan
BuildRequires:  lm_sensors-devel
# For openlmi-account
BuildRequires:  libuser-devel
BuildRequires:  python2-devel
# for openlmi-*-doc packages
BuildRequires:  konkretcmpi-python >= %{required_konkret_ver}
BuildRequires:  python-sphinx
# For openlmi-hardware
BuildRequires:  pciutils-devel
# For openlmi-logicalfile
BuildRequires:  libudev-devel
BuildRequires:  libselinux-devel
# For openlmi-mof-register script
Requires:       python2
# for openlmi-journald
BuildRequires:  systemd-devel
# sblim-sfcb or tog-pegasus
# (required to be present during install/uninstall for registration)
Requires:       cim-server
Requires(pre):  cim-server
Requires(preun): cim-server
Requires(post): cim-server
Requires:       pywbem
Requires(pre):  pywbem
Requires(preun): pywbem
Requires(post):  pywbem
Requires:       cim-schema

# XXX
# Just because we have wired python's scripts
# Remove in future
BuildRequires:  python-setuptools

%description
%{name} is set of (usually) small CMPI providers (agents) for basic
monitoring and management of host system using Common Information
Model (CIM).

%package devel
Summary:        Development files for %{name}
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       konkretcmpi-python >= %{required_konkret_ver}
Provides:       cura-providers-devel = %{version}-%{release}
Obsoletes:      cura-providers-devel < 0.0.10-1

%description devel
%{summary}.

%package -n openlmi-fan
Summary:        CIM provider for controlling fans
Requires:       %{name}%{?_isa} = %{version}-%{release}
Provides:       cura-fan = %{version}-%{release}
Obsoletes:      cura-fan < 0.0.10-1

%description -n openlmi-fan
%{summary}.

%package -n openlmi-fan-doc
Summary:        CIM fan provider documentation
Group:          Documentation
BuildArch:      noarch

%description -n openlmi-fan-doc
This package contains the documents for OpenLMI fan provider.

%package -n openlmi-powermanagement
Summary:        Power management CIM provider
Requires:       %{name}%{?_isa} = %{version}-%{release}
Provides:       cura-powermanagement = %{version}-%{release}
Obsoletes:      cura-powermanagement < 0.0.10-1

%description -n openlmi-powermanagement
%{summary}.

%package -n openlmi-powermanagement-doc
Summary:        Power management CIM provider documentation
Group:          Documentation
BuildArch:      noarch

%description -n openlmi-powermanagement-doc
This package contains the documents for OpenLMI power management provider.

%package -n openlmi-service
Summary:        CIM provider for controlling system services
Requires:       %{name}%{?_isa} = %{version}-%{release}
Provides:       cura-service = %{version}-%{release}
Obsoletes:      cura-service < 0.0.10-1

%description -n openlmi-service
%{summary}.

%package -n openlmi-service-doc
Summary:        CIM service provider documentation
Group:          Documentation
BuildArch:      noarch

%description -n openlmi-service-doc
This package contains the documents for OpenLMI service provider.

%package -n openlmi-account
Summary:        CIM provider for managing accounts on system
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       openlmi-indicationmanager-libs%{?_isa} = %{version}-%{release}
Provides:       cura-account = %{version}-%{release}
Obsoletes:      cura-account < 0.0.10-1

%description -n openlmi-account
%{summary}.

%package -n openlmi-account-doc
Summary:        CIM account provider documentation
Group:          Documentation
BuildArch:      noarch

%description -n openlmi-account-doc
This package contains the documents for OpenLMI account provider.

%package -n openlmi-hardware
Summary:        CIM provider for hardware on system
Requires:       %{name}%{?_isa} = %{version}-%{release}
# For Hardware information
%ifarch %{ix86} x86_64 ia64
Requires:       dmidecode
%endif
Requires:       util-linux

%description -n openlmi-hardware
%{summary}.

%package -n openlmi-hardware-doc
Summary:        CIM hardware provider documentation
Group:          Documentation
BuildArch:      noarch

%description -n openlmi-hardware-doc
This package contains the documents for OpenLMI hardware provider.

%package -n openlmi-python-base
Summary:        Python namespace package for OpenLMI python projects
Requires:       python-setuptools
Requires:       cmpi-bindings-pywbem
BuildArch:      noarch
Obsoletes:      openlmi-python < 0.1.0-1
Provides:       openlmi-python = %{version}-%{release}

%description -n openlmi-python-base
The openlmi-python-base package contains python namespace package
for all OpenLMI related projects running on python.

%package -n openlmi-python-providers
Summary:        Python namespace package for pywbem providers
Requires:       %{name} = %{version}-%{release}
Requires:       openlmi-python-base = %{version}-%{release}
BuildArch:      noarch

%description -n openlmi-python-providers
The openlmi-python-providers package contains library with common
code for implementing CIM providers using cmpi-bindings-pywbem.

%package -n openlmi-software
Summary:        CIM providers for software management
Requires:       %{name} = %{version}-%{release}
Requires:       openlmi-python-providers = %{version}-%{release}
Provides:       cura-software = %{version}-%{release}
Obsoletes:      cura-software < 0.0.10-1
BuildArch:      noarch

Requires:       yum

%description -n openlmi-software
The openlmi-software package contains CMPI providers for management of software
through yum package manager with Common Information Managemen (CIM) protocol.

The providers can be registered in any CMPI-aware CIMOM, both OpenPegasus and
SFCB were tested.

%package -n openlmi-software-doc
Summary:        CIM software provider documentation
Group:          Documentation
BuildArch:      noarch

%description -n openlmi-software-doc
This package contains the documents for OpenLMI software provider.

%package -n openlmi-logicalfile
Summary:        CIM provider for reading files and directories
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n openlmi-logicalfile
%{summary}.

%package -n openlmi-logicalfile-doc
Summary:        CIM logicalfile provider documentation
Group:          Documentation
BuildArch:      noarch

%description -n openlmi-logicalfile-doc
This package contains the documents for OpenLMI logicalfile provider.

%package -n openlmi-realmd
Summary:        CIM provider for Realmd
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       realmd

%description -n openlmi-realmd
The openlmi-realmd package contains CMPI providers for Realmd, which is an on
demand system DBus service, which allows callers to configure network 
authentication and domain membership in a standard way.

%package -n openlmi-realmd-doc
Summary:        CIM Realmd provider documentation
Group:          Documentation
BuildArch:      noarch

%description -n openlmi-realmd-doc
This package contains the documents for OpenLMI Realmd provider.

%package -n openlmi-indicationmanager-libs
Summary:        Libraries for CMPI indication manager
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n openlmi-indicationmanager-libs
%{summary}.

%package -n openlmi-indicationmanager-libs-devel
Summary:        Development files for openlmi-indicationmanager-libs
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       openlmi-indicationmanager-libs%{_isa} = %{version}-%{release}

%description -n openlmi-indicationmanager-libs-devel
%{summary}.

%package -n openlmi-pcp
Summary:        pywbem providers for accessing PCP metrics
Requires:       %{name} = %{version}-%{release}
BuildArch:      noarch
Requires:       python-setuptools
Requires:       cmpi-bindings-pywbem
Requires:       python-pcp

%description -n openlmi-pcp
openlmi-pcp exposes metrics from a local PMCD (Performance Co-Pilot server)
to the CIMOM.  They appear as potentially hundreds of MOF classes, e.g. 
class "PCP_Metric_kernel__pernode__cpu__use", with instances for each PCP
metric instance, e.g. "node0".  PCP metric values and metadata are transcribed
into strings on demand.

%package -n openlmi
Summary:        OpenLMI managed system software components
Requires:       %{name} = %{version}-%{release}
BuildArch:      noarch
Requires:       tog-pegasus
# List of "safe" providers
Requires:       openlmi-storage
Requires:       openlmi-networking
Requires:       openlmi-hardware
Requires:       openlmi-software
Requires:       openlmi-powermanagement
Requires:       openlmi-account
Requires:       openlmi-service

%description -n openlmi
OpenLMI provides a common infrastructure for the management of Linux systems.
This package installs a core set of OpenLMI providers and necessary
infrastructure packages enabling the system to be managed remotely.

%package -n python-sphinx-theme-openlmi
Summary:        OpenLMI theme for Sphinx documentation generator
Requires:       python-sphinx
BuildArch:      noarch

%description -n python-sphinx-theme-openlmi
python-sphinx-theme-openlmi contains Sphinx theme for OpenLMI provider
documentation.

%package -n openlmi-journald
Summary:        CIM provider for Journald
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n openlmi-journald
The openlmi-journald package contains CMPI providers for systemd journald
service, allowing listing, iterating through and writing new message log
records.

%package -n openlmi-journald-doc
Summary:        CIM Journald provider documentation
Group:          Documentation
BuildArch:      noarch

%description -n openlmi-journald-doc
This package contains the documents for OpenLMI Journald provider.

%prep
%setup -q

%build
mkdir -p %{_target_platform}
pushd %{_target_platform}
%{cmake} ..
popd

make -k %{?_smp_mflags} -C %{_target_platform} all doc

pushd src/python
%{__python} setup.py build
popd # src/python
# for software providers
pushd src/software
%{__python} setup.py build
popd # src/software
pushd src/pcp
%{__python} setup.py build
popd

%install
make install/fast DESTDIR=$RPM_BUILD_ROOT -C %{_target_platform}

# The log file must be created
mkdir -p "$RPM_BUILD_ROOT/%{_localstatedir}/log"
touch "$RPM_BUILD_ROOT/%logfile"

# XXX
# Remove pythonies
# Don't forget to remove this dirty hack in the future
rm -rf "$RPM_BUILD_ROOT"/usr/bin/*.py
rm -rf "$RPM_BUILD_ROOT"/usr/lib/python*

pushd src/python
%{__python} setup.py install -O1 --skip-build --root $RPM_BUILD_ROOT
cp -p lmi/__init__.* $RPM_BUILD_ROOT%{python2_sitelib}/lmi
popd # src/python

# for software providers
pushd src/software
%{__python} setup.py install -O1 --skip-build --root $RPM_BUILD_ROOT
install -m 755 -d $RPM_BUILD_ROOT/%{_libexecdir}/pegasus
install -m 755 pycmpiLMI_Software-cimprovagt $RPM_BUILD_ROOT/%{_libexecdir}/pegasus/
popd # src/software
cp mof/LMI_Software.reg $RPM_BUILD_ROOT/%{_datadir}/%{name}/

# pcp
pushd src/pcp
%{__python} setup.py install -O1 --skip-build --root $RPM_BUILD_ROOT
popd
cp -p src/pcp/openlmi-pcp-generate $RPM_BUILD_ROOT/%{_bindir}/openlmi-pcp-generate
mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/cron.daily
cp -p src/pcp/openlmi-pcp.cron $RPM_BUILD_ROOT/%{_sysconfdir}/cron.daily/openlmi-pcp
sed -i -e 's,^_LOCALSTATEDIR=.*,_LOCALSTATEDIR="%{_localstatedir}",' \
       -e 's,^_DATADIR=.*,_DATADIR="%{_datadir}",' \
       -e 's,^NAME=.*,NAME="%{name}",' \
       -e 's,^PYTHON2_SITELIB=.*,PYTHON2_SITELIB="%{python2_sitelib}",' \
    $RPM_BUILD_ROOT/%{_bindir}/openlmi-pcp-generate \
    $RPM_BUILD_ROOT/%{_sysconfdir}/cron.daily/openlmi-pcp
mkdir -p $RPM_BUILD_ROOT/%{_localstatedir}/lib/%{name}
touch $RPM_BUILD_ROOT/%{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.mof
touch $RPM_BUILD_ROOT/%{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.reg
touch $RPM_BUILD_ROOT/%{_localstatedir}/lib/%{name}/stamp

# documentation
install -m 755 -d $RPM_BUILD_ROOT/%{_docdir}/%{name}
install -m 644 README COPYING $RPM_BUILD_ROOT/%{_docdir}/%{name}
for provider in account fan hardware journald logicalfile power realmd software; do
    install -m 755 -d $RPM_BUILD_ROOT/%{_docdir}/%{name}/${provider}/admin_guide
    cp -pr %{_target_platform}/doc/admin/${provider}/html/* $RPM_BUILD_ROOT/%{_docdir}/%{name}/${provider}/admin_guide
done
install -m 755 -d $RPM_BUILD_ROOT/%{_docdir}/%{name}/service/admin_guide
cp -pr %{_target_platform}/doc/admin/service-dbus/html/* $RPM_BUILD_ROOT/%{_docdir}/%{name}/service/admin_guide

# sphinx theme
install -m 755 -d $RPM_BUILD_ROOT/%{python_sitelib}/sphinx/themes/openlmitheme
cp -pr tools/openlmitheme/* $RPM_BUILD_ROOT/%{python_sitelib}/sphinx/themes/openlmitheme/


%files
%dir %{_docdir}/%{name}
%doc %{_docdir}/%{name}/README
%doc %{_docdir}/%{name}/COPYING
%dir %{_datadir}/%{name}
%dir %{_sysconfdir}/openlmi
%config(noreplace) %{_sysconfdir}/openlmi/openlmi.conf
%{_datadir}/%{name}/05_LMI_Qualifiers.mof
%{_datadir}/%{name}/30_LMI_Jobs.mof
%{_libdir}/libopenlmicommon.so.*
%attr(755, root, root) %{_bindir}/openlmi-mof-register
%ghost %logfile

%files devel
%doc README COPYING
%{_bindir}/openlmi-doc-class2rst
%{_bindir}/openlmi-doc-class2uml
%{_libdir}/libopenlmicommon.so
%{_libdir}/pkgconfig/openlmi.pc
%dir %{_includedir}/openlmi
%{_includedir}/openlmi/openlmi.h
%{_datadir}/cmake/Modules/OpenLMIMacros.cmake
%{_datadir}/cmake/Modules/FindOpenLMI.cmake
%{_datadir}/cmake/Modules/FindCMPI.cmake
%{_datadir}/cmake/Modules/FindKonkretCMPI.cmake
%{_datadir}/cmake/Modules/FindOpenLMIIndManager.cmake

%files -n openlmi-fan
%doc README COPYING
%{_libdir}/cmpi/libcmpiLMI_Fan.so
%{_datadir}/%{name}/60_LMI_Fan.mof
%{_datadir}/%{name}/60_LMI_Fan.reg
%{_datadir}/%{name}/90_LMI_Fan_Profile.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_Fan-cimprovagt

%files -n openlmi-fan-doc
%{_docdir}/%{name}/fan/

%files -n openlmi-powermanagement
%doc README COPYING
%{_libdir}/cmpi/libcmpiLMI_PowerManagement.so
%{_datadir}/%{name}/60_LMI_PowerManagement.mof
%{_datadir}/%{name}/60_LMI_PowerManagement.reg
%{_datadir}/%{name}/90_LMI_PowerManagement_Profile.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_PowerManagement-cimprovagt

%files -n openlmi-powermanagement-doc
%{_docdir}/%{name}/power/

%files -n openlmi-service
%doc README COPYING
%{_libdir}/cmpi/libcmpiLMI_Service.so
%{_datadir}/%{name}/60_LMI_Service.mof
%{_datadir}/%{name}/60_LMI_Service.reg
%{_datadir}/%{name}/90_LMI_Service_Profile.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_Service-cimprovagt

%files -n openlmi-service-doc
%{_docdir}/%{name}/service/

%files -n openlmi-account
%doc README COPYING
%{_libdir}/cmpi/libcmpiLMI_Account.so
%{_datadir}/%{name}/60_LMI_Account.mof
%{_datadir}/%{name}/60_LMI_Account.reg
%{_datadir}/%{name}/90_LMI_Account_Profile.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_Account-cimprovagt

%files -n openlmi-account-doc
%{_docdir}/%{name}/account/

%files -n openlmi-hardware
%doc README COPYING
%{_libdir}/cmpi/libcmpiLMI_Hardware.so
%{_datadir}/%{name}/60_LMI_Hardware.mof
%{_datadir}/%{name}/60_LMI_Hardware.reg
%{_datadir}/%{name}/90_LMI_Hardware_Profile.mof
%{_datadir}/%{name}/90_LMI_Hardware_Profile_DMTF.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_Hardware-cimprovagt

%files -n openlmi-hardware-doc
%{_docdir}/%{name}/hardware/

%files -n openlmi-python-base
%doc README COPYING
%dir %{python2_sitelib}/lmi
%{python2_sitelib}/lmi/__init__.py
%{python2_sitelib}/lmi/__init__.py[co]
%{python2_sitelib}/openlmi-*
%{python2_sitelib}/lmi/base/

%files -n openlmi-python-providers
%doc README COPYING
%dir %{python2_sitelib}/lmi/providers
%{python2_sitelib}/lmi/providers/*.py
%{python2_sitelib}/lmi/providers/*.py[co]

%files -n openlmi-software
%doc README COPYING
%config(noreplace) %{_sysconfdir}/openlmi/software/software.conf
%config(noreplace) %{_sysconfdir}/openlmi/software/yum_worker_logging.conf
%{python2_sitelib}/lmi/software/
%{python2_sitelib}/openlmi_software-*
%{_libexecdir}/pegasus/pycmpiLMI_Software-cimprovagt
%{_datadir}/%{name}/60_LMI_Software.mof
%{_datadir}/%{name}/60_LMI_Software_MethodParameters.mof
%{_datadir}/%{name}/70_LMI_SoftwareIndicationFilters.mof
%{_datadir}/%{name}/90_LMI_Software_Profile.mof
%{_datadir}/%{name}/LMI_Software.reg

%files -n openlmi-software-doc
%{_docdir}/%{name}/software/

%files -n openlmi-pcp
%doc README COPYING
%{_datadir}/%{name}/60_LMI_PCP.mof
%{python2_sitelib}/lmi/pcp/
%{python2_sitelib}/openlmi_pcp-*
%attr(755, root, root) %{_bindir}/openlmi-pcp-generate
%attr(755, root, root) %{_sysconfdir}/cron.daily/openlmi-pcp
%dir %{_localstatedir}/lib/%{name}
%ghost %{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.mof
%ghost %{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.reg
%ghost %{_localstatedir}/lib/%{name}/stamp

%files -n openlmi-logicalfile
%doc README COPYING
%{_libdir}/cmpi/libcmpiLMI_LogicalFile.so
%{_datadir}/%{name}/60_LMI_LogicalFile.mof
%{_datadir}/%{name}/60_LMI_LogicalFile.reg
%{_datadir}/%{name}/90_LMI_LogicalFile_Profile.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_LogicalFile-cimprovagt

%files -n openlmi-logicalfile-doc
%{_docdir}/%{name}/logicalfile/

%files -n openlmi-realmd
%doc README COPYING
%{_libdir}/cmpi/libcmpiLMI_Realmd.so
%{_datadir}/%{name}/60_LMI_Realmd.mof
%{_datadir}/%{name}/60_LMI_Realmd.reg
%{_datadir}/%{name}/90_LMI_Realmd_Profile.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_Realmd-cimprovagt

%files -n openlmi-realmd-doc
%{_docdir}/%{name}/realmd/

%files -n openlmi-indicationmanager-libs
%doc COPYING src/indmanager/README
%{_libdir}/libopenlmiindmanager.so.*

%files -n openlmi-indicationmanager-libs-devel
%doc COPYING src/indmanager/README
%{_libdir}/libopenlmiindmanager.so
%{_libdir}/pkgconfig/openlmiindmanager.pc
%{_includedir}/openlmi/ind_manager.h

%files -n openlmi
%doc COPYING README

%files -n python-sphinx-theme-openlmi
%doc COPYING README
%{python_sitelib}/sphinx/themes/openlmitheme/

%files -n openlmi-journald
%doc README COPYING
%{_libdir}/cmpi/libcmpiLMI_Journald.so
%{_datadir}/%{name}/60_LMI_Journald.mof
%{_datadir}/%{name}/60_LMI_Journald.reg
%{_datadir}/%{name}/90_LMI_Journald_Profile.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_Journald-cimprovagt

%files -n openlmi-journald-doc
%{_docdir}/%{name}/journald/

%pre
# If upgrading, deregister old version
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register --just-mofs unregister \
        %{_datadir}/%{name}/05_LMI_Qualifiers.mof \
        %{_datadir}/%{name}/30_LMI_Jobs.mof || :;
fi >> %logfile 2>&1

%post
/sbin/ldconfig
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register --just-mofs register \
        %{_datadir}/%{name}/05_LMI_Qualifiers.mof \
        %{_datadir}/%{name}/30_LMI_Jobs.mof || :;
fi >> %logfile 2>&1

%preun
# Deregister only if not upgrading
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register --just-mofs unregister \
        %{_datadir}/%{name}/05_LMI_Qualifiers.mof \
        %{_datadir}/%{name}/30_LMI_Jobs.mof || :;
fi >> %logfile 2>&1

%postun -p /sbin/ldconfig

%post -n openlmi-indicationmanager-libs -p /sbin/ldconfig
%postun -n openlmi-indicationmanager-libs -p /sbin/ldconfig

%pre -n openlmi-fan
# If upgrading, deregister old version
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Fan.mof \
        %{_datadir}/%{name}/60_LMI_Fan.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Fan_Profile.mof || :;
fi >> %logfile 2>&1

%pre -n openlmi-powermanagement
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_PowerManagement.mof \
        %{_datadir}/%{name}/60_LMI_PowerManagement.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_PowerManagement_Profile.mof || :;
fi >> %logfile 2>&1

%pre -n openlmi-service
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Service.mof \
        %{_datadir}/%{name}/60_LMI_Service.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Service_Profile.mof || :;
fi >> %logfile 2>&1

%pre -n openlmi-account
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Account.mof \
        %{_datadir}/%{name}/60_LMI_Account.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Account_Profile.mof || :;
fi >> %logfile 2>&1

%pre -n openlmi-software
if [ "$1" -gt 1 ]; then
    # delete indication filters
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop unregister \
        %{_datadir}/%{name}/70_LMI_SoftwareIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Software_Profile.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -c tog-pegasus unregister \
        %{_datadir}/%{name}/60_LMI_Software_MethodParameters.mof || :;
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Software.mof \
        %{_datadir}/%{name}/LMI_Software.reg || :;
fi >> %logfile 2>&1

%pre -n openlmi-logicalfile
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_LogicalFile.mof \
        %{_datadir}/%{name}/60_LMI_LogicalFile.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_LogicalFile_Profile.mof || :;
fi >> %logfile 2>&1

%pre -n openlmi-realmd
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Realmd.mof \
        %{_datadir}/%{name}/60_LMI_Realmd.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Realmd_Profile.mof || :;
fi >> %logfile 2>&1

%pre -n openlmi-hardware
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Hardware.mof \
        %{_datadir}/%{name}/60_LMI_Hardware.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile.mof \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile_DMTF.mof || :;
fi >> %logfile 2>&1

%pre -n openlmi-pcp
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_PCP.mof \
        %{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.mof \
        %{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.reg || :;
fi >> %logfile 2>&1

%pre -n openlmi-journald
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Journald.mof \
        %{_datadir}/%{name}/60_LMI_Journald.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Journald_Profile.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-fan
# Register Schema and Provider
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} register \
        %{_datadir}/%{name}/60_LMI_Fan.mof \
        %{_datadir}/%{name}/60_LMI_Fan.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Fan_Profile.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-powermanagement
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} register \
        %{_datadir}/%{name}/60_LMI_PowerManagement.mof \
        %{_datadir}/%{name}/60_LMI_PowerManagement.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_PowerManagement_Profile.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-service
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} register \
        %{_datadir}/%{name}/60_LMI_Service.mof \
        %{_datadir}/%{name}/60_LMI_Service.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Service_Profile.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-account
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} register \
        %{_datadir}/%{name}/60_LMI_Account.mof \
        %{_datadir}/%{name}/60_LMI_Account.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Account_Profile.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-software
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} register \
        %{_datadir}/%{name}/60_LMI_Software.mof \
        %{_datadir}/%{name}/LMI_Software.reg || :;
    # install indication filters for sfcbd
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop register \
        %{_datadir}/%{name}/70_LMI_SoftwareIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Software_Profile.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -c tog-pegasus register \
        %{_datadir}/%{name}/60_LMI_Software_MethodParameters.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-logicalfile
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} register \
        %{_datadir}/%{name}/60_LMI_LogicalFile.mof \
        %{_datadir}/%{name}/60_LMI_LogicalFile.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_LogicalFile_Profile.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-realmd
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} register \
        %{_datadir}/%{name}/60_LMI_Realmd.mof \
        %{_datadir}/%{name}/60_LMI_Realmd.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Realmd_Profile.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-hardware
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} register \
        %{_datadir}/%{name}/60_LMI_Hardware.mof \
        %{_datadir}/%{name}/60_LMI_Hardware.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile.mof \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile_DMTF.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-pcp
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} register \
        %{_datadir}/%{name}/60_LMI_PCP.mof \
        %{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.mof \
        %{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.reg || :;
fi >> %logfile 2>&1

%post -n openlmi-journald
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} register \
        %{_datadir}/%{name}/60_LMI_Journald.mof \
        %{_datadir}/%{name}/60_LMI_Journald.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Journald_Profile.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-fan
# Deregister only if not upgrading
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Fan.mof \
        %{_datadir}/%{name}/60_LMI_Fan.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Fan_Profile.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-powermanagement
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_PowerManagement.mof \
        %{_datadir}/%{name}/60_LMI_PowerManagement.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_PowerManagement_Profile.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-service
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Service.mof \
        %{_datadir}/%{name}/60_LMI_Service.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Service_Profile.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-account
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Account.mof \
        %{_datadir}/%{name}/60_LMI_Account.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Account_Profile.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-software
if [ "$1" -eq 0 ]; then
    # delete indication filters
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop unregister \
        %{_datadir}/%{name}/70_LMI_SoftwareIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Software_Profile.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -c tog-pegasus unregister \
        %{_datadir}/%{name}/60_LMI_Software_MethodParameters.mof || :;
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Software.mof \
        %{_datadir}/%{name}/LMI_Software.reg || :;
fi >> %logfile 2>&1

%preun -n openlmi-logicalfile
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_LogicalFile.mof \
        %{_datadir}/%{name}/60_LMI_LogicalFile.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_LogicalFile_Profile.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-realmd
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Realmd.mof \
        %{_datadir}/%{name}/60_LMI_Realmd.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Realmd_Profile.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-hardware
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Hardware.mof \
        %{_datadir}/%{name}/60_LMI_Hardware.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile.mof \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile_DMTF.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-pcp
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_PCP.mof \
        %{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.mof \
        %{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.reg || :;
fi >> %logfile 2>&1

%preun -n openlmi-journald
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Journald.mof \
        %{_datadir}/%{name}/60_LMI_Journald.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Journald_Profile.mof || :;
fi >> %logfile 2>&1

%changelog
* Mon Nov 04 2013 Radek Novacek <rnovacek@redhat.com> 0.4.1-3
- Version 0.4.1
- Add powermanagement and hardware providers documentation
- Require cim-schema
- Explicit dependency on systemd-libs removed
- Use python2_sitelib instead of python_sitelib
- Remove dependency on sblim-cmpi-base

* Fri Oct 18 2013 Tomas Bzatek <tbzatek@redhat.com> 0.4.0-3
- Added journald documentation

* Thu Oct 17 2013 Michal Minar <miminar@redhat.com> 0.4.0-2
- Added documentation package for fan provider.

* Tue Oct 15 2013 Tomas Bzatek <tbzatek@redhat.com> 0.4.0-1
- Version 0.4.0
- New journald provider
- Added documentation for service, logicalfile and realmd providers
- Documentation foundation improvements
- logicalfile and software misc. fixes
- Use PG_ComputerSystem by default

* Tue Oct 08 2013 Michal Minar <miminar@redhat.com> 0.3.0-2
- Added documentation for software.

* Mon Sep 30 2013 Roman Rakus <rrakus@redhat.com> - 0.3.0-1
- Version 0.3.0
- Enhancement in account api.
- Fixed logging.
- per provider configuration files as well as global configuration files

* Mon Sep 16 2013 Tomas Smetana <tsmetana@redhat.com> 0.2.0-2
- Add the openlmi metapackage

* Tue Aug 27 2013 Michal Minar <miminar@redhat.com> 0.2.0-1
- Version 0.2.0
- Enhancement in software api.

* Tue Aug 27 2013 Michal Minar <miminar@redhat.com> 0.1.1-3
- Added openlmi-account-doc package with admin guide.
- Fixed installation of python packages.

* Wed Aug 14 2013 Jan Safranek <jsafrane@redhat.com> 0.1.1-2
- Register __MethodParameters classes only in Pegasus

* Wed Aug 07 2013 Radek Novacek <rnovacek@redhat.com> 0.1.1-1
- Version 0.1.1
- Improve scripts logging
- Require dmidecode only on supported archs

* Tue Aug 06 2013 Michal Minar <miminar@redhat.com> 0.1.0-2
- Make lmi namespace directory compatible for user installed python eggs.

* Wed Jul 31 2013 Radek Novacek <rnovacek@redhat.com> 0.1.0-1
- Version 0.1.0

* Tue Jul 30 2013 Michal Minar <miminar@redhat.com> 0.0.25-11
- python subpackage split into python-base and python-providers

* Mon Jul 29 2013 Peter Schiffer <pschiffe@redhat.com> 0.0.25-11
- Added hardware profile registration

* Fri Jul 26 2013 Michal Minar <miminar@redhat.com> 0.0.25-10
- Got rid of root/PG_InterOp namespace
- Added software registration mof

* Thu Jul 25 2013 Jan Synáček <jsynacek@redhat.com> - 0.0.25-9
- Add logicalfile profile registration
- Correctly register account profiles

* Thu Jul 25 2013 Radek Novacek <rnovacek@redhat.com> - 0.0.25-8
- Add version to mof/reg registration

* Tue Jul 23 2013 Michal Minar <miminar@redhat.com> 0.0.25-7
- Added configuration files for software.

* Thu Jul 18 2013 Frank Ch. Eigler <fche@redhat.com> 0.0.25-6
- Added PCP provider in optional openlmi-pcp subrpm.

* Mon Jul 15 2013 Jan Synáček <jsynacek@redhat.com> - 0.0.25-5
- Added libselinux-devel to BuildRequires.

* Thu Jul 04 2013 Michal Minar <miminar@redhat.com> 0.0.25-4
- Added profile registration mof file.

* Wed Jul 03 2013 Michal Minar <miminar@redhat.com> 0.0.25-3
- Renamed openlmi python namespace to lmi.

* Tue Jul 02 2013 Michal Minar <miminar@redhat.com> 0.0.25-2
- Added cimprovagt wrapper for SELinux for software providers.

* Mon Jun 03 2013 Roman Rakus <rrakus@redhat.com> - 0.0.25-1
- Release 0.0.25

* Mon May 27 2013 Roman Rakus <rrakus@redhat.com> - 0.0.24-1
- Added Indication manager

* Wed May 22 2013 Jan Safranek <jsafrane@redhat.com> 0.0.22-2
- Removed openlmi-cimmof tool, added dependency on pywbem instead.

* Fri May 10 2013 Jan Safranek <jsafrane@redhat.com> 0.0.22-1
- Create the spec file.

