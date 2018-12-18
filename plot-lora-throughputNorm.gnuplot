# Reset all previously set options
reset

# Set terminal up
set term pngcairo font "FreeSans, 10" size 1024, 768
set grid
set xrange [0:11.5]
set xtics font ",10"
set yrange [0:1]
set ytics font ",10"
set ylabel "S - Throughput" font "Times-Roman,16"
set xlabel "G - offered traffic" font "times-Roman,16"
set output './TestResult/throughputNorm.png'

# Filename of the data
filename='./TestResult/result-STAs.dat'


# Plot the data
plot filename using 6:7 w lp lw 2 t 'free space' 
