# PCP bridge Providers
#
# Copyright (C) 2013-2014 Red Hat, Inc.  All rights reserved.
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
# Authors: Frank Ch. Eigler <fche@redhat.com>
#

"""Python Provider for PCP_Metric_*
Instruments the CIM class family PCP_Metric_*
"""

from pywbem.cim_provider2 import CIMProvider2
import pywbem
from pcp import pmapi
import cpmapi as c_api
import datetime

context = None  # persistent pcp context


# Since we serve a whole slew of PCP_Metric_** subclasses, we can't
# use a straight classname->provider-class dictionary.
#
#def get_providers(env):
#    return {'PCP_Metric_****': PCP_MetricProvider}
#
# Instead, we implement the one-hop-higher proxy-level function calls,
# namely the IM_* functions at the bottom.


# Undo mangling done by PCP_pmns2mofreg.sh
def MOFname_to_PCPmetric (op):
    assert (op.namespace == 'root/cimv2')
    assert (op.classname[0:11] == 'PCP_Metric_')
    return op.classname[11:].replace('__', '.')


# Search the given PM_ResultSet for a given instance number (if any);
# return formatted CIM of the value (or CIM None)
def PCP_CIMValueString (context, result, desc, inst):
    for i in range (0,result.contents.get_numval(0)):
        value = result.contents.get_vlist(0,i)
        iv = value.inst
        if (inst is not None and inst != iv):
            continue
        atom = context.pmExtractValue (result.contents.get_valfmt(0),
                                       value,
                                       desc.contents.type,
                                       desc.contents.type)
        atomValue = atom.dref(desc.contents.type) # nb: atomValue could be numeric etc.
        return pywbem.CIMProperty(name='ValueString',
                                  value=str(atomValue), # stringify it here
                                  type='string')

    return pywbem.CIMProperty(name='ValueString',
                              value=None,
                              type='string')


def PCP_CIMStatisticTime (result):
    dt = datetime.datetime.fromtimestamp(float(str(result.contents.timestamp)))
    return pywbem.CIMDateTime(dt)


# generic payload generator, used for
# - iterating across instance domains (keys_only=1)
# - fetching metric values
# - fetching metric metadata (for those model/filter fields set)
def get_instance (env, op, model, keys_only):
    metric = MOFname_to_PCPmetric (op)
    global context

    try:
        if (context == None):
            context = pmapi.pmContext() # localhost or equivalent
        context.pmReconnectContext() # in case it was nuked recently
    except pmapi.pmErr, e:
        context = None
        raise pywbem.CIMError(pywbem.CIM_ERR_FAILED, "Unable to connect to local PMCD:" + str(e))

    pmids = context.pmLookupName(metric)
    pmid = pmids[0]
    desc = context.pmLookupDesc(pmid)

    if ('InstanceID' in model):
        selected_instanceid = model['InstanceID']
    else:
        selected_instanceid = None

    model.path.update({'InstanceID':None})

    if (not keys_only):
        model['PMID'] = pywbem.Uint32(pmid)
        model['ElementName'] = metric
        # must not fail, or else we have no metric value data worth sharing
        try:
            results = context.pmFetch(pmids)
        except pmapi.pmErr, e:
            # fatal
            raise pywbem.CIMError(pywbem.CIM_ERR_FAILED, "PCP pmFetch failed:" + str(e))
        # cannot fail
        model['Units'] = context.pmUnitsStr(desc.contents.units)
        model['Type'] = context.pmTypeStr(desc.contents.type)
        # these may fail, but not fatally
        try:
            model['Caption'] = context.pmLookupText(pmid)
        except pmapi.pmErr:
            pass
        try:
            model['Description'] = context.pmLookupText(pmid, c_api.PM_TEXT_HELP)
        except pmapi.pmErr:
            pass

    try:
        instL, nameL = context.pmGetInDom(desc)
        for iL, nL in zip(instL, nameL):
            new_instanceid = 'PCP:'+metric+':'+nL
            if (selected_instanceid is None or
                new_instanceid == selected_instanceid):
                model['InstanceNumber'] = pywbem.Uint32(iL)
                model['InstanceName'] = nL
                model['InstanceID'] = new_instanceid
                if (not keys_only):
                    model['StatisticTime'] = PCP_CIMStatisticTime (results)
                    model['ValueString'] = PCP_CIMValueString (context, results, desc, iL)
                yield model
    except pmapi.pmErr: # pmGetInDom is expected to fail for non-instance (PM_INDOM_NULL) metrics
        new_instanceid = 'PCP:'+metric
        if (selected_instanceid is None or
            new_instanceid == selected_instanceid):
            model['InstanceNumber'] = pywbem.CIMProperty(name='InstanceNumber',
                                                         value=None,type='uint32')
            model['InstanceName'] = pywbem.CIMProperty(name='InstanceName',
                                                       value=None,type='string')
            model['InstanceID'] = new_instanceid
            if (not keys_only):
                model['StatisticTime'] = PCP_CIMStatisticTime (results)
                model['ValueString'] = PCP_CIMValueString (context, results, desc, None)
            yield model

# hooks for impersonating CIMProvider2 functions


def MI_enumInstanceNames (env, op):
    model = pywbem.CIMInstance(classname = op.classname, path=op)
    for x in get_instance (env, op, model, True):
        yield x.path

def MI_enumInstances (env, op, plist):
    model = pywbem.CIMInstance(classname = op.classname, path=op)
    return get_instance (env, op, model, False)

def MI_getInstance (env, op, plist):
    proplist = None
    if plist is not None:
        proplist = [s.lower() for s in propertyList]
        proplist+= [s.lower() for s in op.keybindings.keys()]
    model = pywbem.CIMInstance(classname=op.classname, path=op,
                               property_list=proplist)
    model.update(model.path.keybindings)
    for x in get_instance (env, op, model, False):
        return x # XXX: first one

def MI_createInstance (env, pinst):
    raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED)

def MI_modifyInstance (env, pinst, plist):
    raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED)

def MI_deleteInstance (env, piname):
    raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED)

# See also extra MI_* functions for associations, etc.;
# cmpi-bindings.git swig/python/cmpi_pywbem_bindings.py
