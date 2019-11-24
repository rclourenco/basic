basic: basic.c basic.h basrpn.c basexec.c
	gcc -o basic basic.c basrpn.c basexec.c

basicd: basic.c basic.h basrpn.c basexec.c graphics.h graphics.c
	tcc basic.c basrpn.c basexec.c graphics.c
