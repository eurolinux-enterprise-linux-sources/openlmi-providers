#!/bin/bash

# This script generates Python code skeleton for each class defined in the MOF files.
# At the moment, MOF files need to be registered in the CIMOM in order to be able
# to generate the code. This is rather unfortunate and requires root privileges.
#
# Make sure to set proper LMI_CIMOM_USERNAME and LMI_CIMOM_PASSWORD env. variables!
#
# This script takes optional argument of project name, i.e. when regenerating within
# an existing project. Leave blank for devassistant template preparation.


SITE_PACKAGES=`python -c "from distutils.sysconfig import get_python_lib; print get_python_lib()"`
TMPL_NAME="{{PROJECT_NAME}}"
TMPL_SUFFIX=".tpl"

if [ ! -z $1 ]; then
  TMPL_NAME=$1
  TMPL_SUFFIX=""
fi

openlmi-mof-register --just-mofs register ../mof/*.mof*
rm -f ../mof/${TMPL_NAME}.reg${TMPL_SUFFIX}

pushd ../src/lmi/*
for i in `cat ../../../mof/*.mof* | grep '^class.*:' | sed 's/^class[\t ]*\(.*\)[\t ]*:.*/\1/'`; do
  echo "Generating code for class '${i}'"
  python > ${i}.py << PYGEN
import pywbem
import pywbem.cim_provider2
import os

username = os.environ.get("LMI_CIMOM_USERNAME", "root")
password = os.environ.get("LMI_CIMOM_PASSWORD", "")

con = pywbem.WBEMConnection("https://localhost", (username, password))
cc = con.GetClass("${i}", "root/cimv2")

python_code, registration = pywbem.cim_provider2.codegen(cc)
print python_code

PYGEN

  # Fill the reg file
  cat >> ../../../mof/${TMPL_NAME}.reg${TMPL_SUFFIX} << REG
[${i}]
   provider: ${SITE_PACKAGES}/lmi/${TMPL_NAME}/${i}.py
   location: pyCmpiProvider
   type: instance
   namespace: root/cimv2
   group: pycmpi${TMPL_NAME}

REG

done
popd

openlmi-mof-register --just-mofs unregister ../mof/*.mof*
