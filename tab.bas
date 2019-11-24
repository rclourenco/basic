
10  print "****************************************"
100 x = 0
110 input "Qual a tabuada: ", x

120 print "Tabuada Do "; x; ":"
130 for j=1 to 10
140	print  x; "x"; j;"="; x*j 
150 next
155 print "****************************************"
160 input "Deseja continuar: ", y

170 if y then
180	goto 110
190 end

1000 print "The End!"
