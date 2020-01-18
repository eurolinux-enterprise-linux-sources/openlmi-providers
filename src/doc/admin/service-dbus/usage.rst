OpenLMI Service usage
=====================

Some common use cases are described in the following parts.

List services
-------------
List all services available on managed machine, print whether the service has been
started (TRUE), or stopped (FALSE) and print status string of the service::

    for service in c.root.cimv2.LMI_Service.instances():
        print "%s:\t%s" % (service.Name, service.Status)

List only enabled by default services (automatically started on boot). Note that value
of EnabledDefault property is '2' for enabled services (and it's '3' for disabled services)::

    service_cls = c.root.cimv2.LMI_Service
    for service in service_cls.instances():
        if service.EnabledDefault == service_cls.EnabledDefaultValues.Enabled:
            print service.Name

See available information about the 'cups' service::

    cups = c.root.cimv2.LMI_Service.first_instance({"Name" : "cups.service"})
    cups.doc()


Start/stop service
------------------
Start and stop 'cups' service, see status::

    cups = c.root.cimv2.LMI_Service.first_instance({"Name" : "cups.service"})
    cups.StartService()
    print cups.Status
    cups.StopService()
    print cups.Status

Enable/disable service
----------------------
Disable and enable 'cups' service, print EnabledDefault property::

    cups = c.root.cimv2.LMI_Service.first_instance({"Name" : "cups.service"})
    cups.TurnServiceOff()
    print cups.EnabledDefault
    cups.TurnServiceOn()
    print cups.EnabledDefault
