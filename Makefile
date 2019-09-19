p2.exe : project2.c student2.c
	gcc -g project2.c student2.c -o p2
clean :
	rm -f p2.exe