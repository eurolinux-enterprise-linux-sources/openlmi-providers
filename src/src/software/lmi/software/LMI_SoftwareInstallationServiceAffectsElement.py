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

"""Python Provider for LMI_SoftwareInstallationServiceAffectsElement

Instruments the CIM class LMI_SoftwareInstallationServiceAffectsElement
"""

import pywbem
from pywbem.cim_provider2 import CIMProvider2

from lmi.providers import cmpi_logging
from lmi.providers import ComputerSystem
from lmi.software import util
from lmi.software.core import Identity
from lmi.software.core import InstallationService
from lmi.software.core import InstallationServiceAffectsElement
from lmi.software.yumdb import YumDB

LOG = cmpi_logging.get_logger(__name__)

def generate_service_referents(env, model, object_name, keys_only):
    """
    Used in references method.
    """
    InstallationService.check_path(env, object_name, "object_name")
    InstallationServiceAffectsElement.fill_model_computer_system(
            env, model, keys_only=keys_only)
    yield model

    avail_model = util.new_instance_name("LMI_SoftwareIdentity")
    for pkg_info in YumDB.get_instance().get_package_list('available'):
        model["AffectedElement"] = InstallationServiceAffectsElement. \
                fill_model_identity(model, pkg_info,
                        keys_only=keys_only,
                        identity_model=avail_model)
        yield model

class LMI_SoftwareInstallationServiceAffectsElement(CIMProvider2):
    """Instrument the CIM class LMI_SoftwareInstallationServiceAffectsElement

    ServiceAffectsElement represents an association between a Service and
    the ManagedElements that might be affected by its execution.
    Instantiating this association indicates that running the service may
    change, manage, provide functionality for,or pose some burden on the
    ManagedElement. This burden might affect performance, throughput,
    availability, and so on.
    """

    def __init__ (self, _env):
        self.values = InstallationServiceAffectsElement.Values

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
        InstallationService.check_path_property(
                env, model, "AffectingElement")

        ch = env.get_cimom_handle()

        affected = model["AffectedElement"]
        if ch.is_subclass(affected.namespace,
                sub=affected.classname, super='LMI_SoftwareIdentity'):
            pkg_info = Identity.object_path2pkg(affected, kind='available')
            InstallationServiceAffectsElement.fill_model_identity(
                    model, pkg_info, keys_only=False)
        elif ch.is_subclass(affected.namespace,
                sub=affected.classname,
                super="CIM_ComputerSystem"):
            InstallationServiceAffectsElement.fill_model_computer_system(
                    env, model, keys_only=False)
        else:
            LOG().error("Unhandled classname: %s", affected.classname)
            raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                    "Provided AffectedElement not affected by this service.")

        return model


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
        model.path.update({'AffectedElement': None, 'AffectingElement': None})

        model['AffectingElement'] = InstallationService.get_path(env)
        InstallationServiceAffectsElement.fill_model_computer_system(
                env, model, keys_only=keys_only)
        yield model

        avail_model = util.new_instance_name("LMI_SoftwareIdentity")
        for pkg_info in YumDB.get_instance().get_package_list('available'):
            model["AffectedElement"] = InstallationServiceAffectsElement. \
                    fill_model_identity(model, pkg_info, keys_only=keys_only,
                            identity_model=avail_model)
            yield model

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

    @cmpi_logging.trace_method
    def references(self, env, object_name, model, result_class_name, role,
                   result_role, keys_only):
        """Instrument Associations.

        All four association-related operations (Associators, AssociatorNames,
        References, ReferenceNames) are mapped to this method.
        This method is a python generator

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName that defines the source
            CIM Object whose associated Objects are to be returned.
        model -- A template pywbem.CIMInstance to serve as a model
            of the objects to be returned.  Only properties present on this
            model need to be set.
        result_class_name -- If not empty, this string acts as a filter on
            the returned set of Instances by mandating that each returned
            Instances MUST represent an association between object_name
            and an Instance of a Class whose name matches this parameter
            or a subclass.
        role -- If not empty, MUST be a valid Property name. It acts as a
            filter on the returned set of Instances by mandating that each
            returned Instance MUST refer to object_name via a Property
            whose name matches the value of this parameter.
        result_role -- If not empty, MUST be a valid Property name. It acts
            as a filter on the returned set of Instances by mandating that
            each returned Instance MUST represent associations of
            object_name to other Instances, where the other Instances play
            the specified result_role in the association (i.e. the
            name of the Property in the Association Class that refers to
            the Object related to object_name MUST match the value of this
            parameter).
        keys_only -- A boolean.  True if only the key properties should be
            set on the generated instances.

        The following diagram may be helpful in understanding the role,
        result_role, and result_class_name parameters.
        +------------------------+                    +-------------------+
        | object_name.classname  |                    | result_class_name |
        | ~~~~~~~~~~~~~~~~~~~~~  |                    | ~~~~~~~~~~~~~~~~~ |
        +------------------------+                    +-------------------+
           |              +-----------------------------------+      |
           |              |  [Association] model.classname    |      |
           | object_name  |  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~    |      |
           +--------------+ object_name.classname REF role    |      |
        (CIMInstanceName) | result_class_name REF result_role +------+
                          |                                   |(CIMInstanceName)
                          +-----------------------------------+

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_NOT_SUPPORTED
        CIM_ERR_INVALID_NAMESPACE
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized
            or otherwise incorrect parameters)
        CIM_ERR_FAILED (some other unspecified error occurred)
        """
        ch = env.get_cimom_handle()

        model.path.update({'AffectedElement': None, 'AffectingElement': None})
        try:
            if (   (not role or role.lower() == 'affectingelement')
               and ch.is_subclass(object_name.namespace,
                       sub=object_name.classname,
                       super="LMI_SoftwareInstallationService")):
                for ref in generate_service_referents(
                        env, model, object_name, keys_only):
                    yield ref

            elif (   (not role or role.lower() == 'affectedelement')
                 and ch.is_subclass(object_name.namespace,
                         sub=object_name.classname,
                         super="CIM_ComputerSystem")):
                ComputerSystem.check_path(env, object_name, "object_name")
                model = InstallationServiceAffectsElement. \
                        fill_model_computer_system(
                                env, model, keys_only=keys_only)
                model["AffectingElement"] = InstallationService.get_path(env)
                yield model

            elif (   (not role or role.lower() == 'affectedelement')
                 and ch.is_subclass(object_name.namespace,
                         sub=object_name.classname,
                         super="LMI_SoftwareIdentity")):
                pkg_info = Identity.object_path2pkg(object_name, "available")
                model = InstallationServiceAffectsElement. \
                        fill_model_identity(model, pkg_info,
                                keys_only=keys_only)
                model["AffectingElement"] = InstallationService.get_path(env)
                yield model

        except pywbem.CIMError as exc:
            if exc.args[0] != pywbem.CIM_ERR_NOT_FOUND:
                LOG().exception('failed to generate referents of "%s"',
                        object_name)
                raise
