# Reset all previously set options
reset

# Set terminal up
#set term pngcairo font "FreeSans, 10" size 1024, 768
set term postscript eps color blacktext "FreeSans-Bold" 10
set grid
set key box lt -1 lw 2
set xrange [0:2400]
set xtics font ",10"
set yrange [86:99]
set ytics font ",10"
set ylabel "Regular Success Prob. (%)" font "Times-Roman-Bold,16"
set xlabel "number of alarms" font "times-Roman-Bold,16"
set output './TestResult/probSuccess_regular_smartMeter_1GW.eps'
#set key bottom
#set encoding utf8

set multiplot

# Filename of the data
filename='result-regSTAs_aSF7.dat'
filename1='result-regSTAs_aSF8.dat'
filename2='result-regSTAs_aSF9.dat'
#filename3='TestResult/result-almSmartMeter_1GW.dat'
#filename4='TestResult/result-almSTAs_7GW.dat'


# Plot the data
#plot filename1 using 1:3 w lp lw 2 t 'Regular no cenário 2', filename2 using 1:3 w lp lw 2 t 'Regular no cenário 3', filename3 using 1:3 w lp lw 2 t 'Alarme no cenário 2', filename4 using 1:3 w lp lw 2 t 'Alarme no cenário 3'
plot filename using 1:3 w lp lt 7 lw 2 t 'Alarm SF-7', filename1 using 1:3 w lp lw 2 t 'Alarm SF-8', filename2 using 1:3 w lp lw 2 t 'Alarm SF-9'


#reset 
#set grid
#set origin 0.425,0.4
#set size 0.4,0.45
#set title 'zoom'
#set xlabel "% of Regulars"
#set ylabel ""
#set xrange [150:200]
#set yrange [95.2:96.2]

#plot filename1 using 1:3 w lp lt 7 lw 4 dt 2 notitle, filename2 using 1:3 w lp lw 4 dt 4 notitle'
