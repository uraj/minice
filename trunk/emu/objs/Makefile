CFLAGS=-c
CFLAGS+=-g
CFLAGS+=-Wall
CFLAGS+=-I..
MINIEMU_OBJS=fetch.o decode.o control.o execute.o memaccess.o writeback.o elfmanip.o vmem.o memory.o
miniemu:../miniemu
../miniemu:$(MINIEMU_OBJS) emulator.o debug.o
	gcc $(MINIEMU_OBJS) emulator.o debug.o -o ../miniemu
testemu:$(MINIEMU_OBJS) test.o
	gcc $(MINIEMU_OBJS) test.o -o testemu
fetch.o:../pipeline/fetch.c ../pipeline/fetch.h
	gcc ../pipeline/fetch.c $(CFLAGS) -o $@
control.o:../pipeline/control.c ../pipeline/control.h
	gcc ../pipeline/control.c $(CFLAGS) -o $@
decode.o:../pipeline/decode.c ../pipeline/decode.h
	gcc ../pipeline/decode.c $(CFLAGS) -o $@
execute.o:../pipeline/execute.c ../pipeline/execute.h
	gcc ../pipeline/execute.c $(CFLAGS) -o $@
memaccess.o:../pipeline/memaccess.c ../pipeline/memaccess.h
	gcc ../pipeline/memaccess.c $(CFLAGS) -o $@
writeback.o:../pipeline/writeback.c ../pipeline/writeback.h
	gcc ../pipeline/writeback.c $(CFLAGS) -o $@
elfmanip.o:../loader/elfmanip.c ../loader/elfmanip.h
	gcc ../loader/elfmanip.c $(CFLAGS) -o $@
vmem.o:../memory/vmem.c ../memory/vmem.h
	gcc ../memory/vmem.c $(CFLAGS) -o $@
memory.o:../memory/memory.c ../memory/memory.h
	gcc ../memory/memory.c $(CFLAGS) -o $@
emulator.o:../emulator.c
	gcc ../emulator.c $(CFLAGS) -o $@
debug.o:../debug.c
	gcc ../debug.c $(CFLAGS) -o $@
test.o:../test.c
	gcc ../test.c $(CFLAGS) -o $@