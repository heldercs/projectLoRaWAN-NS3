set terminal png
set output "plot-2d.png"
set title "2-D Plot"
set xlabel "X Values"
set ylabel "Y Quadratic Values"

set xrange [-6:+6]
plot "-"  title "2-D Data1" with linespoints, "-"  title "2-D Data2" with linespoints
-5 25
-4 16
-3 9
-2 4
-1 1
0 0
1 1
2 4
3 9
4 16
5 25
e
-5 15
-4 8
-3 3
-2 0
-1 -1
0 0
1 3
2 8
3 15
4 24
5 35
e
