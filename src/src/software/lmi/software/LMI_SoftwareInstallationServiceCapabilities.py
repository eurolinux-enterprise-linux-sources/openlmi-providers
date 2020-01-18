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

"""Python Provider for LMI_SoftwareInstallationServiceCapabilities

Instruments the CIM class LMI_SoftwareInstallationServiceCapabilities

"""

import pywbem
from pywbem.cim_provider2 import CIMProvider2

from lmi.providers import cmpi_logging
from lmi.software.core import InstallationServiceCapabilities

class LMI_SoftwareInstallationServiceCapabilities(CIMProvider2):
    """Instrument the CIM class LMI_SoftwareInstallationServiceCapabilities

    A subclass of capabilities that defines the capabilities of a
    SoftwareInstallationService. A single instance of
    SoftwareInstallationServiceCapabilities is associated with a
    SoftwareInstallationService using ElementCapabilities.
    """

    def __init__ (self, _env):
        self.values = InstallationServiceCapabilities.Values

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
        InstallationServiceCapabilities.check_path(env,
                model.path, "path")
        return InstallationServiceCapabilities.get_instance(model)

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
        model.path.update({'InstanceID': None})

        if keys_only:
            model['InstanceID'] = \
                    InstallationServiceCapabilities.get_path()['InstanceID']
        else:
            model = InstallationServiceCapabilities.get_instance(model)
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
    def cim_method_creategoalsettings(self, env, object_name,
            param_supportedgoalsettings=None,
            param_templategoalsettings=None):
        """Implements LMI_SoftwareInstallationServiceCapabilities. \
                CreateGoalSettings()

        Method to create a set of supported SettingData elements, from two
        sets of SettingData elements, provided by the caller.  CreateGoal
        should be used when the SettingData instances that represents the
        goal will not persist beyond the execution of the client and where
        those instances are not intended to be shared with other,
        non-cooperating clients.  Both TemplateGoalSettings and
        SupportedGoalSettings are represented as strings containing
        EmbeddedInstances of a CIM_SettingData subclass. These embedded
        instances do not exist in the infrastructure supporting this
        method but are maintained by the caller/client.  This method
        should return CIM_Error(s) representing that a single named
        property of a setting (or other) parameter (either reference or
        embedded object) has an invalid value or that an invalid
        combination of named properties of a setting (or other) parameter
        (either reference or embedded object) has been requested.  If the
        input TemplateGoalSettings is NULL or the empty string, this
        method returns a default SettingData element that is supported by
        this Capabilities element.  If the TemplateGoalSettings specifies
        values that cannot be supported, this method shall return an
        appropriate CIM_Error and should return a best match for a
        SupportedGoalSettings.  The client proposes a goal using the
        TemplateGoalSettings parameter and gets back Success if the
        TemplateGoalSettings is exactly supportable. It gets back
        "Alternative Proposed" if the output SupportedGoalSettings
        represents a supported alternative. This alternative should be a
        best match, as defined by the implementation.  If the
        implementation is conformant to a RegisteredProfile, then that
        profile may specify the algorithms used to determine best match. A
        client may compare the returned value of each property against the
        requested value to determine if it is left unchanged, degraded or
        upgraded.   Otherwise, if the TemplateGoalSettings is not
        applicable an "Invalid Parameter" error is returned.   When a
        mutually acceptable SupportedGoalSettings has been achieved, the
        client may use the contained SettingData instances as input to
        methods for creating a new object ormodifying an existing object.
        Also the embedded SettingData instances returned in the
        SupportedGoalSettings may be instantiated via CreateInstance,
        either by a client or as a side-effect of the execution of an
        extrinsic method for which the returned SupportedGoalSettings is
        passed as an embedded instance.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName
            specifying the object on which the method CreateGoalSettings()
            should be invoked.
        param_supportedgoalsettings --  The input parameter
                SupportedGoalSettings (type pywbem.CIMInstance(
                    classname='CIM_SettingData', ...))
            SupportedGoalSettings are elements of class CIM_SettingData, or
            a derived class.  At most, one instance of each SettingData
            subclass may be supplied.  All SettingData instances provided
            by this property are interpreted as a set, relative to this
            Capabilities instance.   To enable a client to provide
            additional information towards achieving the
            TemplateGoalSettings, an input set of SettingData instances
            may be provided. If not provided, this property shall be set
            to NULL on input.. Note that when provided, what property
            values are changed, and how, is implementation dependent and
            may be the subject of other standards.  If provided, the input
            SettingData instances must be ones that the implementation is
            able to support relative to the ManagedElement associated via
            ElementCapabilities. Typically, the input SettingData
            instances are created by a previous instantiation of
            CreateGoalSettings.  If the input SupportedGoalSettings is not
            supported by the implementation, then an "Invalid Parameter"
            (5) error is returned by this call. In this case, a
            corresponding CIM_ERROR should also be returned.  On output,
            this property is used to return the best supported match to
            the TemplateGoalSettings.  If the output SupportedGoalSettings
            matches the input SupportedGoalSettings, then the
            implementation is unable to improve further towards meeting
            the TemplateGoalSettings.

        param_templategoalsettings --  The input parameter
                TemplateGoalSettings (type pywbem.CIMInstance(
                    classname='CIM_SettingData', ...))
            If provided, TemplateGoalSettings are elements of class
            CIM_SettingData, or a derived class, that is used as the
            template to be matched. .  At most, one instance of each
            SettingData subclass may be supplied.  All SettingData
            instances provided by this property are interpreted as a set,
            relative to this Capabilities instance.  SettingData instances
            that are not relevant to this instance are ignored.  If not
            provided, it shall be set to NULL. In that case, a SettingData
            instance representing the default settings of the associated
            ManagedElement is used.


        Returns a two-tuple containing the return value (type pywbem.Uint16
            self.Values.CreateGoalSettings)
        and a list of CIMParameter objects representing the output parameters

        Output parameters:
        SupportedGoalSettings -- (type
                pywbem.CIMInstance(classname='CIM_SettingData', ...))
            SupportedGoalSettings are elements of class CIM_SettingData, or
            a derived class.  At most, one instance of each SettingData
            subclass may be supplied.  All SettingData instances provided
            by this property are interpreted as a set, relative to this
            Capabilities instance.   To enable a client to provide
            additional information towards achieving the
            TemplateGoalSettings, an input set of SettingData instances
            may be provided. If not provided, this property shall be set
            to NULL on input.. Note that when provided, what property
            values are changed, and how, is implementation dependent and
            may be the subject of other standards.  If provided, the input
            SettingData instances must be ones that the implementation is
            able to support relative to the ManagedElement associated via
            ElementCapabilities. Typically, the input SettingData
            instances are created by a previous instantiation of
            CreateGoalSettings.  If the input SupportedGoalSettings is not
            supported by the implementation, then an "Invalid Parameter"
            (5) error is returned by this call. In this case, a
            corresponding CIM_ERROR should also be returned.  On output,
            this property is used to return the best supported match to
            the TemplateGoalSettings.  If the output SupportedGoalSettings
            matches the input SupportedGoalSettings, then the
            implementation is unable to improve further towards meeting
            the TemplateGoalSettings.


        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate,
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)
        """
        return (self.values.CreateGoalSettings.Not_Supported, [])

