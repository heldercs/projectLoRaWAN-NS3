# Reset all previously set options
reset

# Set terminal up
set term pngcairo font "FreeSans, 10" size 1024, 768
set grid
set xrange [100:3000]
set xtics font ",10"
set yrange [0:3]
set ytics font ",10"
set ylabel "Throughput (Kbps)" font "Times-Roman,16"
set xlabel "No. de EndDevices" font "times-Roman,16"
set output './data/throughput.png'

# Filename of the data
filename='./data/throughput.dat'


# Plot the data
plot filename using 1:2 w lp lw 2 t 't_{int}=1sec', filename using 1:3 w lp lw 2 t 't_{int}=2sec', filename using 1:4 w lp lw 2 t 't_{int}=5sec', filename using 1:5 w lp lw 2 t 't_{int}=10sec', filename using 1:6 w lp lw 2 t 't_{int}=20sec', filename using 1:7 w lp lw 2 t 't_{int}=30sec'
