CFLAGS=-c
CFLAGS+=-g
CFLAGS+=-Wall
CFLAGS+=-I..
EMU_OBJS=fetch.o decode.o control.o execute.o memaccess.o writeback.o elfmanip.o vmem.o memory.o
EMU_OBJS_DB=fetch.db decode.db control.db execute.db memaccess.db writeback.db elfmanip.db vmem.db memory.db

emu:../emu
../emu:$(EMU_OBJS) emulator.o debug.o
	gcc $(EMU_OBJS) emulator.o debug.o -o ../emu
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

emu-db:../emu-db
../emu-db:$(EMU_OBJS_DB) emulator.db debug.db
	gcc -DDEBUG $(EMU_OBJS_DB) emulator.db debug.db -o ../emu-db
fetch.db:../pipeline/fetch.c ../pipeline/fetch.h
	gcc -DDEBUG ../pipeline/fetch.c $(CFLAGS) -o $@
control.db:../pipeline/control.c ../pipeline/control.h
	gcc -DDEBUG ../pipeline/control.c $(CFLAGS) -o $@
decode.db:../pipeline/decode.c ../pipeline/decode.h
	gcc -DDEBUG ../pipeline/decode.c $(CFLAGS) -o $@
execute.db:../pipeline/execute.c ../pipeline/execute.h
	gcc -DDEBUG ../pipeline/execute.c $(CFLAGS) -o $@
memaccess.db:../pipeline/memaccess.c ../pipeline/memaccess.h
	gcc -DDEBUG ../pipeline/memaccess.c $(CFLAGS) -o $@
writeback.db:../pipeline/writeback.c ../pipeline/writeback.h
	gcc -DDEBUG ../pipeline/writeback.c $(CFLAGS) -o $@
elfmanip.db:../loader/elfmanip.c ../loader/elfmanip.h
	gcc -DDEBUG ../loader/elfmanip.c $(CFLAGS) -o $@
vmem.db:../memory/vmem.c ../memory/vmem.h
	gcc -DDEBUG ../memory/vmem.c $(CFLAGS) -o $@
memory.db:../memory/memory.c ../memory/memory.h
	gcc -DDEBUG ../memory/memory.c $(CFLAGS) -o $@
emulator.db:../emulator.c
	gcc -DDEBUG ../emulator.c $(CFLAGS) -o $@
debug.db:../debug.c
	gcc -DDEBUG ../debug.c $(CFLAGS) -o $@
test.db:../test.c
	gcc -DDEBUG ../test.c $(CFLAGS) -o $@