# Reset all previously set options
reset

# Set terminal up
set term pngcairo font "FreeSans, 10" size 1024, 768
set output outFile

set style line 1 lc rgb 'black' pt 9 ps 2

# Use a good looking palette
set palette defined ( 0 '#D53E4F',\
    1 '#F46D43',\
    2 '#FDAE61',\
    3 '#FEE08B',\
    4 '#E6F598',\
    5 '#ABDDA4',\
    6 '#66C2A5',\
    7 '#3288BD' )

# Set up style for buildings
set style rect fc lt -1 fs solid 0.15 noborder

# Filename of the data
#filename='endDevices.dat'

# load the building locations
load 'buildings.dat'

# Plot the data
plot filename using 1:2:3 with points pt 7 palette t 'Nodes', filename2 using 1:2 with points ls 1 t 'Gateway'
