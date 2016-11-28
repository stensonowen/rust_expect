#!/usr/bin/gnuplot

set datafile separator ","
set xrange [1:10]
set xtics 1
set yrange [0:30]
set xlabel 'Test'
set ylabel 'Time (seconds)'
# set key right bottom
set title '__builtin_expect performance, n=10**10' 
set grid
plot 'data.csv' using 0:2 smooth bezier title 'Control' lw 2 lc rgb '#0000FF', \
     'data.csv' using 0:3 smooth bezier title 'Builtin' lw 2 lc rgb '#FF0000'
