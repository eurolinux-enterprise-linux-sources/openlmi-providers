OpenLMI LogicalFile configuration
=================================

Configuration is stored in ``/etc/openlmi/logicalfile/logicalfile.conf``.

Default configuration::

  [LMI_UnixDirectory]
  # Allow user to create directories. (default = True)
  AllowMkdir=True

  # Allow user to remove empty directories. (default = True)
  AllowRmdir=True

  [LMI_SymbolicLink]
  # Allow user to create symbolic links. (default = False)
  AllowSymlink=False

Options and their values are self-explanatory. The example configuration is
complete, meaning that no more sections or options are available.
