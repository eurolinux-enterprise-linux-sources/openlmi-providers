OpenLMI Service Provider documentation
===========================================
OpenLMI Service is CIM provider for managing Linux system services (using
the systemd D-Bus interface).

It allows to enumerate system services and get their status, start/stop/restart/...
a service and enable/disable a service.

The provider should be also able to do event based monitoring of service status
(emit indication event upon service status change) in the future.

Contents:

.. toctree::
   :maxdepth: 2

   usage

.. ifconfig:: includeClasses

    OpenLMI Service CIM Classes:

    .. toctree::
       :maxdepth: 1

       mof/tree
       mof/index
