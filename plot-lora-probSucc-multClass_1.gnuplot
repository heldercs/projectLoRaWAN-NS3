# Reset all previously set options
reset

# Set terminal up
#set term pngcairo font "FreeSans, 10" size 1024, 768
set term postscript eps color blacktext "FreeSans-Bold" 10
set grid
set key box lt -1 lw 2
set xrange [200:2000]
set xtics 500 font ",10"
set yrange [0:1]
set ytics font ",10"
set ylabel "Prob. Success (%)" font "Times-Roman-Bold,16"
set xlabel "End-Nodes" font "times-Roman-Bold,16"
set output './TestResult/probSuccess_multClass_1.eps'
#set key bottom
set key reverse vertical right

#set multiplot

# Filename of the data
filename='./TestResult/result-multClass_1-SF7.dat'
filename1='./TestResult/result-multClass_1-SF8.dat'
filename2='./TestResult/result-multClass_1_2-SF7.dat'
filename3='./TestResult/result-multClass_1_2-SF8.dat'
#filename4='./TestResult/result-multClass_2_2-SF8.dat'
#filename5='./TestResult/result-multClass_2_2-SF9.dat'
#filename6='~/Doutorado/projectLoRa/result_OpenField_2GW/test36/traffic-600/result-regSTAs.dat'
#filename7='~/Doutorado/projectLoRa/result_OpenField_2GW/test37/traffic-600/result-regSTAs.dat'
#filename8='~/Doutorado/projectLoRa/result_OpenField_2GW/test38/traffic-600/result-regSTAs.dat'
#filename9='~/Doutorado/projectLoRa/result_OpenField_2GW/test39/traffic-600/result-regSTAs.dat'

# Plot the data
#plot filename using 1:3 w lp lw 4 t 'SF = 7', filename1 using 1:3 w lp lw 4 t 'SF = 8', filename2 using 1:3 w lp lw 4 t 'SF = 9'
#plot filename using 1:3 w lp lw 4 t 'SF = 7', filename1 using 1:3 w lp lw 4 t 'SF = 8'
plot filename using 1:3 w lp lw 4 t 'SF = 7', filename1 using 1:3 w lp lw 4 t 'SF = 8', filename2 using 1:3 w lp lw 4 t 'SF = 7_2', filename3 using 1:3 w lp lw 4 t 'SF = 8_2'





#reset 
#set grid
#set origin 0.425,0.4
#set size 0.4,0.45
#set title 'zoom'
#set xlabel "End-Nodes"
#set ylabel ""
#set xrange [1150:1200]
#set yrange [98.4:98.9]

#plot filename using 1:3 w lp lw 4 notitle, filename1 using 1:3 w lp lw 4 notitle, filename2 using 1:3 w lp lw 4 notitle, filename3 using 1:3 w lp lw 4 notitle, filename4 using 1:3 w lp lw 4 notitle, filename5 using 1:3 w lp lw 4 notitle, filename6 using 1:3 w lp lw 4 notitle, filename7 using 1:3 w lp lw 4 notitle, filename8 using 1:3 w lp lw 4 notitle, filename9 using 1:3 w lp lw 4 notitle
