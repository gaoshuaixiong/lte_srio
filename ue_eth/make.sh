#!/bin/bash
BASE_DIR=`pwd`
LTE_SRIO_DIR=$BASE_DIR/lte_virtual_srio_module
#LTE_MAC_DIR=$BASE_DIR/lte_mac_module
#LTE_RLC_DIR=$BASE_DIR/lte_rlc_module
LTE_RLC_MAC_DIR=$BASE_DIR/lte_rlc_mac_module
LTE_PDCP_DIR=$BASE_DIR/lte_pdcp_module
LTE_RRC_DIR=$BASE_DIR/lte_rrc_module
LTE_IPADP_DIR=$BASE_DIR/lte_ip_adp_module


cd $LTE_SRIO_DIR
make clean
make
#cp lte_virtual_srio_module.ko /var/lib/tftpboot/srio.ko

#cd $LTE_MAC_DIR
#make clean
#make
#cp lte_mac_module.ko /var/lib/tftpboot/mac.ko

cd $LTE_RLC_MAC_DIR
make clean
make
#cp lte_rlc_module.ko /var/lib/tftpboot/rlc.ko

cd $LTE_PDCP_DIR
make clean
make
#cp lte_pdcp_module.ko /var/lib/tftpboot/pdcp.ko

cd $LTE_RRC_DIR
make clean
make
#cp lte_rrc_module.ko /var/lib/tftpboot/rrc.ko

cd $LTE_IPADP_DIR
make clean
make
#cp lte_ipadp_module.ko /var/lib/tftpboot/ipadp.ko

echo "~~~Module making is complete.~~~"


