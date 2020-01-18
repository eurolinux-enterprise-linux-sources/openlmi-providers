all: index.rst

WORKSPACE_PREFIX ?= ../clones

MOFS=$(WORKSPACE_PREFIX)/openlmi-providers/mof/05_LMI_Qualifiers.mof \
     $(WORKSPACE_PREFIX)/openlmi-providers/mof/30_LMI_Jobs.mof \
     $(WORKSPACE_PREFIX)/openlmi-providers/mof/50_LMI_LogicalFile.mof \
     $(WORKSPACE_PREFIX)/openlmi-providers/mof/60_LMI_Account.mof \
     $(WORKSPACE_PREFIX)/openlmi-providers/mof/60_LMI_Fan.mof \
     $(WORKSPACE_PREFIX)/openlmi-providers/mof/60_LMI_Hardware.mof \
     $(WORKSPACE_PREFIX)/openlmi-providers/mof/60_LMI_PCP.mof \
     $(WORKSPACE_PREFIX)/openlmi-providers/mof/60_LMI_PowerManagement.mof \
     $(WORKSPACE_PREFIX)/openlmi-providers/mof/60_LMI_Realmd.mof \
     $(WORKSPACE_PREFIX)/openlmi-providers/mof/60_LMI_Service.mof \
     $(WORKSPACE_PREFIX)/openlmi-providers/mof/60_LMI_Software.mof \
     $(WORKSPACE_PREFIX)/openlmi-networking/mof/60_LMI_Networking.mof \
     $(WORKSPACE_PREFIX)/openlmi-storage/mof/LMI_Storage-Base.mof \
     $(WORKSPACE_PREFIX)/openlmi-storage/mof/LMI_Storage-BlockStorage.mof \
     $(WORKSPACE_PREFIX)/openlmi-storage/mof/LMI_Storage-Filesystem.mof \
     $(WORKSPACE_PREFIX)/openlmi-storage/mof/LMI_Storage-Jobs.mof \
     $(WORKSPACE_PREFIX)/openlmi-storage/mof/LMI_Storage-Luks.mof \
     $(WORKSPACE_PREFIX)/openlmi-storage/mof/LMI_Storage-Mounting.mof \
     $(WORKSPACE_PREFIX)/openlmi-storage/mof/LMI_Storage-Partitions.mof

clean:
	rm -f *.rst

index.rst: $(MOFS)
	../clones/openlmi-providers/tools/openlmi-doc-class2rst -H header.txt \
		-M `echo $(MOFS) | sed 's/ / -M /g'` \
		-S /usr/share/mof/cim-current/CIM_Schema.mof \
		`cat $(MOFS) | grep "^class" | cut -d ' ' -f 2 | sed 's/://'`
