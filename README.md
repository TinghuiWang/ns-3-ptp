# ns-3-ptp
IEEE 1588 Precision Time Protocol Simulation

This package implements the IEEE 1588 PTP protocol as part of the project required in WSU CptS 555.
To run the code, browse to `ns-3-dev/src` and clone the repository into `ptp` folder.

```bash
ns-3-dev/src $ git clone https://github.com/TinghuiWang/ns-3-ptp.git ptp
```

The package contains two examples that evaluates the performance of PTP in a wired ethernet network (802.3) and a Wifi AdHoc network (802.11b OFDM 2.4GHz).
To run the exmaples, you can use the following command.

```bash
ns-3-dev/src $ ./waf --run "ptp-csma --logdir=./ptp_test/csma"
ns-3-dev/src $ ./waf --run "ptp-wifi-adhoc --logdir=./ptp_test/wifi_adhoc"
```

Both examples support visualization (`--visualize`) and network animation with NetAnim 3.08.
