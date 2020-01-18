# Software Management Providers
#
# Copyright (C) 2012-2014 Red Hat, Inc.  All rights reserved.
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
#
# Authors: Michal Minar <miminar@redhat.com>
#

"""Python Provider for LMI_InstalledSoftwareIdentity

Instruments the CIM class LMI_InstalledSoftwareIdentity

"""

import pywbem
from pywbem.cim_provider2 import CIMProvider2

from lmi.providers import cmpi_logging
from lmi.providers import ComputerSystem
from lmi.software import util
from lmi.software.core import generate_references
from lmi.software.core import Identity
from lmi.software.yumdb import YumDB

LOG = cmpi_logging.get_logger(__name__)

@cmpi_logging.trace_function
def generate_system_referents(env, object_name, model, _keys_only):
    """
    Handler for referents enumeration request.
    """
    ComputerSystem.check_path(env, object_name, "object_name")

    model["System"] = ComputerSystem.get_path(env)
    pkg_model = util.new_instance_name('LMI_SoftwareIdentity')
    with YumDB.get_instance() as ydb:
        for pkg_info in ydb.get_package_list('installed', sort=True):
            model["InstalledSoftware"] = Identity.pkg2model(
                        pkg_info, model=pkg_model)
            yield model

@cmpi_logging.trace_function
def generate_package_referents(env, object_name, model, _keys_only):
    """
    Handler for referents enumeration request.
    """
    pkg_info = Identity.object_path2pkg(object_name, kind="installed")
    model['InstalledSoftware'] = Identity.pkg2model(pkg_info)
    model["System"] = ComputerSystem.get_path(env)
    yield model

class LMI_InstalledSoftwareIdentity(CIMProvider2):
    """Instrument the CIM class LMI_InstalledSoftwareIdentity

    The InstalledSoftwareIdentity association identifies the System on
    which a SoftwareIdentity is installed. This class is a corollary to
    InstalledSoftwareElement, but deals with the asset aspects of software
    (as indicated by SoftwareIdentity), versus the deployment aspects (as
    indicated by SoftwareElement).

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
        ComputerSystem.check_path_property(env, model, 'System')

        if not isinstance(model['InstalledSoftware'], pywbem.CIMInstanceName):
            raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                    "Expected object path for InstalledSoftware!")

        model["System"] = model.path["System"] = ComputerSystem.get_path(env)
        pkg_info = Identity.object_path2pkg(
            model['InstalledSoftware'], kind='installed')
        model['InstalledSoftware'] = Identity.pkg2model(pkg_info)
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
        model.path.update({'InstalledSoftware': None, 'System': None})

        model['System'] = ComputerSystem.get_path(env)
        inst_model = util.new_instance_name("LMI_SoftwareIdentity")
        with YumDB.get_instance() as yb:
            pl = yb.get_package_list('installed', sort=True)
            for pkg in pl:
                model['InstalledSoftware'] = Identity.pkg2model(
                        pkg, model=inst_model)
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

        if modify_existing:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED,
                    "Installed package can not be modified.")

        if not "InstalledSoftware" in instance:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                    "Missing InstalledSoftware property.")
        if not "System" in instance:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                    "Missing System property.")

        ComputerSystem.check_path_property(env, instance, 'System')
        with YumDB.get_instance() as ydb:
            pkg_info = Identity.object_path2pkg(
                    instance["InstalledSoftware"], kind="all")
            if pkg_info.installed:
                raise pywbem.CIMError(pywbem.CIM_ERR_ALREADY_EXISTS,
                        'Package "%s" is already installed.' % pkg_info)
            LOG().info('installing package "%s"' % pkg_info)
            installed = ydb.install_package(pkg_info)
            LOG().info('package "%s" installed' % pkg_info)
        instance["InstalledSoftware"] = Identity.pkg2model(installed)
        return instance

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
        if not "InstalledSoftware" in instance_name:
            raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                    "Missing InstalledSoftware property.")
        if not "System" in instance_name:
            raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                    "Missing System property.")
        ComputerSystem.check_path_property(env, instance_name, 'System')

        with YumDB.get_instance() as ydb:
            pkg_info = Identity.object_path2pkg(
                    instance_name["InstalledSoftware"], kind="installed")
            LOG().info('removing package "%s"' % pkg_info)
            ydb.remove_package(pkg_info)
            LOG().info('package "%s" removed' % pkg_info)

    @cmpi_logging.trace_method
    def MI_associators(self,
            env,
            objectName,
            assocClassName,
            resultClassName,
            role,
            resultRole,
            propertyList):
        """
        Yield instance names associated to ``ComputerSystem`` instance.

        This overrides method of superclass for a very good reason. Original
        method calls ``GetInstance()`` on every single object path returned by
        :py:meth:`LMI_InstalledSoftwareIdentity.references` which is very time
        consuming operation on ``LMI_SoftwareIdentity`` class in particular.
        On slow machine this could take several minutes. This method just
        enumerates requested instances directly.
        """
        ns = util.Configuration.get_instance().namespace
        ch = env.get_cimom_handle()
        if (   (not role or role.lower() == "system")
           and (not resultRole or resultRole.lower() == "installedsoftware")
           and ch.is_subclass(ns,
               sub=objectName.classname,
               super="CIM_ComputerSystem")):
            try:
                ComputerSystem.check_path(env, objectName, 'ObjectName')
            except pywbem.CIMError as e:
                if e.args[0] != pywbem.CIM_ERR_NOT_FOUND:
                    raise
                raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                        e.args[1])

            model = pywbem.CIMInstance(classname='LMI_SoftwareIdentity')
            model.path = util.new_instance_name('LMI_SoftwareIdentity')
            with YumDB.get_instance() as ydb:
                pkglist = ydb.get_package_list('installed', sort=True)
                for pkg_info in pkglist:
                    yield Identity.pkg2model(pkg_info,
                            keys_only=False, model=model)

        else:
            # no need to optimize associators for SoftwareIdentity
            for obj in CIMProvider2.MI_associators(self,
                    env,
                    objectName,
                    assocClassName,
                    resultClassName,
                    role,
                    resultRole,
                    propertyList):
                yield obj

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
        handlers = [
                ("System", "CIM_ComputerSystem", generate_system_referents),
                ("InstalledSoftware", "LMI_SoftwareIdentity",
                    generate_package_referents)
        ]

        for ref in generate_references(env, object_name, model,
                result_class_name, role, result_role, keys_only, handlers):
            yield ref

