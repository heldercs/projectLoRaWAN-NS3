# Reset all previously set options
reset

# Set terminal up
#set term pngcairo font "FreeSans, 10" size 1024, 768
set term postscript eps color blacktext "Helvetica" 10
#set output outFile
set output 'alarm.eps'

set xrange [-500:500]
set yrange [-500:500]

set style line 1 lc rgb '#00008B' pt 7 ps 0.6
set style line 2 lc rgb 'red' pt 10 ps 0.6
set style line 3 lc rgb 'black' pt 9 ps 1.8

# Set up style for buildings
set style rect fc lt -1 fs solid 0.15 noborder

# Filename of the data
filename='endDevicesAlm.dat'
#filename2='GWs.dat'

# load the building locations
#load 'buildings.dat'

# Plot the data
plot filename using 1:2 with points ls 2 t 'Alarm'
#plot filename_edR using 1:2 with points ls 1 t 'Regular', filename_edA using 1:2 with points ls 2 t 'Alarm', filename_gw using 1:2 with points ls 3 t 'Gateway'
#plot filename_edA using 1:2 with points ls 1 t 'endDevices', filename_gw using 1:2 with points ls 3 t 'Gateway'
