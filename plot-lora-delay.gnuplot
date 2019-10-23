# Reset all previously set options
reset

set terminal png size 800,500 enhanced font "Helvetica,20"
set output './TestResult/delay_IIP_Alarms.png'

#set term postscript eps color blacktext "Times-Roman-Bold" 20
#set output './TestResult/delay_IIP_Alarms.eps'

red = "#FF0000"; green = "#00FF00"; blue = "#0000FF"; skyblue = "#87CEEB";
set yrange [0:300]
set style data histogram
set style histogram cluster gap 1
set style fill solid
set boxwidth 0.9
set ylabel "Average delay (Sec)" offset 1 font "Times-Roman-Bold,24"
set xlabel "end-nodes" font "Times-Roman-Bold,24"
set xtics format ''
set grid ytics

set key left top
set key reverse vertical Left

# Filename of the data
filename1='TestResult/result-IIP-Alm-bi-rtx.dat'
filename2='TestResult/result-IIP-Alm-bii-rtx.dat'
filename3='TestResult/result-IIP-Alm-biii-rtx.dat'

# set title "A Sample Bar Chart"
plot filename1 using 4:xtic(1) title "Alarm: b-i" linecolor rgb red, \
            filename2 using 4 title "Alarm: b-ii" linecolor rgb blue, \
            filename3 using 4 title "Alarm: b-iii" linecolor rgb green
