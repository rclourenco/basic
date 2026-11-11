1 cls
2 overscan 0
5 b = rnd(320)
6 c = rnd(320)
7 d = rnd(320)
8 vb = rnd(4) + 1
9 vc = rnd(4) + 1
10 vd = rnd(4) + 1
11 by = 20
12 cy = 60
13 dy = 100
19 image "scene.bmp"
20 sprload 0, "cat.bmp"
21 sprload 1, "cat.bmp"
22 sprload 2, "cat.bmp"
30 sprmove 0, b, by
31 sprmove 1, c, cy
32 sprmove 2, d, dy
40 sprshow 0
41 sprshow 1
42 sprshow 2
50 for a = 0 to 2000
60 next
61 sprhide 2
62 sprhide 1
63 sprhide 0
70 sprmove 0, b, by
71 sprmove 1, c, cy
72 sprmove 2, d, dy
73 sprshow 0
74 sprshow 1
75 sprshow 2
80 b = b + vb
81 c = c + vc
82 d = d + vd
91 if b > 320 then
92    b = 0-32
93    by = by + 2
94 end
95 if by > 200 then
96   by = 0 - 38
97   vb = rnd(4) + 1
98 end
101 if c > 320 then
102   c = 0-32
103   cy = cy + 2
104 end
105 if cy > 200 then
106    cy = 0 - 38 
107    vc = rnd(4) + 1
108 end
111 if d > 320 then
112   d = 0-32
113   dy = dy + 2
114 end
115 if dy > 200 then
116   dy = 0 - 38 
117   vd = rnd(4) + 1
118 end
200 goto 50
