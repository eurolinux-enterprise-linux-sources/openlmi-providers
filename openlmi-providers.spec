%global logfile %{_localstatedir}/log/openlmi-install.log
%global required_konkret_ver 0.9.0-2
%global required_libuser_ver 0.60

Name:           openlmi-providers
Version:        0.4.2
Release:        8%{?dist}
Summary:        Set of basic CIM providers

License:        LGPLv2+
URL:            http://fedorahosted.org/openlmi/
Source0:        http://fedorahosted.org/released/openlmi-providers/%{name}-%{version}.tar.gz

# Upstream name has been changed from cura-providers to openlmi-providers
Provides:       cura-providers = %{version}-%{release}
Obsoletes:      cura-providers < 0.0.10-1

# == Provider versions ==

# Don't use %%{version} and %%{release} later on, it will be overwritten by openlmi metapackage
%global providers_version %{version}
%global providers_release %{release}
%global providers_version_release %{version}-%{release}
# Providers built from this package need to be strictly
# matched, so that they are always upgraded together.
%global hw_version %{providers_version_release}
%global sw_version %{providers_version_release}
%global pwmgmt_version %{providers_version_release}
%global acct_version %{providers_version_release}
%global svc_version %{providers_version_release}
%global pcp_version %{providers_version_release}
%global journald_version %{providers_version_release}
%global realmd_version %{providers_version_release}

# Storage and networking providers are built out of tree
# We will require a minimum and maximum version of them
# to ensure that they are tested together.
%global storage_min_version 0.7.1
%global storage_max_version 0.8

%global nw_min_version 0.2.2
%global nw_max_version 0.3

BuildRequires:  cmake
BuildRequires:  konkretcmpi-devel >= %{required_konkret_ver}
BuildRequires:  sblim-cmpi-devel
BuildRequires:  cim-schema
# For openlmi-fan
BuildRequires:  lm_sensors-devel
# For openlmi-account
BuildRequires:  libuser-devel >= %{required_libuser_ver}
BuildRequires:  python2-devel
# for openlmi-*-doc packages
BuildRequires:  konkretcmpi-python >= %{required_konkret_ver}
BuildRequires:  python-sphinx
# For openlmi-hardware
BuildRequires:  pciutils-devel
# For openlmi-logicalfile
BuildRequires:  libudev-devel
BuildRequires:  libselinux-devel
# For openlmi-realmd
BuildRequires:  dbus-devel
# For openlmi-mof-register script
Requires:       python2
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
# for lmi.base.mofparse:
Requires:       openlmi-python-base = %{providers_version_release}

# XXX
# Just because we have wired python's scripts
# Remove in future
BuildRequires:  python-setuptools

# from upstream
# https://bugzilla.redhat.com/show_bug.cgi?id=1050841
Patch0:        openlmi-providers-0.4.2-thread-unsafe.patch
# https://bugzilla.redhat.com/show_bug.cgi?id=1042843
Patch1:        openlmi-providers-0.4.2-dont-autofill-fsname-fsclassname.patch
# https://bugzilla.redhat.com/show_bug.cgi?id=1049819
Patch2:        openlmi-providers-0.4.2-proper-method-result.patch
# https://bugzilla.redhat.com/show_bug.cgi?id=1049816
Patch3:        openlmi-providers-0.4.2-methods-description.patch
# https://bugzilla.redhat.com/show_bug.cgi?id=1031650
Patch4:        openlmi-providers-0.4.2-fan-make-the-sprintf_chip_name-thread-safe.patch
# https://bugzilla.redhat.com/show_bug.cgi?id=1062264
Patch5:        openlmi-providers-0.4.2-logicalfile-socket-getinstance.patch
# https://bugzilla.redhat.com/show_bug.cgi?id=1074419
Patch6:        openlmi-providers-0.4.2-system_name_verification.patch

%description
%{name} is set of (usually) small CMPI providers (agents) for basic
monitoring and management of host system using Common Information
Model (CIM).

