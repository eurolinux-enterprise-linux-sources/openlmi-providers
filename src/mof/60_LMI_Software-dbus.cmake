[Version("0.4.3"), Association, Description(
    "This association ties a SoftwareIdentityFileCheck to a UnixFile from"
    " LogicalFile provider. One UnixFile object can have associated multiple"
    " SoftwareIdentityFileCheck objects as some files can be owned by multiple"
    " packages.")]
class LMI_SoftwareIdentityCheckUnixFile {

  [Key, Description(
      "The SoftwareIdentityFileCheck of the file.")]
  LMI_SoftwareIdentityFileCheck REF Check;

  [Key, Description("The UnixFile representation of the file.")]
  LMI_UnixFile REF File;

};
