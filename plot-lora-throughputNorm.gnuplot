# Reset all previously set options
reset

# Set terminal up
set term pngcairo font "FreeSans, 10" size 1024, 768
set grid
set xrange [0:5]
set xtics font ",10"
set yrange [0:1.5]
set ytics font ",10"
set ylabel "S - Throughput" font "Times-Roman,16"
set xlabel "G - offered traffic" font "times-Roman,16"
set output './TestResult/throughputNorm.png'

# Filename of the data
filename1='./TestResult/result-regSTAs.dat'
filename2='./TestResult/result-almSTAs.dat'

# Plot the data
#plot filename1 using 6:7 w lp lw 2 t '1 GW free space', filename2 using 6:7 w lp lw 2 t '7 GWs free space', filename3 using 6:7 w lp lw 2 t '1 GW shadowing', filename4 using 6:7 w lp lw 2 t '7 GWs shadowing'
plot filename1 using 9:10 w lp lw 2 t 'Regular', filename2 using 9:10 w lp lw 2 t 'Alarm'

