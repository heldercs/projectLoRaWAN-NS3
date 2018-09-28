#!/bin/bash

if [ $# -ne 6 ]
then
echo "parameters missing"
exit 1
fi

gwRing=$1
rad=$2
gwRad=$3
simTime=$4
interval=$5
pEDs=$6


echo "##### Simulation Start #####"

if [ ! -d TestResult/ ]
	then
	mkdir TestResult/
fi

trial=0;
#for trial in 1 2 3
#do
	if [ ! -d TestResult/test$trial/ ]
		then
		mkdir TestResult/test$trial/
	fi

	if [ ! -d TestResult/test$trial/traffic-$interval/ ]
		then
		mkdir TestResult/test$trial/traffic-$interval/
	fi

	touch ./TestResult/test$trial/traffic-$interval/mac-STAs-GW-$gwRing.txt
	file1="./TestResult/test$trial/traffic-$interval/mac-STAs-GW-$gwRing.txt"

	touch ./TestResult/test$trial/traffic-$interval/result-STAs.dat
	file2="./TestResult/test$trial/traffic-$interval/result-STAs.dat"
	echo "#numSta    Throughput(Kbps)    ProbSucc(%)    ProbLoss(%)" > ./TestResult/test$trial/traffic-$interval/result-STAs.dat 

	for numSta in 100 200 300 400 500 600 700 800 900 1000 1100 1200 1300 1400 1500 1600 1700 1800 1900 2000
	do
			echo "trial:$trial-numSTA:$numSta"

			if [ ! -d TestResult/test$trial/traffic-$interval/pcap-sta-$numSta/ ]
			then
				mkdir TestResult/test$trial/traffic-$interval/pcap-sta-$numSta/
			fi

			touch TestResult/test$trial/time-record$numSta.txt

			echo "Time: $(date) $interval $numSta" >> TestResult/test$trial/time-record$numSta.txt

	  	./waf --run "lorawan-network-sim --nDevices=$numSta --gatewayRings=$gwRing --radius=$rad -gatewayRadius=$gwRad --simulationTime=$simTime --appPeriod=$interval --printEDs=$pEDs --file1=$file1 --file2=$file2 --trial=$trial"  > ./TestResult/test$trial/traffic-$interval/pcap-sta-$numSta/record-$numSta.txt 2>&1

	done
#done
echo "##### Simulation finish #####"


