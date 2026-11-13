10 dim t(3,10)
20 for i = 0 to 9
30 t(0,i) = i + 1
40 t(1,i) = 8
50 t(2,i) = t(0,i)*t(1,i)
60 next
70 for i = 0 to 9
80 print t(0,i); " * "; t(1, i);
90 print " = "; t(2,i)
100 next
