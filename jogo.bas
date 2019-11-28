10 cls
20 for x=0 to 39
30 for y=0 to 23
31  color x+20
40  locate x,y
41  a=x*y%40+33
42  print chr(a)
50 next
60 next
