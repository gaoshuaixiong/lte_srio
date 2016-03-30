#!/bin/bash
ifconfig LTE_IPADP0 down
ifconfig LTE_RRC0 down
sleep 2
ifconfig LTE_PDCP0 down
sleep 2
ifconfig LTE_RLC_MAC0 down
sleep 2
#ifconfig LTE_RLC0 down
#sleep 2
#ifconfig LTE_MAC0 down
#sleep 2
ifconfig LTE_SRIO0 down
sleep 2

rmmod lte_ipadp_module
sleep 2
rmmod lte_rrc_module
sleep 2
rmmod lte_pdcp_module
sleep 2
rmmod lte_rlc_mac_module
sleep 2
#rmmod lte_mac_module
#sleep 2
rmmod lte_virtual_srio_module

echo "~~~Modules are removed~~~"
