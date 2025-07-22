#!/bin/bash

OAI_CONFIG_DIR="/home/grad/boeira/openairinterface5g/"

# x310
OAI_CONFIG_FILE="ue.conf"

# x410
# OAI_CONFIG_FILE="gnb.band78.sa.fr1.106PRB.usrpx400.conf"

# b210 not tested in a while, may not work
# OAI_CONFIG_FILE="gnb.sa.band78.fr1.106PRB.usrpb210.conf"

rm -rf /tmp/dapps 

source oaienv
cd ./cmake_targets/ran_build/build

umask 0000

rm statsMAC.log
rm statsPRB.log

# x310 and x410
# gdb --args
#taskset -ca 0-45 ./nr-softmodem -O ${OAI_CONFIG_DIR}${OAI_CONFIG_FILE} --gNBs.[0].min_rxtxtime 6 --sa --usrp-tx-thread-config 1  -E --T_stdout 2 --gNBs.[0].do_SRS 0
#./nr-uesoftmodem -O ${OAI_CONFIG_DIR}${OAI_CONFIG_FILE}  -E -r 106 --numerology 1 -C 3619200000  --rfsim --rfsimulator.serveraddr 129.173.67.141 # --log_config.global_log_level debug
./nr-uesoftmodem -O ${OAI_CONFIG_DIR}${OAI_CONFIG_FILE}  -E -r 106 --numerology 1 -C 3619200000  --rfsim --rfsimulator.serveraddr 0.0.0.0 # --log_config.global_log_level debug

# b210
# ./nr-softmodem -O ${OAI_CONFIG_DIR}${OAI_CONFIG_FILE} --gNBs.[0].min_rxtxtime 6 --sa --usrp-tx-thread-config 1 -E  --T_stdout  2

cd -
