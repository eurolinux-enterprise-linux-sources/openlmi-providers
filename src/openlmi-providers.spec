%if 0%{?rhel} && 0%{?rhel} <= 6
%{!?__python2: %global __python2 /usr/bin/python2}
%{!?python2_sitelib: %global python2_sitelib %(%{__python2} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())")}
%{!?python2_sitearch: %global python2_sitearch %(%{__python2} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(1))")}
%endif

%global logfile %{_localstatedir}/log/openlmi-install.log
%global required_konkret_ver 0.9.0-2
%global required_libuser_ver 0.60

# interop namepsace for tog-pegasus, sfcbd always uses root/interop
%global interop root/interop

%global with_devassistant 1
%global with_journald 1
%global with_service 1
%global with_service_legacy 0
%global with_account 1
%global with_pcp 1
%global with_realmd 1
%global with_fan 1
%global with_locale 1
%global with_indsender 1
%global with_jobmanager 1
%global with_sssd 1

%if 0%{?rhel} == 6
%global with_journald 0
%global with_service 0
%global with_service_legacy 1
%global with_pcp 0
%global with_realmd 0
%global with_fan 0
%global with_locale 0
%global with_indsender 0
%global with_jobmanager 0
%global with_sssd 0
%global interop root/PG_InterOp
%endif

%if 0%{?rhel}
%global with_devassistant 0
%endif

Name:           openlmi-providers
Version:        0.5.0
Release:        1%{?dist}
Summary:        Set of basic CIM providers

%if 0%{?suse_version}
License:        LGPL-2.0+
Group:          System/Management
%else
License:        LGPLv2+
%endif
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
%global sssd_version %{providers_version_release}

# Storage and networking providers are built out of tree
# We will require a minimum and maximum version of them
# to ensure that they are tested together.
%global storage_min_version 0.8
%global storage_max_version 0.9

%global nw_min_version 0.3
%global nw_max_version 0.4

BuildRequires:  cmake
BuildRequires:  konkretcmpi-devel >= %{required_konkret_ver}
BuildRequires:  sblim-cmpi-devel
BuildRequires:  cim-schema
BuildRequires:  glib2-devel
%if 0%{?suse_version}
BuildRequires:  gcc-c++
BuildRequires:  libselinux-devel
BuildRequires:  libudev-devel
BuildRequires:  pkg-config
%endif

%if 0%{?with_fan}
%if 0%{?suse_version}
BuildRequires:  libsensors4-devel
%else
BuildRequires:  lm_sensors-devel
%endif
%endif

%if 0%{?with_account}
BuildRequires:  libuser-devel >= %{required_libuser_ver}
%endif

%if 0%{?suse_version}
BuildRequires:  python
%else
BuildRequires:  python2-devel
%endif
# for openlmi-*-doc packages
BuildRequires:  konkretcmpi-python >= %{required_konkret_ver}
%if 0%{?suse_version}
BuildRequires:  python-Sphinx
%else
BuildRequires:  python-sphinx
%endif

# For openlmi-hardware
BuildRequires:  pciutils-devel
# For openlmi-logicalfile
BuildRequires:  libudev-devel
BuildRequires:  libselinux-devel

# For openlmi-mof-register script
%if 0%{?suse_version}
BuildRequires:  python
%else
Requires:       python2
%endif
# for openlmi-journald
%if 0%{?with_journald}
BuildRequires:  systemd-devel
%endif
# for openlmi-realmd:
%if 0%{?suse_version} >= 1110
BuildRequires:  dbus-1-devel
%else
BuildRequires:  dbus-devel
%endif
%if 0%{?with_sssd}
BuildRequires:  libsss_simpleifp-devel
%endif
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
Requires:       openlmi-providers-libs%{?_isa} = %{providers_version_release}


# XXX
# Just because we have wired python's scripts
# Remove in future
%if 0%{?suse_version} == 0 || 0%{?suse_version} > 1110
# SLE_11_SP3: unresolvable: conflict for provider of python-distribute needed by python-Pygments, (provider python-distribute obsoletes installed python-setuptools)
BuildRequires:  python-setuptools
%endif

%if 0%{?with_jobmanager}
BuildRequires:  json-glib-devel
%endif

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
Provides:       openlmi-indicationmanager-libs-devel = %{providers_version_release}

%description devel
%{summary}.

%package libs
Summary:        Libraries for %{name}
Provides:       openlmi-indicationmanager-libs = %{providers_version_release}

%description libs
%{summary}.

%if 0%{?with_fan}
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
%endif

%package -n openlmi-powermanagement
Summary:        Power management CIM provider
Requires:       %{name}%{?_isa} = %{providers_version_release}
Provides:       cura-powermanagement = %{providers_version_release}
%if 0%{?suse_version}
Requires:       upower
# For Linux_ComputerSystem
Requires:       sblim-cmpi-base
%endif
Obsoletes:      cura-powermanagement < 0.0.10-1

%description -n openlmi-powermanagement
%{summary}.

%package -n openlmi-powermanagement-doc
Summary:        Power management CIM provider documentation
Group:          Documentation
BuildArch:      noarch

%description -n openlmi-powermanagement-doc
This package contains the documents for OpenLMI power management provider.

%if 0%{?with_service} || 0%{?with_service_legacy}
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
%endif

%if 0%{?with_account}
%package -n openlmi-account
Summary:        CIM provider for managing accounts on system
Requires:       %{name}%{?_isa} = %{providers_version_release}
Requires:       libuser >= %{required_libuser_ver}
%if 0%{?suse_version}
# For Linux_ComputerSystem
Requires:       sblim-cmpi-base
%endif
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
%endif

%package -n openlmi-hardware
Summary:        CIM provider for hardware on system
Requires:       %{name}%{?_isa} = %{providers_version_release}
# For Hardware information
%ifarch %{ix86} x86_64 ia64
Requires:       dmidecode
%endif
%if 0%{?suse_version}
# For Linux_ComputerSystem
Requires:       sblim-cmpi-base
%endif
Requires:       util-linux
Requires:       smartmontools
Requires:       virt-what

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
%if 0%{?suse_version}
%{!?python_sitelib: %global python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}
%{!?python2_sitelib: %global python2_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}
%{!?python_sitearch: %global python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}
%{!?py_requires: %define py_requires Requires: python}
%{py_requires}
%endif

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
%if 0%{?suse_version}
# For Linux_ComputerSystem
Requires:       sblim-cmpi-base
%endif
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
%if 0%{?suse_version}
# For Linux_ComputerSystem
Requires:       sblim-cmpi-base
%endif

%description -n openlmi-logicalfile
%{summary}.

%package -n openlmi-logicalfile-doc
Summary:        CIM logicalfile provider documentation
Group:          Documentation
BuildArch:      noarch

%description -n openlmi-logicalfile-doc
This package contains the documents for OpenLMI logicalfile provider.

%if 0%{?with_realmd}
%package -n openlmi-realmd
Summary:        CIM provider for Realmd
Requires:       %{name}%{?_isa} = %{providers_version_release}
Requires:       realmd
%if 0%{?suse_version}
# For Linux_ComputerSystem
Requires:       sblim-cmpi-base
%endif

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
%endif

%if 0%{?with_pcp}
%package -n openlmi-pcp
Summary:        pywbem providers for accessing PCP metrics
Requires:       %{name} = %{providers_version_release}
BuildArch:      noarch
Requires:       python-setuptools
Requires:       cmpi-bindings-pywbem
Requires:       python-pcp
%if 0%{?suse_version}
Requires:       cron
%endif

%description -n openlmi-pcp
openlmi-pcp exposes metrics from a local PMCD (Performance Co-Pilot server)
to the CIMOM.  They appear as potentially hundreds of MOF classes, e.g.
class "PCP_Metric_kernel__pernode__cpu__use", with instances for each PCP
metric instance, e.g. "node0".  PCP metric values and metadata are transcribed
into strings on demand.
%endif

%package -n openlmi
Summary:        OpenLMI managed system software components
Version:        1.0.1
Requires:       %{name} = %{providers_version}
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

%if 0%{?with_journald}
%package -n openlmi-journald
Summary:        CIM provider for Journald
Requires:       %{name}%{?_isa} = %{providers_version_release}

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
%endif

%if 0%{?with_sssd}
%package -n openlmi-sssd
Summary:        CIM provider for SSSD
Requires:       %{name}%{?_isa} = %{providers_version_release}

%description -n openlmi-sssd
The openlmi-sssd package contains CMPI providers for SSSD service.

%package -n openlmi-sssd-doc
Summary:        CIM SSSD provider documentation
Group:          Documentation
BuildArch:      noarch

%description -n openlmi-sssd-doc
This package contains the documents for OpenLMI SSSD provider.
%endif

%if 0%{?with_devassistant}
%package -n openlmi-devassistant
Summary:        OpenLMI provider templates for Developer Assistant
BuildArch:      noarch
Requires:       devassistant >= 0.9.0
Requires:       %{name}-devel%{?_isa} = %{providers_version_release}

%description -n openlmi-devassistant
This package contains template files for Developer Assistant.
%endif

%if 0%{?with_locale}
%package -n openlmi-locale
Summary:        CIM provider for controlling system locale and keyboard mapping
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n openlmi-locale
%{summary}.

%package -n openlmi-locale-doc
Summary:        CIM Locale provider documentation
Group:          Documentation
BuildArch:      noarch

%description -n openlmi-locale-doc
This package contains the documents for OpenLMI Locale provider.
%endif

%prep
%setup -q

%build
%if 0%{?suse_version}
# SUSE %%cmake creates build/ subdir
%global target_builddir %{_target_platform}/build
%global source_dir ../..
%else
%global target_builddir %{_target_platform}
%global source_dir ..
%endif

mkdir -p %{_target_platform}
pushd %{_target_platform}

%{cmake} \
%if ! 0%{with_devassistant}
    -DWITH-DEVASSISTANT=OFF \
%else
    -DWITH-DEVASSISTANT=ON \
%endif
%if ! 0%{with_journald}
    -DWITH-JOURNALD=OFF \
%endif
%if ! 0%{with_service}
    -DWITH-SERVICE=OFF \
%endif
%if 0%{with_service_legacy}
    -DWITH-SERVICE-LEGACY=ON \
%endif
%if ! 0%{with_account}
    -DWITH-ACCOUNT=OFF \
%endif
%if ! 0%{with_pcp}
    -DWITH-PCP=OFF \
%endif
%if ! 0%{with_realmd}
    -DWITH-REALMD=OFF \
%endif
%if ! 0%{with_fan}
    -DWITH-FAN=OFF \
%endif
%if ! 0%{with_locale}
    -DWITH-LOCALE=OFF \
%endif
%if ! 0%{with_indsender}
    -DWITH-INDSENDER=OFF \
%endif
%if ! 0%{with_jobmanager}
    -DWITH-JOBMANAGER=OFF \
%endif
%if ! 0%{with_sssd}
    -DWITH-SSSD=OFF \
%endif
    %{source_dir}

popd

make -k %{?_smp_mflags} -C %{target_builddir} all doc

pushd src/python
%{__python} setup.py build
popd # src/python
# for software providers
pushd src/software
%{__python} setup.py build
popd # src/software

%if 0%{with_pcp}
pushd src/pcp
%{__python} setup.py build
popd
%endif

%install
make install/fast DESTDIR=$RPM_BUILD_ROOT -C %{target_builddir}

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
sed -i 's#/usr/lib/python2.7/site-packages#%{python2_sitelib}#g' mof/LMI_Software.reg
cp mof/LMI_Software.reg $RPM_BUILD_ROOT/%{_datadir}/%{name}/

# pcp
%if 0%{with_pcp}
pushd src/pcp
%{__python} setup.py install -O1 --skip-build --root $RPM_BUILD_ROOT
popd
cp -p %{target_builddir}/src/pcp/openlmi-pcp-generate $RPM_BUILD_ROOT/%{_bindir}/openlmi-pcp-generate
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
%endif

# documentation
install -m 755 -d $RPM_BUILD_ROOT/%{_docdir}/%{name}
install -m 644 README COPYING $RPM_BUILD_ROOT/%{_docdir}/%{name}
for provider in \
%if 0%{?with_account}
    account \
%endif
%if 0%{?with_fan}
    fan \
%endif
    hardware \
%if 0%{?with_journald}
    journald \
%endif
    logicalfile power \
%if 0%{?with_realmd}
    realmd \
%endif
%if 0%{?with_locale}
    locale \
%endif
%if 0%{?with_sssd}
    sssd \
%endif
    software; do

    install -m 755 -d $RPM_BUILD_ROOT/%{_docdir}/%{name}/${provider}/admin_guide
    cp -pr %{target_builddir}/doc/admin/${provider}/html/* $RPM_BUILD_ROOT/%{_docdir}/%{name}/${provider}/admin_guide
done

%if 0%{?with_service}
install -m 755 -d $RPM_BUILD_ROOT/%{_docdir}/%{name}/service/admin_guide
cp -pr %{target_builddir}/doc/admin/service-dbus/html/* $RPM_BUILD_ROOT/%{_docdir}/%{name}/service/admin_guide
%endif
# TODO: service_legacy

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
%attr(755, root, root) %{_bindir}/openlmi-mof-register
%ghost %logfile
%dir %{_sharedstatedir}/openlmi-registration
%dir %{_sharedstatedir}/openlmi-registration/mof
%dir %{_sharedstatedir}/openlmi-registration/reg
%ghost %{_sharedstatedir}/openlmi-registration/regdb.sqlite

%files libs
%doc README COPYING src/libs/indmanager/README.indmanager
%{_libdir}/libopenlmicommon.so.*
%{_libdir}/libopenlmiindmanager.so.*
%if 0%{?with_indsender}
%{_libdir}/libopenlmiindsender.so.*
%endif
%if 0%{?with_jobmanager}
%doc src/libs/jobmanager/README.jobmanager
%{_libdir}/libopenlmijobmanager.so.*
%endif


%files devel
%doc README COPYING src/libs/indmanager/README.indmanager src/libs/jobmanager/README.jobmanager
%{_bindir}/openlmi-doc-class2rst
%{_bindir}/openlmi-doc-class2uml
%{_libdir}/libopenlmicommon.so
%{_libdir}/pkgconfig/openlmi.pc
%dir %{_includedir}/openlmi
%{_includedir}/openlmi/openlmi.h
%{_includedir}/openlmi/openlmi-utils.h
%{_datadir}/cmake/Modules/OpenLMIMacros.cmake
%{_datadir}/cmake/Modules/FindOpenLMI.cmake
%{_datadir}/cmake/Modules/FindCMPI.cmake
%{_datadir}/cmake/Modules/FindKonkretCMPI.cmake
%{_datadir}/cmake/Modules/FindOpenLMIIndManager.cmake
%{_libdir}/libopenlmiindmanager.so
%{_libdir}/pkgconfig/openlmiindmanager.pc
%{_includedir}/openlmi/ind_manager.h
%if 0%{?with_indsender}
%{_libdir}/libopenlmiindsender.so
%{_libdir}/pkgconfig/openlmiindsender.pc
%{_includedir}/openlmi/ind_sender.h
%endif
%if 0%{?with_jobmanager}
%{_libdir}/libopenlmijobmanager.so
%{_libdir}/pkgconfig/openlmijobmanager.pc
%{_includedir}/openlmi/job_manager.h
%{_includedir}/openlmi/lmi_job.h
%endif

%if 0%{with_fan}
%files -n openlmi-fan
%doc README COPYING
%{_libdir}/cmpi/libcmpiLMI_Fan.so
%{_datadir}/%{name}/60_LMI_Fan.mof
%{_datadir}/%{name}/60_LMI_Fan.reg
%{_datadir}/%{name}/90_LMI_Fan_Profile.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_Fan-cimprovagt

%files -n openlmi-fan-doc
%{_docdir}/%{name}/fan/
%endif

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
%if 0%{?with_service_legacy}
%{_libexecdir}/servicedisc.sh
%{_libexecdir}/serviceutil.sh
%endif
%{_datadir}/%{name}/90_LMI_Service_Profile.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_Service-cimprovagt

%files -n openlmi-service-doc
%if 0%{?with_service}
%{_docdir}/%{name}/service/
%endif

%if 0%{with_account}
%files -n openlmi-account
%doc README COPYING
%{_libdir}/cmpi/libcmpiLMI_Account.so
%{_datadir}/%{name}/60_LMI_Account.mof
%{_datadir}/%{name}/60_LMI_Account.reg
%{_datadir}/%{name}/90_LMI_Account_Profile.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_Account-cimprovagt

%files -n openlmi-account-doc
%{_docdir}/%{name}/account/
%endif

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

%if 0%{with_pcp}
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
%endif

%files -n openlmi-logicalfile
%doc README COPYING
%config(noreplace) %{_sysconfdir}/openlmi/logicalfile/logicalfile.conf
%{_libdir}/cmpi/libcmpiLMI_LogicalFile.so
%{_datadir}/%{name}/50_LMI_LogicalFile.mof
%{_datadir}/%{name}/50_LMI_LogicalFile.reg
%{_datadir}/%{name}/90_LMI_LogicalFile_Profile.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_LogicalFile-cimprovagt

%files -n openlmi-logicalfile-doc
%{_docdir}/%{name}/logicalfile/

%if 0%{with_realmd}
%files -n openlmi-realmd
%doc README COPYING
%{_libdir}/cmpi/libcmpiLMI_Realmd.so
%{_datadir}/%{name}/60_LMI_Realmd.mof
%{_datadir}/%{name}/60_LMI_Realmd.reg
%{_datadir}/%{name}/90_LMI_Realmd_Profile.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_Realmd-cimprovagt

%files -n openlmi-realmd-doc
%{_docdir}/%{name}/realmd/
%endif

%files -n openlmi
%doc COPYING README

%files -n python-sphinx-theme-openlmi
%doc COPYING README
%{python_sitelib}/sphinx/themes/openlmitheme/

%if 0%{with_journald}
%files -n openlmi-journald
%doc README COPYING
%{_libdir}/cmpi/libcmpiLMI_Journald.so
%{_datadir}/%{name}/60_LMI_Journald.mof
%{_datadir}/%{name}/60_LMI_Journald.reg
%{_datadir}/%{name}/70_LMI_JournaldIndicationFilters.mof
%{_datadir}/%{name}/90_LMI_Journald_Profile.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_Journald-cimprovagt

%files -n openlmi-journald-doc
%{_docdir}/%{name}/journald/
%endif

%if 0%{with_sssd}
%files -n openlmi-sssd
%doc README COPYING
%{_libdir}/cmpi/libcmpiLMI_SSSD.so
%{_datadir}/%{name}/60_LMI_SSSD.mof
%{_datadir}/%{name}/60_LMI_SSSD.reg
%{_datadir}/%{name}/90_LMI_SSSD_Profile.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_SSSD-cimprovagt

%files -n openlmi-sssd-doc
%{_docdir}/%{name}/sssd/
%endif

%if 0%{with_devassistant}
%files -n openlmi-devassistant
%dir %{_datadir}/devassistant/files/crt/python/openlmi/
%dir %{_datadir}/devassistant/files/crt/c/openlmi/
%{_datadir}/devassistant/assistants/
%{_datadir}/devassistant/files/crt/python/openlmi/*
%{_datadir}/devassistant/files/crt/c/openlmi/*
%endif

%if 0%{with_locale}
%files -n openlmi-locale
%doc README COPYING
%{_libdir}/cmpi/libcmpiLMI_Locale.so
%{_datadir}/%{name}/60_LMI_Locale.mof
%{_datadir}/%{name}/60_LMI_Locale.reg
%{_datadir}/%{name}/90_LMI_Locale_Profile.mof
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_Locale-cimprovagt

%files -n openlmi-locale-doc
%{_docdir}/%{name}/locale/
%endif

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

%post libs -p /sbin/ldconfig
%postun libs -p /sbin/ldconfig

%if 0%{with_fan}
%pre -n openlmi-fan
# If upgrading, deregister old version
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Fan.mof \
        %{_datadir}/%{name}/60_LMI_Fan.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Fan_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%pre -n openlmi-powermanagement
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_PowerManagement.mof \
        %{_datadir}/%{name}/60_LMI_PowerManagement.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_PowerManagement_Profile.mof || :;
fi >> %logfile 2>&1

%pre -n openlmi-service
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Service.mof \
        %{_datadir}/%{name}/60_LMI_Service.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Service_Profile.mof || :;
fi >> %logfile 2>&1

%if 0%{with_account}
%pre -n openlmi-account
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Account.mof \
        %{_datadir}/%{name}/60_LMI_Account.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Account_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%pre -n openlmi-software
if [ "$1" -gt 1 ]; then
    # delete indication filters
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c sfcbd unregister \
        %{_datadir}/%{name}/70_LMI_SoftwareIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/70_LMI_SoftwareIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
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
        %{_datadir}/%{name}/50_LMI_LogicalFile.mof \
        %{_datadir}/%{name}/50_LMI_LogicalFile.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_LogicalFile_Profile.mof || :;
fi >> %logfile 2>&1

%if 0%{with_realmd}
%pre -n openlmi-realmd
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Realmd.mof \
        %{_datadir}/%{name}/60_LMI_Realmd.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Realmd_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%pre -n openlmi-hardware
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Hardware.mof \
        %{_datadir}/%{name}/60_LMI_Hardware.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile.mof \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile_DMTF.mof || :;
fi >> %logfile 2>&1

%if 0%{with_pcp}
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
%endif

%if 0%{with_journald}
%pre -n openlmi-journald
if [ "$1" -gt 1 ]; then
    # delete indication filters
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c sfcbd unregister \
        %{_datadir}/%{name}/70_LMI_JournaldIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/70_LMI_JournaldIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Journald.mof \
        %{_datadir}/%{name}/60_LMI_Journald.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Journald_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%if 0%{with_locale}
%pre -n openlmi-locale
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Locale.mof \
        %{_datadir}/%{name}/60_LMI_Locale.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Locale_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%if 0%{with_sssd}
%pre -n openlmi-sssd
if [ "$1" -gt 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_SSSD.mof \
        %{_datadir}/%{name}/60_LMI_SSSD.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_SSSD_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%if 0%{with_fan}
%post -n openlmi-fan
# Register Schema and Provider
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_Fan.mof \
        %{_datadir}/%{name}/60_LMI_Fan.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Fan_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%post -n openlmi-powermanagement
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_PowerManagement.mof \
        %{_datadir}/%{name}/60_LMI_PowerManagement.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_PowerManagement_Profile.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-service
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_Service.mof \
        %{_datadir}/%{name}/60_LMI_Service.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Service_Profile.mof || :;
fi >> %logfile 2>&1

%if 0%{with_account}
%post -n openlmi-account
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_Account.mof \
        %{_datadir}/%{name}/60_LMI_Account.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Account_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%post -n openlmi-software
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_Software.mof \
        %{_datadir}/%{name}/LMI_Software.reg || :;
    # install indication filters for sfcbd
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c sfcbd register \
        %{_datadir}/%{name}/70_LMI_SoftwareIndicationFilters.mof || :;
        %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus register \
        %{_datadir}/%{name}/70_LMI_SoftwareIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Software_Profile.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -c tog-pegasus register \
        %{_datadir}/%{name}/60_LMI_Software_MethodParameters.mof || :;
fi >> %logfile 2>&1

%post -n openlmi-logicalfile
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/50_LMI_LogicalFile.mof \
        %{_datadir}/%{name}/50_LMI_LogicalFile.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_LogicalFile_Profile.mof || :;
fi >> %logfile 2>&1

%if 0%{with_realmd}
%post -n openlmi-realmd
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_Realmd.mof \
        %{_datadir}/%{name}/60_LMI_Realmd.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Realmd_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%post -n openlmi-hardware
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_Hardware.mof \
        %{_datadir}/%{name}/60_LMI_Hardware.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile.mof \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile_DMTF.mof || :;
fi >> %logfile 2>&1

%if 0%{with_journald}
%post -n openlmi-journald
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_Journald.mof \
        %{_datadir}/%{name}/60_LMI_Journald.reg || :;
    # install indication filters for sfcbd
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c sfcbd register \
        %{_datadir}/%{name}/70_LMI_JournaldIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus register \
        %{_datadir}/%{name}/70_LMI_JournaldIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Journald_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%if 0%{with_locale}
%post -n openlmi-locale
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_Locale.mof \
        %{_datadir}/%{name}/60_LMI_Locale.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Locale_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%if 0%{with_sssd}
%post -n openlmi-sssd
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} register \
        %{_datadir}/%{name}/60_LMI_SSSD.mof \
        %{_datadir}/%{name}/60_LMI_SSSD.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_SSSD_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%if 0%{with_fan}
%preun -n openlmi-fan
# Deregister only if not upgrading
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Fan.mof \
        %{_datadir}/%{name}/60_LMI_Fan.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Fan_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%preun -n openlmi-powermanagement
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_PowerManagement.mof \
        %{_datadir}/%{name}/60_LMI_PowerManagement.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_PowerManagement_Profile.mof || :;
fi >> %logfile 2>&1

%preun -n openlmi-service
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Service.mof \
        %{_datadir}/%{name}/60_LMI_Service.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Service_Profile.mof || :;
fi >> %logfile 2>&1

%if 0%{with_account}
%preun -n openlmi-account
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Account.mof \
        %{_datadir}/%{name}/60_LMI_Account.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Account_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%preun -n openlmi-software
if [ "$1" -eq 0 ]; then
    # delete indication filters
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c sfcbd unregister \
        %{_datadir}/%{name}/70_LMI_SoftwareIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/70_LMI_SoftwareIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
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
        %{_datadir}/%{name}/50_LMI_LogicalFile.mof \
        %{_datadir}/%{name}/50_LMI_LogicalFile.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_LogicalFile_Profile.mof || :;
fi >> %logfile 2>&1

%if 0%{with_realmd}
%preun -n openlmi-realmd
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Realmd.mof \
        %{_datadir}/%{name}/60_LMI_Realmd.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Realmd_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%preun -n openlmi-hardware
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Hardware.mof \
        %{_datadir}/%{name}/60_LMI_Hardware.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile.mof \
        %{_datadir}/%{name}/90_LMI_Hardware_Profile_DMTF.mof || :;
fi >> %logfile 2>&1

%if 0%{with_pcp}
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
%endif

%if 0%{with_journald}
%preun -n openlmi-journald
if [ "$1" -eq 0 ]; then
    # delete indication filters
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c sfcbd unregister \
        %{_datadir}/%{name}/70_LMI_JournaldIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/70_LMI_JournaldIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Journald.mof \
        %{_datadir}/%{name}/60_LMI_Journald.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Journald_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%if 0%{with_locale}
%preun -n openlmi-locale
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_Locale.mof \
        %{_datadir}/%{name}/60_LMI_Locale.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Locale_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%if 0%{with_sssd}
%preun -n openlmi-sssd
if [ "$1" -eq 0 ]; then
    %{_bindir}/openlmi-mof-register -v %{providers_version} unregister \
        %{_datadir}/%{name}/60_LMI_SSSD.mof \
        %{_datadir}/%{name}/60_LMI_SSSD.reg || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n %{interop} -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_SSSD_Profile.mof || :;
fi >> %logfile 2>&1
%endif

%changelog
* Thu Sep 04 2014 Radek Novacek <rnovacek@redhat.com> 0.5.0-1
- Move openlmicommon, indicationsender, jobmanager and indicationmanager into -libs subpackage
- Version 0.5.0

* Tue Jul  8 2014 Jan Safranek <jsafrane@redhat.com> 0.4.2-18
- Merged all -devel packages into one.

* Thu Jul  3 2014 Peter Schiffer <pschiffe@redhat.com> 0.4.2-17
- Renamed 60_LMI_LogicalFile.mof to 50_LMI_LogicalFile.mof

* Thu Jun 26 2014 Radek Novacek <rnovacek@redhat.com> 0.4.2-16
- Add BR: json-glib-devel for jobmanager

* Wed Jun 25 2014 Michal Minar <miminar@redhat.com> 0.4.2-15
- Added openlmi-jobmanager-libs subpackage
- Added openlmi-jobmanager-libs-devel subpackage

* Fri Jun 20 2014 Michal Minar <miminar@redhat.com> 0.4.2-14
- Added openlmi-indicationsender-libs subpackage
- Added openlmi-indicationsender-libs-devel subpackage

* Mon Jun 09 2014 Vitezslav Crhonek <vcrhonek@redhat.com> - 0.4.2-13
- Add openlmi-locale-doc subpackage

* Thu May 29 2014 Vitezslav Crhonek <vcrhonek@redhat.com> - 0.4.2-12
- Add openlmi-locale subpackage

* Mon May 26 2014 Tomas Bzatek <tbzatek@redhat.com> 0.4.2-11
- Include journald indication filters

* Tue May 20 2014 Radek Novacek <rnovacek@redhat.com> - 0.4.2-10
- Use PG_InterOp on RHEL-6 with tog-pegasus

* Thu May 15 2014 Radek Novacek <rnovacek@redhat.com> - 0.4.2-9
- Use python2.6 as a provider location in .reg file

* Wed May  7 2014 Jan Synáček <jsynacek@redhat.com> - 0.4.2-8
- Install LogicalFile configuration

* Tue May  6 2014 Jan Synáček <jsynacek@redhat.com> - 0.4.2-7
- Fix indmanager paths

* Tue May  6 2014 Jan Synáček <jsynacek@redhat.com> - 0.4.2-6
- Change openlmi.so* back to openlmicommon.so*

* Wed Apr 30 2014 Jan Synáček <jsynacek@redhat.com> - 0.4.2-5
- Change openlmicommon.so* to openlmi.so*

* Mon Apr 14 2014 Tomas Bzatek <tbzatek@redhat.com> 0.4.2-4
- Disable the openlmi-devassistant subpackage on rhel

* Mon Feb 24 2014 Tomas Bzatek <tbzatek@redhat.com> 0.4.2-3
- Added openlmi-devassistant subpackage

* Wed Feb 12 2014 Peter Schiffer <pschiffe@redhat.com> 0.4.2-2
- Added dependency on virt-what to hardware subpackage

* Tue Jan  7 2014 Jan Safranek <jsafrane@redhat.com> 0.4.2-1
- Version 0.4.2

* Tue Dec 17 2013 Michal Minar <miminar@redhat.com> 0.4.1-5
- Added new openlmi-python-test subpackage.

* Mon Nov 25 2013 Stephen Gallagher <sgallagh@redhat.com> 0.4.1-4
- Define OpenLMI 1.0.0
- Set strict version dependencies for the meta-package

* Wed Nov 06 2013 Tomas Bzatek <tbzatek@redhat.com> 0.4.1-3
- Added explicit dependency on new libuser release

* Tue Nov 05 2013 Peter Schiffer <pschiffe@redhat.com> 0.4.1-2
- Added dependency on smartmontools to hardware subpackage

* Mon Nov 04 2013 Radek Novacek <rnovacek@redhat.com> 0.4.1-1
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

