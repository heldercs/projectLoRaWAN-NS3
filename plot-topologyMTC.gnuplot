# Reset all previously set options
reset

# Set terminal up
set term pngcairo font "FreeSans, 10" size 1024, 768
set output outFile

set style line 3 lc rgb 'black' pt 9 ps 2

# Use a good looking palette
set palette defined (0.0 "#D53E4F",\
 0.5 "#D53E4F",\
 0.5001 "#F46D43",\
 1.5 "#F46D43",\
 1.5001 "#FDAE61",\
 2.5 "#FDAE61",\
 2.5001 "#FEE08B",\
 3.5 "#FEE08B",\
 3.5001 "#E6F598",\
 4.5 "#E6F598",\
 4.5001 "#ABDDA4",\
 5.0 "#ABDDA4")

set cblab 'Spreading Factor' font "freeSans,16"

# Set up style for buildings
set style rect fc lt -1 fs solid 0.15 noborder

#set ytics font ",10"
#set ylabel "SFs" font "freeSans,16" offset 3,0,0

# Filename of the data
#filename='endDevices.dat'

# load the building locations
#load 'buildings.dat'

# Plot the data
#plot filename_edN using 1:2:3 with points pt 7 palette t 'ED Normal', filename_edA using 1:2:3 with points pt 10 palette t 'ED Alarm', filename_gw using 1:2 with points ls 3 t 'Gateway'
plot filename_edN using 1:2:3 with points pt 7 palette t 'EDs', filename_gw using 1:2 with points ls 3 t 'Gateway'
