# Reset all previously set options
reset

# Set terminal up
set term pngcairo font "FreeSans, 10" size 1024, 768
set grid
set xrange [100:2000]
set xtics font ",10"
set yrange [0:100]
set ytics font ",10"
set ylabel "Prob. Success (%)" font "Times-Roman,16"
set xlabel "No. de EndDevices" font "times-Roman,16"
set output './TestResult/probSuccess.png'

# Filename of the data
filename1='./TestResult/result-STAs-1GW-FS.dat'
filename2='./TestResult/result-STAs-7GW-FS.dat'
filename3='./TestResult/result-STAs-1GW-SH.dat'
filename4='./TestResult/result-STAs-7GW-SH.dat'

# Plot the data
plot filename1 using 1:3 w lp lw 2 t '1 GW free space', filename2 using 1:3 w lp lw 2 t '7 GW free space', filename3 using 1:3 w lp lw 2 t '1 GW shadowing', filename4 using 1:3 w lp lw 2 t '7 GW shadowing'
