# Reset all previously set options
reset

# Set terminal up
#set term pngcairo font "FreeSans, 10" size 1024, 768
set term postscript eps color blacktext "Helvetica" 10
set grid
set xrange [100:2000]
set xtics font ",10"
set yrange [0:100]
set ytics font ",10"
set ylabel "ProbLoss (%)" font "Times-Roman,16"
set xlabel "No. de EndDevices" font "times-Roman,16"
set output './TestResult/probLoss.png'

# Filename of the data
filename1='./TestResult/result-STAs.dat'
filename2='./TestResult/result-STAs_7GW.dat'
filename3='./TestResult/result-regSTAs_7GW.dat'
filename4='./TestResult/result-almSTAs_7GW.dat'
filename5='./TestResult/result-regSTAs2_7GW.dat'
filename6='./TestResult/result-almSTAs2_7GW.dat'

# Plot the data
plot filename1 using 1:4 w lp lw 2 t 'Regular whitout Alarm 1GW', filename2 using 1:4 w lp lw 2 t 'Regular without Alarm', filename3 using 1:4 w lp lw 2 t 'Regular (sf7) with Alarm (sf7)', filename4 using 1:4 w lp lw 2 t 'Alarm (sf7)', filename5 using 1:4 w lp lw 2 t 'Regular (sf7) with Alarm (sf8)', filename6 using 1:4 w lp lw 2 t 'Alarm (sf8)'
