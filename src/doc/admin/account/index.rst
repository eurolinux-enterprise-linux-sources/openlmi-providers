.. OpenLMI Account documentation master file, created by
   sphinx-quickstart on Wed Aug 21 16:15:23 2013.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

OpenLMI Account Provider documentation
===========================================
OpenLMI Account is CIM provider which manages POSIX accounts.
It allows to create, delete and modify users and groups.

The provider implements DMTF identity profile, for more details read
:ref:`DMTF profile <dmtf-profile>`.

Contents:

.. toctree::
   :maxdepth: 2

   dmtf-profile
   usage

.. ifconfig:: includeClasses

   OpenLMI Account CIM Classes:

   .. toctree::
      :maxdepth: 1

      mof/tree
      mof/index
