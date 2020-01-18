OpenLMI Journald usage
======================

The OpenLMI Journald provider depends on running journald daemon. See the `systemd
<http://www.freedesktop.org/software/systemd/man/systemd-journald.service.html>`_
manual for how to enable the journald service.


Listing a log
-------------

This example shows simple enumeration through available :ref:`LMI_JournalLogRecord<LMI-JournalLogRecord>`
instances in classic syslog-like format::

    #!/usr/bin/lmishell
    c = connect("localhost", "pegasus", "test")
    for rec in c.root.cimv2.LMI_JournalMessageLog.first_instance().associators():
        print "%s %s %s" % (rec.MessageTimestamp.datetime.ctime(), rec.HostName, rec.DataFormat)

.. note::
   Only a limited number of records are being enumerated and printed out, please
   see the :ref:`inst-enum-limit` remark.


Iterating through the log
-------------------------

This example uses iterator methods of the :ref:`LMI_JournalMessageLog<LMI-JournalMessageLog>`
class to continuously go through the whole journal::

    #!/usr/bin/lmishell
    c = connect("localhost", "pegasus", "test")
    inst = c.root.cimv2.LMI_JournalMessageLog.first_instance()
    r = inst.PositionToFirstRecord()
    iter_id = r.rparams['IterationIdentifier']
    while True:
        x = inst.GetRecord(IterationIdentifier=iter_id, PositionToNext=True)
        if x.rval != 0:
            break
        print "".join(map(chr, x.rparams['RecordData']))
        iter_id = x.rparams['IterationIdentifier']


Sending new message to log
--------------------------

Simple example that uses :ref:`LMI_JournalLogRecord.create_instance()<LMI-JournalLogRecord>`
CIM method to send a new message in the log::

    #!/usr/bin/lmishell
    c = connect("localhost", "pegasus", "test")
    c.root.cimv2.LMI_JournalLogRecord.create_instance({"CreationClassName": "LMI_JournalLogRecord",
                                                       "LogCreationClassName": "LMI_JournalMessageLog",
                                                       "LogName": "Journal",
                                                       "DataFormat": ""})
