# commands/Makefile

#ENTRYPOINT	= 0x1000
HD		= ../80m.img

ASM		= nasm
DASM		= objdump
CC		= gcc
LD		= ld
ASMFLAGS	= -I ../include/ -f elf
CFLAGS		= -I ../include/ -c -fno-builtin -fno-stack-protector -Wall
LDFLAGS		= -Ttext 0x1000
DASMFLAGS	= -D
LIB		= ../lib/orangescrt.a
BIN		= print pwd

# All Phony Targets
.PHONY : everything final clean realclean disasm all install

# Default starting position
everything : $(BIN)

install : all clean
	cp ../kernel.bin ./ -v
	#tar vcf inst.tar kernel.bin $(BIN) print
	#dd if=inst.tar of=$(HD) seek=`print "obase=10;ibase=16;(\`egrep -e '^ROOT_BASE' ../boot/include/load.inc | sed -e 's/.*0x//g'\`+\`egrep -e '#define[[:space:]]*INSTALL_START_SECT' ../include/sys/config.h | sed -e 's/.*0x//g'\`)*200" | bc` bs=1 count=`ls -l inst.tar | awk -F " " '{print $$5}'` conv=notrunc

all : realclean everything

final : all clean

clean :
	rm -f *.o

realclean :
	rm -f $(BIN) print *.o

kernel.bin :
	cp ../kernel.bin ./

start.o : start.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

pwd.o: pwd.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

pwd : pwd.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

print.o : print.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

print : print.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

calc.o : calc.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

calc : calc.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

24.o : 24.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

24 : 24.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

box.o : box.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

box : box.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

laststep : 
	tar vcf inst.tar kernel.bin print calc 24 box
	dd if=inst.tar of=$(HD) seek=`echo "obase=10;ibase=16;(\`egrep -e '^ROOT_BASE' ../boot/include/load.inc | sed -e 's/.*0x//g'\`+\`egrep -e '#define[[:space:]]*INSTALL_START_SECT' ../include/sys/config.h | sed -e 's/.*0x//g'\`)*200" | bc` bs=1 count=`ls -l inst.tar | awk -F " " '{print $$5}'` conv=notrunc

