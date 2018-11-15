%: %.c
	gcc $@.c -o $@ -Wall -Werror -lm
	#/home/yukatan/Scripts/checkpatch.pl --no-tree -f $@.c
