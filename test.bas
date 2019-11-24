100 x = 0
110 input "Qual a tabuada: ", x

120 print "Tabuada Do "; x; ":"
130 for j=1 to 10
140	print  x; "x"; j;"="; x*j 
150 next

160 input "Deseja continuar: ", y

if y = 5 then
  goto 1000
end
if y = 4 then
	goto 900
end

if y then
	goto 110
else
	' isto e um comentario
	if x=0 then
		print "Eq", x, "= 0"
	else
		if x > 0 then
			print "Sup", x, "> 0"
		else
			print "Inf", x, "< 0"
		end
	end

	' isto e um comentario

	for i = 0 to 10 step .90
		print "hello"; i
	next

	for i = 1 to 10
		print "Tabuada Do  "; i; ":"
		for j=1 to 10
			print  i; "x"; j;"="; i*j 
		next
		' print a+cos(x)*12
	next
end
goto 1000

900 x = 0
910 print x, "Ahahaha"
920 x = x + 1
990 if x < 10 then
991	goto 910
995 end

1000 print "The End!"
