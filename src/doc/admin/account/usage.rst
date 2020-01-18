OpenLMI Account usage
=====================

General manipulation of users and groups are done with the objects
from following classes:

* :ref:`LMI_AccountManagementService <LMI-AccountManagementService>`

* :ref:`LMI_Account <LMI-Account>`

* :ref:`LMI_Group <LMI-Group>`

* :ref:`LMI_MemberOfGroup <LMI-MemberOfGroup>`

* :ref:`LMI_Identity <LMI-Identity>`

* :ref:`LMI_AccountInstanceCreationIndication <LMI-AccountInstanceCreationIndication>`

* :ref:`LMI_AccountInstanceDeletionIndication <LMI-AccountInstanceDeletionIndication>`

Some common use cases are described in the following parts

List users
----------
List of users are provided by :ref:`LMI_Account <LMI-Account>`. Each one object
of this class represents one user on the system. Both system and non-sytem users
are directly in :ref:`LMI_Account <LMI-Account>` class::

    # List user by name
    print c.root.cimv2.LMI_Account.first_instance({"Name": "root"})
    # List user by id
    print c.root.cimv2.LMI_Account.first_instance({"UserID": "0"})


List groups
-----------
Similarly like users, groups are represented by objects
of :ref:`LMI_Group <LMI-Group>` class::

    # List group by name
    print c.root.cimv2.LMI_Group.first_instance({"Name": "root"})
    # List group by id
    print c.root.cimv2.LMI_Group.first_instance({"InstanceID": "LMI:GID:0"})


