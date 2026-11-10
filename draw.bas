10 c=rnd(30)+30
20 color c,c+5
30 x=rnd(300)
40 y=rnd(180)
50 dx=rnd(19)+1
60 dy=rnd(19)+1
70 box x,y,x+dx,y+dy
80 goto 10
120 for i = 0 to 199
130 color i, 1
140 line 0,i,319,i
150 next
