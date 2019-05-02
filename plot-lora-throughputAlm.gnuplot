# Reset all previously set options
reset

# Set terminal up
set term pngcairo font "FreeSans, 10" size 1024, 768
set grid
set xrange [5:100]
set xtics font ",10"
set yrange [0:0.005]
set ytics font ",10"
set ylabel "Throughput (Kbps)" font "Times-Roman,16"
set xlabel "No. of Alarms endDevices" font "times-Roman,16"
set output './TestResult/throughputAlm.png'

# Filename of the data
filename='./TestResult/result-almSTAs.dat'

# Plot the data
plot filename using 1:2 w lp lw 2 t 'Alarm'