%package devel
Summary:        Development files for %{name}
Requires:       %{name}%{?_isa} = %{providers_version_release}
Requires:       konkretcmpi-python >= %{required_konkret_ver}
Provides:       cura-providers-devel = %{providers_version_release}
Obsoletes:      cura-providers-devel < 0.0.10-1

%description devel
%{summary}.

%package -n openlmi-fan
Summary:        CIM provider for controlling fans
Requires:       %{name}%{?_isa} = %{providers_version_release}
Provides:       cura-fan = %{providers_version_release}
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
Requires:       %{name}%{?_isa} = %{providers_version_release}
Provides:       cura-powermanagement = %{providers_version_release}
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
Requires:       %{name}%{?_isa} = %{providers_version_release}
Provides:       cura-service = %{providers_version_release}
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
Requires:       %{name}%{?_isa} = %{providers_version_release}
Requires:       openlmi-indicationmanager-libs%{?_isa} = %{providers_version_release}
Requires:       libuser >= %{required_libuser_ver}
Provides:       cura-account = %{providers_version_release}
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
Requires:       %{name}%{?_isa} = %{providers_version_release}
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
Provides:       openlmi-python = %{providers_version_release}

%description -n openlmi-python-base
The openlmi-python-base package contains python namespace package
for all OpenLMI related projects running on python.

%package -n openlmi-python-providers
Summary:        Python namespace package for pywbem providers
Requires:       %{name} = %{providers_version_release}
Requires:       openlmi-python-base = %{providers_version_release}
BuildArch:      noarch

%description -n openlmi-python-providers
The openlmi-python-providers package contains library with common
code for implementing CIM providers using cmpi-bindings-pywbem.

%package -n openlmi-python-test
Summary:        OpenLMI test utilities
Requires:       %{name} = %{providers_version_release}
Requires:       openlmi-python-base = %{providers_version_release}
Requires:       openlmi-tools >= 0.9
BuildArch:      noarch

%description -n openlmi-python-test
The openlmi-python-test package contains test utilities and base
classes for provider test cases.

%package -n openlmi-software
Summary:        CIM providers for software management
Requires:       %{name} = %{providers_version_release}
Requires:       openlmi-python-providers = %{providers_version_release}
Provides:       cura-software = %{providers_version_release}
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
Requires:       %{name}%{?_isa} = %{providers_version_release}

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
Requires:       %{name}%{?_isa} = %{providers_version_release}
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
Requires:       %{name}%{?_isa} = %{providers_version_release}

%description -n openlmi-indicationmanager-libs
%{summary}.

%package -n openlmi-indicationmanager-libs-devel
Summary:        Development files for openlmi-indicationmanager-libs
Requires:       %{name}%{?_isa} = %{providers_version_release}
Requires:       openlmi-indicationmanager-libs%{_isa} = %{providers_version_release}

%description -n openlmi-indicationmanager-libs-devel
%{summary}.

%package -n openlmi-pcp
Summary:        pywbem providers for accessing PCP metrics
Requires:       %{name} = %{providers_version_release}
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
Version:        1.0.1
Requires:       %{name} = %{providers_version_release}
BuildArch:      noarch
Requires:       tog-pegasus
# List of "safe" providers
Requires:       openlmi-hardware = %{hw_version}
Requires:       openlmi-software = %{sw_version}
Requires:       openlmi-powermanagement = %{pwmgmt_version}
Requires:       openlmi-account = %{acct_version}
Requires:       openlmi-service = %{svc_version}

# Mandatory, out-of-tree providers
Requires:       openlmi-storage >= %{storage_min_version}
Conflicts:      openlmi-storage >= %{storage_max_version}
Requires:       openlmi-networking >= %{nw_min_version}
Conflicts:      openlmi-networking >= %{nw_max_version}

# Optional Providers
# This ensures that only the appropriate version is installed but does
# not install it by default. If these packages are installed, this will
# guarantee that they are updated to the appropriate version on upgrade.
Conflicts:      openlmi-pcp > %{pcp_version}
Conflicts:      openlmi-pcp < %{pcp_version}

