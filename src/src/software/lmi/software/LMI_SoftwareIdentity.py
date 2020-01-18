# -*- encoding: utf-8 -*-
# Software Management Providers
#
# Copyright (C) 2012-2013 Red Hat, Inc.  All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

"""Python Provider for LMI_SoftwareIdentity

Instruments the CIM class LMI_SoftwareIdentity

"""

import pywbem
from pywbem.cim_provider2 import CIMProvider2

from lmi.providers import cmpi_logging
from lmi.software.core import Identity
from lmi.software.yumdb import YumDB

class LMI_SoftwareIdentity(CIMProvider2):
    """Instrument the CIM class LMI_SoftwareIdentity

    SoftwareIdentity provides descriptive information about a software
    component for asset tracking and/or installation dependency
    management. When the IsEntity property has the value TRUE, the
    instance of SoftwareIdentity represents an individually identifiable
    entity similar to Physical Element. SoftwareIdentity does NOT indicate
    whether the software is installed, executing, etc. This extra
    information may be provided through specialized associations to
    Software Identity. For instance, both InstalledSoftwareIdentity and
    ElementSoftwareIdentity may be used to indicate that the software
    identified by this class is installed. SoftwareIdentity is used when
    managing the software components of a ManagedElement that is the
    management focus. Since software may be acquired, SoftwareIdentity can
    be associated with a Product using the ProductSoftwareComponent
    relationship. The Application Model manages the deployment and
    installation of software via the classes, SoftwareFeatures and
    SoftwareElements. SoftwareFeature and SoftwareElement are used when
    the software component is the management focus. The
    deployment/installation concepts are related to the asset/identity
    one. In fact, a SoftwareIdentity may correspond to a Product, or to
    one or more SoftwareFeatures or SoftwareElements - depending on the
    granularity of these classes and the deployment model. The
    correspondence of Software Identity to Product, SoftwareFeature or
    SoftwareElement is indicated using the ConcreteIdentity association.
    Note that there may not be sufficient detail or instrumentation to
    instantiate ConcreteIdentity. And, if the association is instantiated,
    some duplication of information may result. For example, the Vendor
    described in the instances of Product and SoftwareIdentity MAY be the
    same. However, this is not necessarily true, and it is why vendor and
    similar information are duplicated in this class.  Note that
    ConcreteIdentity can also be used to describe the relationship of the
    software to any LogicalFiles that result from installing it. As above,
    there may not be sufficient detail or instrumentation to instantiate
    this association.

    """

    def __init__(self, _env):
        pass

    @cmpi_logging.trace_method
    def get_instance(self, env, model):
        """Return an instance.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        model -- A template of the pywbem.CIMInstance to be returned.  The
            key properties are set on this instance to correspond to the
            instanceName that was requested.  The properties of the model
            are already filtered according to the PropertyList from the
            request.  Only properties present in the model need to be
            given values.  If you prefer, you can set all of the
            values, and the instance will be filtered for you.

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized
            or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the CIM Class does exist, but the requested CIM
            Instance does not exist in the specified namespace)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """
        with YumDB.get_instance():
            pkg_info = Identity.object_path2pkg(model.path, 'all')
            return Identity.pkg2model(pkg_info, keys_only=False, model=model)

    @cmpi_logging.trace_method
    def enum_instances(self, env, model, keys_only):
        """Enumerate instances.

        The WBEM operations EnumerateInstances and EnumerateInstanceNames
        are both mapped to this method.
        This method is a python generator

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        model -- A template of the pywbem.CIMInstances to be generated.
            The properties of the model are already filtered according to
            the PropertyList from the request.  Only properties present in
            the model need to be given values.  If you prefer, you can
            always set all of the values, and the instance will be filtered
            for you.
        keys_only -- A boolean.  True if only the key properties should be
            set on the generated instances.

        Possible Errors:
        CIM_ERR_FAILED (some other unspecified error occurred)

        """
        # too many available packages to enumerate them
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED,
                "Enumeration of instances is not supported.")

    @cmpi_logging.trace_method
    def set_instance(self, env, instance, modify_existing):
        """Return a newly created or modified instance.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        instance -- The new pywbem.CIMInstance.  If modifying an existing
            instance, the properties on this instance have been filtered by
            the PropertyList from the request.
        modify_existing -- True if ModifyInstance, False if CreateInstance

        Return the new instance.  The keys must be set on the new instance.

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_NOT_SUPPORTED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized
            or otherwise incorrect parameters)
        CIM_ERR_ALREADY_EXISTS (the CIM Instance already exists -- only
            valid if modify_existing is False, indicating that the operation
            was CreateInstance)
        CIM_ERR_NOT_FOUND (the CIM Instance does not exist -- only valid
            if modify_existing is True, indicating that the operation
            was ModifyInstance)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED)

    @cmpi_logging.trace_method
    def delete_instance(self, env, instance_name):
        """Delete an instance.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        instance_name -- A pywbem.CIMInstanceName specifying the instance
            to delete.

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_NOT_SUPPORTED
        CIM_ERR_INVALID_NAMESPACE
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized
            or otherwise incorrect parameters)
        CIM_ERR_INVALID_CLASS (the CIM Class does not exist in the specified
            namespace)
        CIM_ERR_NOT_FOUND (the CIM Class does exist, but the requested CIM
            Instance does not exist in the specified namespace)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED)

