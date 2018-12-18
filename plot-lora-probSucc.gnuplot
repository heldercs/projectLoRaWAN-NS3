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
filename='./TestResult/resulta-STAs.dat'

# Plot the data
plot filename using 1:2 w lp lw 2 t 'free space', filename using 1:3 w lp lw 2 t 'buildings'
