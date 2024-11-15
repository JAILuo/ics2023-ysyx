.DEFAULT_GOAL := dts

# For converting the .dtb into a .h file for embeddding.
bintoh :
	echo "#include <stdio.h>" > bintoh.c
	echo "int main(int argc,char ** argv) {if(argc==1) return -1; int c, p=0; printf( \"static const unsigned char %s[] = {\", argv[1] ); while( ( c = getchar() ) != EOF ) printf( \"0x%02x,%c\", c, (((p++)&15)==15)?10:' '); printf( \"};\" ); return 0; }" >> bintoh.c
	gcc bintoh.c -o bintoh

nemu.dtb : nemu.dts
	dtc -I dts -O dtb -o $@ $^ -S 1536                                                  

#nemudts.h : nemu.dtb bintoh
#	./bintoh nemudtb < $< > $@
	# WARNING: sixtyfourmb.dtb MUST hvave at least 16 bytes of buffer room AND be 16-byte aligned.
	#  dtc -I dts -O dtb -o sixtyfourmb.dtb sixtyfourmb.dts -S 1536

dts: nemu.dtb

