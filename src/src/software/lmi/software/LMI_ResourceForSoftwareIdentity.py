# -*- encoding: utf-8 -*-
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

"""Python Provider for LMI_ResourceForSoftwareIdentity

Instruments the CIM class LMI_ResourceForSoftwareIdentity
"""

from pywbem.cim_provider2 import CIMProvider2
import logging
import pywbem

from lmi.providers import cmpi_logging
from lmi.software import util
from lmi.software.core import generate_references
from lmi.software.core import Identity
from lmi.software.core import IdentityResource
from lmi.software.yumdb import YumDB

@cmpi_logging.trace_function
def generate_package_referents(env, object_name, model, _keys_only):
    """
    Generates models of repositories holding package represented
    by object_name.
    """
    with YumDB.get_instance() as ydb:
        pkg_infos = Identity.object_path2pkg(
                object_name, 'available', return_all=True)
        for pkg_info in pkg_infos:
            repos = ydb.filter_repositories('enabled', repoid=pkg_info.repoid)
            model['ManagedElement'] = Identity.pkg2model(pkg_info)
            for repo in repos:
                model["AvailableSAP"] = \
                        IdentityResource.repo2model(env, repo)
                yield model

@cmpi_logging.trace_function
def generate_repository_referents(env, object_name, model, _keys_only):
    """
    Generates models of software identities contained in repository
    represented by object_name.
    """
    with YumDB.get_instance() as ydb:
        repo = IdentityResource.object_path2repo(
                env, object_name, 'all')
        pkglist = ydb.get_package_list('available',
                allow_duplicates=True, sort=True,
                include_repos=repo.repoid,
                exclude_repos='*')
        model['AvailableSAP'] = IdentityResource.repo2model(env, repo)
        pkg_model = util.new_instance_name("LMI_SoftwareIdentity")
        for pkg_info in pkglist:
            model["ManagedElement"] = Identity.pkg2model(
                            pkg_info, model=pkg_model)
            yield model

class LMI_ResourceForSoftwareIdentity(CIMProvider2):
    """Instrument the CIM class LMI_ResourceForSoftwareIdentity

    CIM_SAPAvailableForElement conveys the semantics of a Service Access
    Point that is available for a ManagedElement. When
    CIM_SAPAvailableForElement is not instantiated, then the SAP is
    assumed to be generally available. If instantiated, the SAP is
    available only for the associated ManagedElements. For example, a
    device might provide management access through a URL. This association
    allows the URL to be advertised for the device.

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
        if not isinstance(model['ManagedElement'], pywbem.CIMInstanceName):
            raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                    "Expected object path for ManagedElement!")
        if not isinstance(model['AvailableSAP'], pywbem.CIMInstanceName):
            raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                    "Expected object path for AvailableSAP!")
        if not "Name" in model['AvailableSAP']:
            raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                    'Missing key property "Name" in AvailableSAP!')

        with YumDB.get_instance() as ydb:
            repoid = model["AvailableSAP"]["Name"]
            pkg_info = Identity.object_path2pkg(
                    model["ManagedElement"],
                    kind='available',
                    include_repos=repoid,
                    exclude_repos='*',
                    repoid=repoid)
            repos = ydb.filter_repositories('all', repoid=repoid)
            if len(repos) < 1:
                raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                        'Unknown repository "%s".' % repoid)
            repo = repos[0]
            repo = IdentityResource.object_path2repo(
                    env, model["AvailableSAP"], 'all')

            model["AvailableSAP"] = IdentityResource.repo2model(env, repo)
            model["ManagedElement"] = Identity.pkg2model(pkg_info)

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
        # Prime model.path with knowledge of the keys, so key values on
        # the CIMInstanceName (model.path) will automatically be set when
        # we set property values on the model.
        model.path.update({'ManagedElement': None, 'AvailableSAP': None})

        elem_model = util.new_instance_name("LMI_SoftwareIdentity")
        sap_model = util.new_instance_name("LMI_SoftwareIdentityResource")
        # maps repoid to instance name
        with YumDB.get_instance() as ydb:
            pl = ydb.get_package_list('available',
                    allow_duplicates=True,
                    sort=True)
            repos = dict(  (r.repoid, r)
                        for r in ydb.get_repository_list('enabled'))
            for pkg in pl:
                try:
                    repo = repos[pkg.repoid]
                except KeyError:
                    logging.getLogger(__name__).error(
                        'unknown or disabled repository "%s" for package "%s"',
                        pkg, pkg.repoid)
                model["AvailableSAP"] = IdentityResource.repo2model(
                        env, repo, model=sap_model)
                model["ManagedElement"] = Identity.pkg2model(
                        pkg, model=elem_model)
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
    def MI_associators(self,
            env,
            objectName,
            assocClassName,
            resultClassName,
            role,
            resultRole,
            propertyList):
        """
        Yield identities associated to ``LMI_SoftwareIdentityResource``
        instances.

        This overrides method of superclass for a very good reason. Original
        method calls ``GetInstance()`` on every single object path returned by
        :py:meth:`LMI_ResourceForSoftwareIdentity.references` which is very time
        consuming operation on ``LMI_SoftwareIdentity`` class in particular.
        On slow machine this could take several minutes. This method just
        enumerates requested instances directly.
        """
        ns = util.Configuration.get_instance().namespace
        ch = env.get_cimom_handle()
        if (   (not role or role.lower() == "availablesap")
           and (not resultRole or resultRole.lower() == "managedelement")
           and ch.is_subclass(ns,
               sub=objectName.classname,
               super="LMI_SoftwareIdentityResource")):
            try:
                repo = IdentityResource.object_path2repo(
                        env, objectName, 'all')
            except pywbem.CIMError as e:
                if e.args[0] != pywbem.CIM_ERR_NOT_FOUND:
                    raise
                raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                        e.args[1])

            model = pywbem.CIMInstance(classname='LMI_SoftwareIdentity')
            model.path = util.new_instance_name("LMI_SoftwareIdentity")

            with YumDB.get_instance() as ydb:
                pkglist = ydb.get_package_list('available',
                        allow_duplicates=True, sort=True,
                        include_repos=repo.repoid,
                        exclude_repos='*')
                for pkg_info in pkglist:
                    yield Identity.pkg2model(pkg_info, model=model,
                            keys_only=False)

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
                ("AvailableSAP", "LMI_SoftwareIdentityResource",
                    generate_repository_referents),
                ("ManagedElement", "LMI_SoftwareIdentity",
                    generate_package_referents)
        ]

        for ref in generate_references(env, object_name, model,
                result_class_name, role, result_role, keys_only, handlers):
            yield ref

