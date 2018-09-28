# Reset all previously set options
reset

# Set terminal up
set term pngcairo font "FreeSans, 10" size 1024, 768
set output 'GWs.png'

# Filename of the data
filename='GWs.dat'

# Plot the data
plot filename using 1:2 notitle
