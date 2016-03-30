#!/bin/bash
BASE_DIR=`pwd`
LTE_SRIO_DIR=$BASE_DIR/lte_virtual_srio_module
#LTE_MAC_DIR=$BASE_DIR/lte_mac_module
#LTE_RLC_DIR=$BASE_DIR/lte_rlc_module
LTE_RLC_MAC_DIR=$BASE_DIR/lte_rlc_mac_module
LTE_PDCP_DIR=$BASE_DIR/lte_pdcp_module
LTE_RRC_DIR=$BASE_DIR/lte_rrc_enb_module
LTE_IPADP_DIR=$BASE_DIR/lte_ip_adp_module

cd $LTE_SRIO_DIR
insmod lte_virtual_srio_module.ko
#echo "insmod srio done"

#cd $LTE_MAC_DIR
#insmod lte_mac_module.ko

cd $LTE_RLC_MAC_DIR
insmod lte_rlc_mac_module.ko
#echo "insmod rlc_mac done"

cd $LTE_PDCP_DIR
insmod lte_pdcp_module.ko
#echo "insmod pdcp done"

cd $LTE_RRC_DIR
insmod lte_rrc_module.ko
#echo "insmod rrc done"

cd $LTE_IPADP_DIR
insmod lte_ipadp_module.ko
#echo "insmod ipadp done"

ifconfig LTE_SRIO0 up
sleep 1
#ifconfig LTE_MAC0 up
#sleep 1
ifconfig LTE_RLC_MAC0 up
sleep 1
ifconfig LTE_PDCP0 up
sleep 1
ifconfig LTE_RRC0 up
sleep 1
ifconfig LTE_IPADP0 up
sleep 1
ifconfig LTE_IPADP0 10.10.10.10

echo "~~~Modules are working.~~~"
