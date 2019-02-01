# Reset all previously set options
reset

# Set terminal up
set term pngcairo font "FreeSans, 10" size 1024, 768
set grid
set xrange [100:2000]
set xtics font ",10"
set yrange [0:100]
set ytics font ",10"
set ylabel "ProbLoss (%)" font "Times-Roman,16"
set xlabel "No. de EndDevices" font "times-Roman,16"
set output './TestResult/probLoss.png'

# Filename of the data
filename1='./TestResult/result-STAs-nRetx.dat'
filename2='./TestResult/result-STAs-Retx.dat'

# Plot the data
plot filename1 using 1:4 w lp lw 2 t 'without retx', filename2 using 1:4 w lp lw 2 t 'with retx'
