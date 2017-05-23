set term x11 persist
set title "Fan Demographs"
set ylabel "Number of people"
set logscale y
set style fill solid border
set autoscale y
set xtics ("Fans" 1, "Male" 2, "Female" 3, "Avg.Age(yrs)" 4)
set style data histogram
set style histogram clustered
plot for [COL=2:3] "data.dat" using COL:xticlabels(1)  title columnheader
