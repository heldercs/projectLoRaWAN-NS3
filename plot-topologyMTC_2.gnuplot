# Reset all previously set options
reset

# Set terminal up
set term pngcairo font "FreeSans, 10" size 1024, 768
set output outFile

set style line 1 lc rgb '#00008B' pt 7
set style line 2 lc rgb 'red' pt 10
set style line 3 lc rgb 'black' pt 9 ps 3

# Set up style for buildings
set style rect fc lt -1 fs solid 0.15 noborder

# Filename of the data
#filename='endDevices.dat'

# load the building locations
#load 'buildings.dat'

# Plot the data
plot filename_edR using 1:2 with points ls 1 t 'ED Regular', filename_edA using 1:2 with points ls 2 t 'ED Alarm', filename_gw using 1:2 with points ls 3 t 'Gateway'