Conflicts:      openlmi-journald > %{journald_version}
Conflicts:      openlmi-journald < %{journald_version}

Conflicts:      openlmi-realmd > %{realmd_version}
Conflicts:      openlmi-realmd < %{realmd_version}

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

%prep
%setup -q
%patch0 -p1
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1
%patch5 -p1
%patch6 -p1

%build
mkdir -p %{_target_platform}
pushd %{_target_platform}
%{cmake} .. -DCRYPT_ALGS='"SHA512","SHA256","DES","MD5"' -DWITH-JOURNALD=OFF
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

# The registration database and directories
mkdir -p "$RPM_BUILD_ROOT/%{_sharedstatedir}/openlmi-registration/mof"
mkdir -p "$RPM_BUILD_ROOT/%{_sharedstatedir}/openlmi-registration/reg"
touch "$RPM_BUILD_ROOT/%{_sharedstatedir}/openlmi-registration/regdb.sqlite"

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
cp -p %{_target_platform}/src/pcp/openlmi-pcp-generate $RPM_BUILD_ROOT/%{_bindir}/openlmi-pcp-generate
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
for provider in account fan hardware logicalfile power realmd software; do
    install -m 755 -d $RPM_BUILD_ROOT/%{_docdir}/%{name}/${provider}/admin_guide
    cp -pr %{_target_platform}/doc/admin/${provider}/html/* $RPM_BUILD_ROOT/%{_docdir}/%{name}/${provider}/admin_guide
done
install -m 755 -d $RPM_BUILD_ROOT/%{_docdir}/%{name}/service/admin_guide
cp -pr %{_target_platform}/doc/admin/service-dbus/html/* $RPM_BUILD_ROOT/%{_docdir}/%{name}/service/admin_guide

# sphinx theme
install -m 755 -d $RPM_BUILD_ROOT/%{python_sitelib}/sphinx/themes/openlmitheme
cp -pr tools/openlmitheme/* $RPM_BUILD_ROOT/%{python_sitelib}/sphinx/themes/openlmitheme/

# Remove journald
rm $RPM_BUILD_ROOT/%{_datadir}/%{name}/60_LMI_Journald.mof

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
%dir %{_sharedstatedir}/openlmi-registration
%dir %{_sharedstatedir}/openlmi-registration/mof
%dir %{_sharedstatedir}/openlmi-registration/reg
%ghost %{_sharedstatedir}/openlmi-registration/regdb.sqlite

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

%files -n openlmi-python-test
%doc README COPYING
%dir %{python2_sitelib}/lmi/test
%{python2_sitelib}/lmi/test/*.py
%{python2_sitelib}/lmi/test/*.py[co]

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
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Fan.mof \
        %{_datadir}/%{name}/60_LMI_Fan.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Fan_Profile.mof || :;
fi >> %logfile 2>&1

%pre -n openlmi-powermanagement
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_PowerManagement.mof \
        %{_datadir}/%{name}/60_LMI_PowerManagement.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_PowerManagement_Profile.mof || :;
fi >> %logfile 2>&1

%pre -n openlmi-service
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Service.mof \
        %{_datadir}/%{name}/60_LMI_Service.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Service_Profile.mof || :;
fi >> %logfile 2>&1

%pre -n openlmi-account
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
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
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Software.mof \
        %{_datadir}/%{name}/LMI_Software.reg || :;
fi >> %logfile 2>&1

%pre -n openlmi-logicalfile
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_LogicalFile.mof \
        %{_datadir}/%{name}/60_LMI_LogicalFile.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_LogicalFile_Profile.mof || :;
fi >> %logfile 2>&1

%pre -n openlmi-realmd
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Realmd.mof \
        %{_datadir}/%{name}/60_LMI_Realmd.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Realmd_Profile.mof || :;
fi >> %logfile 2>&1

%pre -n openlmi-hardware
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Hardware.mof \
        %{_datadir}/%{name}/60_LMI_Hardware.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile.mof \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile_DMTF.mof || :;
fi >> %logfile 2>&1

%pre -n openlmi-pcp
if [ "$1" -gt 1 ]; then
    # Only unregister when the provider was already registered
    if [ -e %{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.mof ]; then
        %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
            %{_datadir}/%{name}/60_LMI_PCP.mof \
            %{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.mof \
            %{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.reg || :;
    fi
fi >> %logfile 2>&1

%post -n openlmi-fan
# Register Schema and Provider
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_Fan.mof \
        %{_datadir}/%{name}/60_LMI_Fan.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Fan_Profile.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-powermanagement
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_PowerManagement.mof \
        %{_datadir}/%{name}/60_LMI_PowerManagement.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_PowerManagement_Profile.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-service
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_Service.mof \
        %{_datadir}/%{name}/60_LMI_Service.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Service_Profile.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-account
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_Account.mof \
        %{_datadir}/%{name}/60_LMI_Account.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Account_Profile.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-software
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
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
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_LogicalFile.mof \
        %{_datadir}/%{name}/60_LMI_LogicalFile.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_LogicalFile_Profile.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-realmd
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_Realmd.mof \
        %{_datadir}/%{name}/60_LMI_Realmd.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Realmd_Profile.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-hardware
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_Hardware.mof \
        %{_datadir}/%{name}/60_LMI_Hardware.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile.mof \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile_DMTF.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-fan
# Deregister only if not upgrading
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Fan.mof \
        %{_datadir}/%{name}/60_LMI_Fan.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Fan_Profile.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-powermanagement
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_PowerManagement.mof \
        %{_datadir}/%{name}/60_LMI_PowerManagement.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_PowerManagement_Profile.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-service
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Service.mof \
        %{_datadir}/%{name}/60_LMI_Service.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Service_Profile.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-account
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
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
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Software.mof \
        %{_datadir}/%{name}/LMI_Software.reg || :;
fi >> %logfile 2>&1

%preun -n openlmi-logicalfile
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_LogicalFile.mof \
        %{_datadir}/%{name}/60_LMI_LogicalFile.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_LogicalFile_Profile.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-realmd
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Realmd.mof \
        %{_datadir}/%{name}/60_LMI_Realmd.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Realmd_Profile.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-hardware
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Hardware.mof \
        %{_datadir}/%{name}/60_LMI_Hardware.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile.mof \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile_DMTF.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-pcp
if [ "$1" -eq 0 ]; then
    # Only unregister when the provider was already registered
    if [ -e %{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.mof ]; then
        %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
            %{_datadir}/%{name}/60_LMI_PCP.mof \
            %{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.mof \
            %{_localstatedir}/lib/%{name}/60_LMI_PCP_PMNS.reg || :;
    fi
fi >> %logfile 2>&1

%changelog
* Mon Mar 10 2014 Michal Minar <miminar@redhat.com> 0.4.2-8
- Fixed SysteName verification.
- Resolves: rhbz#1074419

* Tue Feb 11 2014 Jan Synáček <jsynacek@redhat.com> - 0.4.2-7
- Fix GetInstance() on a socket file
- Resolves: rhbz#1062264

* Fri Jan 24 2014 Daniel Mach <dmach@redhat.com> - 0.4.2-6
- Mass rebuild 2014-01-24

* Wed Jan 22 2014 Radek Novacek <rnovacek@redhat.com> 0.4.2-5
- Don't unregister pcp provider if not registered
- Resolves: rhbz#1052144

* Tue Jan 21 2014 Vitezslav Crhonek <vcrhonek@redhat.com> - 0.4.2-4
- Fix service methods report success before the action is actually finished
- Add description to service methods
- Fan provider made thread-safe
- Resolves: rhbz#1049819 rhbz#1049816 rhbz#1031650

* Fri Jan 10 2014 Jan Synáček <jsynacek@redhat.com> - 0.4.2-3
- Correctly fill FSCreationClassName and FSName
- Replace thread-unsafe functions
- Resolves: rhbz#1042843 rhbz#1050841

* Fri Jan 10 2014 Radek Novacek <rnovacek@redhat.com> 0.4.2-2
- Fix version mismatch caused by defining version of openlmi metapackage
- Resolves: rhbz#1051008

* Thu Jan  9 2014 Jan Safranek <jsafrane@redhat.com> - 0.4.2-1
- New upstream release.
- Removing lot of patches.

* Wed Jan 08 2014 Michal Minar <miminar@redhat.com> 0.4.1-17
- Applied one omitted patch.
- Related: rhbz#1032590

* Tue Jan 07 2014 Jan Safranek <jsafrane@redhat.com> - 0.4.1-16
- Fixed openlmi-mof-register not to crash when CIMOM is down
- Added another bunch of software bugfixes.
- Resolves: rhbz#1039018 rhbz#1032590 rhbz#1043243
- Resolves: rhbz#1045058 rhbz#1043161 rhbz#1049389

* Fri Dec 27 2013 Daniel Mach <dmach@redhat.com> - 0.4.1-15
- Mass rebuild 2013-12-27

* Tue Dec 17 2013 Jan Synáček <jsynacek@redhat.com> - 0.4.1-14
- Correctly fill FSCreationClassName and FSName
- Rewrite description of PowerStatesSupported property (power provider)
- Always init provider when starting (power provider)
- Resolves: rhbz#1042843, rhbz#1042718, rhbz#1041310

* Wed Dec 11 2013 Tomas Smetana <tsmetana@redhat.com> 0.4.1-13
- Add "infinite" timeout to Realmd D-Bus calls
- Resolves: rhbz#1013624

* Mon Dec 02 2013 Michal Minar <miminar@redhat.com> 0.4.1-12
- Added Software fixes.
- Resolves: rhbz#1005803 rhbz#1028519 rhbz#1027681 rhbz#1030999 rhbz#1028535
- Resolves: rhbz#1031132 rhbz#1032502 rhbz#1032480 rhbz#1030831 rhbz#1032590
- Resolves: rhbz#1034615 rhbz#1034698 rhbz#1035328 rhbz#1036291

* Thu Nov 28 2013 Jan Safranek <jsafrane@redhat.com> 0.4.1-11
- Fixed openlmi-mof-register to parse "#pragma include" in mof files.
- Resolves: rhbz#1030966

* Thu Nov 21 2013 Radek Novacek <rnovacek@redhat.com> 0.4.1-10
- Fix missing format strings
- Add format string hardening
- realmd: remove potentially unsecure octetstring_parse function
- Resolves: rbhz#1031644, rhbz#1031655, rhbz#1031658

* Thu Nov 21 2013 Tomas Smetana <tsmetana@redhat.com> 0.4.1-9
- Fix the reregister feature of the registration script
- Make openlmi-providers own the registration database and directories
- Resolves: rhbz#1030972, rhbz#1029008

* Mon Nov 18 2013 Jan Safranek <jsafrane@redhat.com> 0.4.1-8
- Another fixes in association checking in LogicalFile provider
- Resolves: #1029447

* Fri Nov 15 2013 Tomas Bzatek <tbzatek@redhat.com> 0.4.1-7
- Fixes account non-existent homedir deletion (#1027338)

* Thu Nov 14 2013 Jan Safranek <jsafrane@redhat.com> 0.4.1-6
- Additional fixes in association checking in LogicalFile provider
- Resolves: #1029447

* Wed Nov 13 2013 Jan Safranek <jsafrane@redhat.com> 0.4.1-5
- Fix association checking in LogicalFile provider
- Resolves: #1029447

* Tue Nov 12 2013 Tomas Smetana <tsmetana@redhat.com> 0.4.1-4
- Fix the PCP provider registration
- Resolves: rhbz#1026515

* Tue Nov 12 2013 Vitezslav Crhonek <vcrhonek@redhat.com> - 0.4.1-3
- Remove maximum boundary for amount of services
  Resolves: #999338

* Wed Nov  6 2013 Tomas Bzatek <tbzatek@redhat.com> 0.4.1-2
- Added explicit dependency on new libuser release (#1027321)

* Mon Nov  4 2013 Jan Safranek <jsafrane@redhat.com>> 0.4.1-1
- New upstream release
- Remove %{isa} from Obsoletes
- Disable journald

* Mon Oct 21 2013 Radek Novacek <rnovacek@redhat.com> 0.3.0-3
- Fix wrong Require in the metapackage (power -> powermanagement)
- Resolves: rhbz#1019664

* Tue Oct 15 2013 Radek Novacek <rnovacek@redhat.com> 0.3.0-2
- Add openlmi metapackage
- Added python-sphinx-theme-openlmi subpackage
- Resolves: rhbz#1019106

* Tue Oct 01 2013 Roman Rakus <rrakus@redhat.com> - 0.3.0-1
- Version 0.3.0
- Enhancement in account api.
- per provider configuration files as well as global configuration files
- Define crypto algorithms for passwords in Account providers
  Resolves: #1004296, #1007251, #999410, #999487, #1001636, #1002531, #1004221,
  #1006339, #1010301, #1012396, #996859, #999299

* Tue Aug 27 2013 Michal Minar <miminar@redhat.com> 0.2.0-1
- Version 0.2.0
- Enhancement in software api.
- Added openlmi-account-doc package with admin guide.
- Fixed installation of python packages.

* Thu Aug 15 2013 Jan Safranek <jsafrane@redhat.com> 0.1.1-2
- Fixed registration into SFCB (#995561)

* Wed Aug 07 2013 Radek Novacek <rnovacek@redhat.com> 0.1.1-1
- Version 0.1.1
- Improve scripts logging
- Require dmidecode only on supported archs

* Wed Jul 31 2013  <jsafrane@redhat.com>  0.1.0-2
- Correctly obsolete openlmi-python package
- Remove dependency between openlmi-python-base and openlmi-providers

* Wed Jul 31 2013 Radek Novacek <rnovacek@redhat.com> 0.1.0-1
- Version 0.1.0
- Add profile registration
- New provider: openlmi-pcp
- Split openlmi-python to openlmi-python-base and openlmi-python-providers

* Thu Jul 18 2013 Radek Novacek <rnovacek@redhat.com> 0.0.25-3
- Rebuild against new konkretcmpi
- Really fix the compilation against new konkretcmpi

* Thu Jul 11 2013 Roman Rakus <rrakus@redhat.com> - 0.0.25-2
- Again add registration of 05_LMI_Qualifiers.mof

* Mon Jun 03 2013 Roman Rakus <rrakus@redhat.com> - 0.0.25-1
- Version 0.0.25

* Tue May 21 2013 Radek Novacek <rnovacek@redhat.com> 0.0.23-1
- Version 0.0.23
- Remove unused libexec files from services provider

* Fri May 17 2013 Jan Safranek <jsafrane@redhat.com> 0.0.22-3
- Remove duplicate definition of LMI_ConcreteJob

* Fri May 17 2013 Radek Novacek <rnovacek@redhat.com> 0.0.22-2
- Add registration of 05_LMI_Qualifiers.mof

* Fri May 10 2013 Jan Safranek <jsafrane@redhat.com> 0.0.22-1
- Version 0.0.22

* Thu May 09 2013 Radek Novacek <rnovacek@redhat.com> 0.0.21-3
- Fix wrong condition when registering provider

* Tue May 07 2013 Michal Minar <miminar@redhat.com> 0.0.21-2
- Software indication filters get (un)installed in pre/post scripts.

* Fri May 03 2013 Radek Novacek <rnovacek@redhat.com> 0.0.21-1
- Version 0.0.21
- Added openlmi-realmd subpackage
- Add numeral prefix for the mof files
- Add FindOpenLMI.cmake and openlmi.pc to the -devel subpackage

* Fri Mar 22 2013 Michal Minar <miminar@redhat.com> 0.0.20-1
- Version 0.0.20
- Added shared LMI_Jobs.mof file.
- This file is registered upon installation of openlmi-providers.

* Mon Mar 11 2013 Radek Novacek <rnovacek@redhat.com> 0.0.19-1
- Version 0.0.19
- Add LogicalFile provider
- Require konkretcmpi >= 0.9.0-2
- Install openlmi-doc-class2* scripts

* Thu Feb 14 2013 Radek Novacek <rnovacek@redhat.com> 0.0.18-2
- Add upstream patch with improvements of OpenLMIMacros.cmake
- Add requirement for cim-server also to pre, preun and post

* Mon Feb 04 2013 Michal Minar <miminar@redhat.com> 0.0.18-1
- Version 0.0.18

* Tue Jan 22 2013 Roman Rakus <rrakus@redhat.com> - 0.0.17-1
- Version 0.0.17

* Wed Jan 16 2013 Roman Rakus <rrakus@redhat.com> - 0.0.16-1
- Version 0.0.16

* Fri Dec 14 2012 Radek Novacek <rnovacek@redhat.com> 0.0.15-4
- Allow number in class name in OpenLMIMacros.cmake

* Fri Dec 07 2012 Radek Novacek <rnovacek@redhat.com> 0.0.15-3
- Add mof file with definition of 'Implemented' qualifier

* Fri Nov 23 2012 Radek Novacek <rnovacek@redhat.com> 0.0.15-2
- Add missing dependency on sblim-cmpi-base for powermanagement and account

* Tue Nov 13 2012 Radek Novacek <rnovacek@redhat.com> 0.0.15-1
- Version 0.0.15
- Upstream changed license from GPLv2+ to LGPLv2+

* Thu Nov 08 2012 Michal Minar <miminar@redhat.com> 0.0.14-1
- Version 0.0.14
- Added python subpackage for python providers

* Wed Oct 31 2012 Roman Rakus <rrakus@redhat.com> - 0.0.12-2
- Requires fixed konkretcmpi >= 0.8.7-7

* Thu Oct 25 2012 Roman Rakus <rrakus@redhat.com> - 0.0.12-1
- Version 0.0.12

* Thu Oct 25 2012 Radek Novacek <rnovacek@redhat.com> 0.0.10-2
- Remove %isa from obsoletes

* Mon Oct 22 2012 Radek Novacek <rnovacek@redhat.com> - 0.0.10-1
- Package rename to openlmi-providers (instead of cura-providers)
- helper scripts moved from usr/share to proper location
- Version 0.0.10

* Sat Oct 13 2012 Michal Minar <miminar@redhat.com> - 0.0.9-1
- Version 0.0.9
- Renamed cura-yum subpackage to cura-software

* Sun Oct 07 2012 Michal Minar <miminar@redhat.com> - 0.0.8-1
- Version 0.0.8

* Wed Oct 03 2012 Michal Minar <miminar@redhat.com> - 0.0.7-1
- Version 0.0.7

* Thu Sep 27 2012 Michal Minar <miminar@redhat.com> - 0.0.6-1
- Added yum software management providers.

* Thu Sep 27 2012 Roman Rakus <rrakus@redhat.com> - 0.0.5-1
- Version 0.0.5

* Tue Sep 18 2012 Roman Rakus <rrakus@redhat.com> - 0.0.4-1
- Version 0.0.4
- Remove python parts. There are not intended to be present here.

* Mon Aug 27 2012 Radek Novacek <rnovacek@redhat.com> 0.0.3-1
- Version 0.0.3
- Rename prefix from Cura_ to LMI_
- Add more development files

* Tue Aug 14 2012 Roman Rakus <rrakus@redhat.com> - 0.0.2-1
- Version 0.0.2 which includes account manager

* Fri Aug 03 2012 Radek Novacek <rnovacek@redhat.com> 0.0.1-2
- BR: cim-schema
- Don't clean buildroot in install
- Fix typo

* Tue Jul 31 2012 Radek Novacek <rnovacek@redhat.com> 0.0.1-1
- Initial package

