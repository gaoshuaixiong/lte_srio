#ifconfig LTE_IPADP0 10.10.10.10
#sleep 2
iperf  -c 10.10.10.8  -u  -p 10008 -l 100 -b 500k

