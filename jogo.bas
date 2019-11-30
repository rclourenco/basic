10 cls
20 for y=0 to 24
30 for x=0 to 39
31  color x+32+y/2, rnd(255)
40  locate x,y
41  a=x*y%40+33
42  b=rnd(26) + 65
43  print chr(b);
45  if y=24 then
46    if x=37 then
47     goto 70
48    end
49 end
50 next
60 next
70 locate 0,0
80 print "Hello, World"
