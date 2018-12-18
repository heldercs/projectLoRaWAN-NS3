#!/bin/bash

if [ $# -ne 7 ]
then
echo "parameters missing"
exit 1
fi

gwRing=$1
rad=$2
gwRad=$3
simTime=$4
interval=$5
trial=$6
pEDs=$7


echo "##### Simulation Start #####"

if [ ! -d TestResult/ ]
	then
	mkdir TestResult/
fi

#for trial in 0 1 2
#do

#if [ $trial -eq 0 ]
#then
#	gwRing=1
#	gwRad=0
#elif [ $trial -eq 1 ]
#then
#	gwRing=2
#	gwRad=0
#else
#	gwRing=2
#	gwRad=4000
#fi  


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
echo "#numSta, Throughput(Kbps), ProbSucc(%), ProbLoss(%), avgDelay(nanoSeconds), G(offered traffic), S(throughput)" > ./TestResult/test$trial/traffic-$interval/result-STAs.dat 
		
touch ./TestResult/test$trial/delay.dat
file3="./TestResult/test$trial/delay.dat"
echo "number STA: delay (in nanoseconds) " >> ./TestResult/test$trial/delay.dat

for numSta in 100 200 300 400 500 600 700 800 900 1000 1100 1200 1300 1400 1500 1600 1700 1800 1900 2000
do
		echo "trial:$trial-numSTA:$numSta"

		if [ ! -d TestResult/test$trial/traffic-$interval/pcap-sta-$numSta/ ]
		then
			mkdir TestResult/test$trial/traffic-$interval/pcap-sta-$numSta/
		fi

		touch TestResult/test$trial/time-record$numSta.txt

		echo "Time: $(date) $interval $numSta" >> TestResult/test$trial/time-record$numSta.txt


  	./waf --run "lorawan-network-sim --nDevices=$numSta --gatewayRings=$gwRing --radius=$rad -gatewayRadius=$gwRad --simulationTime=$simTime --appPeriod=$interval --printEDs=$pEDs --file1=$file1 --file2=$file2 --file3=$file3 --trial=$trial"  > ./TestResult/test$trial/traffic-$interval/pcap-sta-$numSta/record-$numSta.txt 2>&1

done
#done
echo "##### Simulation finish #####"
echo "seinding email..."
echo simulation finish | mail -s Simulator helderhdw@gmail.com