List group members
------------------
:ref:`LMI_Identity <LMI-Identity>` is class representing users and groups
on the system. Group membership is represented
by :ref:`LMI_MemberOfGroup <LMI-MemberOfGroup>` association.  It associates
:ref:`LMI_Group <LMI-Group>` and :ref:`LMI_Identity <LMI-Identity>`, where
:ref:`LMI_Identity <LMI-Identity>` is associated
by :ref:`LMI_AssignedAccountIdentity <LMI-AssignedAccountIdentity>` with
:ref:`LMI_Account <LMI-Account>`::

    # Get users from root group
    # 1) Get root group object
    root_group = c.root.cimv2.LMI_Group.first_instance({"Name": "root")
    # 2) Get LMI_Identity objects associated with root group
    identities = root_group.associators(ResultClass="LMI_Identity", AssocClass="LMI_MemberOfGroup")
    # 3) go through all identites, get LMI_Account associated with identity and print user name
    # Note: associators returns a list, but there is just one LMI_Account
    for i in identities:
        print i.first_associator(ResultClass="LMI_Account").Name

Create user
-----------
For user creation we have to use
:ref:`LMI_AccountManagementService <LMI-AccountManagementService>`. There is
:ref:`CreateAccount <LMI-AccountManagementService-CreateAccount>` method,
which will create user with descired attributes::

    # get computer system
    cs = c.root.cimv2.PG_ComputerSystem.first_instance()
    # get service
    lams = c.root.cimv2.LMI_AccountManagementService.first_instance()
    # invoke method, print result
    lams.CreateAccount(Name="lmishell-user", System=cs)

Create group
------------
Similarly like creating user, creating groups are don in
:ref:`LMI_AccountManagementService <LMI-AccountManagementService>`, using
:ref:`CreateGroup <LMI-AccountManagementService-CreateGroup>` method::

    # get computer system
    cs = c.root.cimv2.PG_ComputerSystem.first_instance()
    # get service
    lams = c.root.cimv2.LMI_AccountManagementService.first_instance()
    # invoke method, print result
    print lams.CreateGroup(Name="lmishell-group", System=cs)


Delete user
-----------
User deletion is done with :ref:`DeleteUser <LMI-Account-DeleteUser>`
method on the desired :ref:`LMI_Account <LMI-Account>` object.

::

    # get the desired user
    acci = c.root.cimv2.LMI_Account.first_instance({"Name": "tobedeleted"})
    # delete the user
    acci.DeleteUser()

.. note::

   Previous releases allowed to use ``DeleteInstance`` intrinsic method to
   delete ``LMI_Account``. This method is now deprecated and
   will be removed from future releases of OpenLMI Account. The reason is that
   ``DeleteInstance`` cannot have parameters; it is equivalent to call
   ``DeleteAccount`` without specifying parameters.


Delete group
------------
Group deletion is done with :ref:`DeleteGroup <LMI-Group-DeleteGroup>`
method on the desired :ref:`LMI_Group <LMI-Group>` object,

::

    # get the desired group
    grp = c.root.cimv2.LMI_Group.first_instance({"Name": "tobedeleted"})
    # delete the group
    grp.DeleteGroup()

.. note::

   Previous releases allowed to use ``DeleteInstance`` intrinsic method to
   delete ``LMI_Group``. This method is now deprecated and
   will be removed from future releases of OpenLMI Account. The reason is that
   we want to have consistent way to delete user and group.


Add user to group
-----------------
Adding user to group is done with ``CreateInstance`` intrinsic method on the
:ref:`LMI_MemberOfGroup <LMI-MemberOfGroup>` class, which requires reference
to :ref:`LMI_Group <LMI-Group>` and :ref:`LMI_Identity <LMI-Identity>`::

    # We will add root user to pegasus group
    # get group pegasus
    grp = c.root.cimv2.LMI_Group.first_instance({"Name": "pegasus"})
    # get user root
    acc = c.root.cimv2.LMI_Account.first_instance({"Name": "root"})
    # get identity of root user
    identity = acc.first_associator(ResultClass="LMI_Identity")
    # create instance of LMI_MemberOfGroup with the above references
    c.root.cimv2.LMI_MemberOfGroup.create_instance({"Member":identity.path, "Collection":grp.path})

Remove user from group
----------------------
Removing user from group is done with ``DeleteInstance`` intrinsic method
on the desired :ref:`LMI_MemberOfGroup <LMI-MemberOfGroup>` object::

    # We will remove root user from pegasus group
    # get group pegasus
    grp = c.root.cimv2.LMI_Group.first_instance({"Name": "pegasus"})
    # get user root
    acc = c.root.cimv2.LMI_Account.first_instance({"Name": "root"})
    # get identity of root user
    identity = acc.associators(ResultClass="LMI_Identity")[0]
    # iterate through all LMI_MemberOfGroup associated with identity and remove the one with our group
    for mog in identity.references(ResultClass="LMI_MemberOfGroup"):
        if mog.Collection == grp.path:
            mog.delete()

Modify user
-----------
It is also possible to modify user details and it is done by ``ModifyInstance``
intrinsic method on the desired :ref:`LMI_Account <LMI-Account>` object::

    # Change login shell of test user
    acci = c.root.cimv2.LMI_Account.first_instance({"Name": "test"})
    acci.LoginShell = '/bin/sh'
    # propagate changes
    acci.push()

Indications
-----------
OpenLMI Account supports the following indications:

* :ref:`LMI_AccountInstanceCreationIndication <LMI-AccountInstanceCreationIndication>`

* :ref:`LMI_AccountInstanceDeletionIndication <LMI-AccountInstanceDeletionIndication>`

Both indications works only on the following classes:

* :ref:`LMI_Account <LMI-Account>`

* :ref:`LMI_Group <LMI-Group>`

* :ref:`LMI_Identity <LMI-Identity>`

See more below.

Creation Indication
^^^^^^^^^^^^^^^^^^^
Client can be notified when instance of class has been created. It is done with
:ref:`LMI_AccountInstanceCreationIndication <LMI-AccountInstanceCreationIndication>`. The indication filter query must be in the following form:
``SELECT * FROM LMI_AccountInstanceCreationIndication WHERE SOURCEINSTANCE ISA class_name``, where ``class_name`` is one of the allowed classes.

The following example creates filter, handler and subscription (lmi shell do it in one step), which will notify client when user is created:

::

    # Notify when a user is created
    c.subscribe_indication(
        FilterCreationClassName="CIM_IndicationFilter",
        FilterSystemCreationClassName="CIM_ComputerSystem",
        FilterSourceNamespace="root/cimv2",
        QueryLanguage="DMTF:CQL",
        Query='SELECT * FROM LMI_AccountInstanceCreationIndication WHERE SOURCEINSTANCE ISA LMI_Account',
        Name="account_creation",
        CreationNamespace="root/interop",
        SubscriptionCreationClassName="CIM_IndicationSubscription",
        HandlerCreationClassName="CIM_IndicationHandlerCIMXML",
        HandlerSystemCreationClassName="CIM_ComputerSystem",
        Destination="http://192.168.122.1:5988" # this is the destination computer, where all the indications will be delivered
    )


Deletion Indication
^^^^^^^^^^^^^^^^^^^
Client can be notified when instance is deleted. The same rules like in `Creation Indication`_ applies here:

::

    # Notify when a user is deleted
    c.subscribe_indication(
        FilterCreationClassName="CIM_IndicationFilter",
        FilterSystemCreationClassName="CIM_ComputerSystem",
        FilterSourceNamespace="root/cimv2",
        QueryLanguage="DMTF:CQL",
        Query='SELECT * FROM LMI_AccountInstanceDeletionIndication WHERE SOURCEINSTANCE ISA LMI_Account',
        Name="account_deletion",
        CreationNamespace="root/interop",
        SubscriptionCreationClassName="CIM_IndicationSubscription",
        HandlerCreationClassName="CIM_IndicationHandlerCIMXML",
        HandlerSystemCreationClassName="CIM_ComputerSystem",
        Destination="http://192.168.122.1:5988" # this is the destination computer, where all the indications will be delivered
    )

.. note::
   Both indications uses indication manager and polling.
