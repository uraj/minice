CFLAGS=-c
CFLAGS+=-Wall
CFLAGS+=-I..
EMU_OBJS=globdefs.o fetch.o decode.o control.o execute.o memaccess.o writeback.o elfmanip.o vmem.o memory.o
EMU_OBJS_DB=globdefs.db fetch.db decode.db control.db execute.db memaccess.db writeback.db elfmanip.db vmem.db memory.db

emu:../emu
../emu:$(EMU_OBJS) emulator.o debug.o
	gcc $(EMU_OBJS) emulator.o debug.o -o ../emu
globdefs.o:../pipeline/globdefs.h
	gcc ../pipeline/globdefs.c $(CFLAGS) -o $@
fetch.o:../pipeline/fetch.c ../pipeline/fetch.h ../pipeline/globdefs.h
	gcc ../pipeline/fetch.c $(CFLAGS) -o $@
control.o:../pipeline/control.c ../pipeline/control.h ../pipeline/globdefs.h
	gcc ../pipeline/control.c $(CFLAGS) -o $@
decode.o:../pipeline/decode.c ../pipeline/decode.h ../pipeline/globdefs.h
	gcc ../pipeline/decode.c $(CFLAGS) -o $@
execute.o:../pipeline/execute.c ../pipeline/execute.h ../pipeline/globdefs.h
	gcc ../pipeline/execute.c $(CFLAGS) -o $@
memaccess.o:../pipeline/memaccess.c ../pipeline/memaccess.h ../pipeline/globdefs.h
	gcc ../pipeline/memaccess.c $(CFLAGS) -o $@
writeback.o:../pipeline/writeback.c ../pipeline/writeback.h ../pipeline/globdefs.h
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

emu-db:../emu-db
../emu-db:$(EMU_OBJS_DB) emulator.db debug.db
	gcc -DDEBUG $(EMU_OBJS_DB) emulator.db debug.db -o ../emu-db
globdefs.db:../pipeline/globdefs.h
	gcc -DDEBUG ../pipeline/globdefs.c $(CFLAGS) -g -o $@
fetch.db:../pipeline/fetch.c ../pipeline/fetch.h ../pipeline/globdefs.h
	gcc -DDEBUG ../pipeline/fetch.c $(CFLAGS) -g -o $@
control.db:../pipeline/control.c ../pipeline/control.h ../pipeline/globdefs.h
	gcc -DDEBUG ../pipeline/control.c $(CFLAGS) -g -o $@
decode.db:../pipeline/decode.c ../pipeline/decode.h ../pipeline/globdefs.h
	gcc -DDEBUG ../pipeline/decode.c $(CFLAGS) -g -o $@
execute.db:../pipeline/execute.c ../pipeline/execute.h ../pipeline/globdefs.h
	gcc -DDEBUG ../pipeline/execute.c $(CFLAGS) -g -o $@
memaccess.db:../pipeline/memaccess.c ../pipeline/memaccess.h ../pipeline/globdefs.h
	gcc -DDEBUG ../pipeline/memaccess.c $(CFLAGS) -g -o $@
writeback.db:../pipeline/writeback.c ../pipeline/writeback.h ../pipeline/globdefs.h
	gcc -DDEBUG ../pipeline/writeback.c $(CFLAGS) -g -o $@
elfmanip.db:../loader/elfmanip.c ../loader/elfmanip.h
	gcc -DDEBUG ../loader/elfmanip.c $(CFLAGS) -g -o $@
vmem.db:../memory/vmem.c ../memory/vmem.h
	gcc -DDEBUG ../memory/vmem.c $(CFLAGS) -g -o $@
memory.db:../memory/memory.c ../memory/memory.h
	gcc -DDEBUG ../memory/memory.c $(CFLAGS) -g -o $@
emulator.db:../emulator.c
	gcc -DDEBUG ../emulator.c $(CFLAGS) -g -o $@
debug.db:../debug.c
	gcc -DDEBUG ../debug.c $(CFLAGS) -g -o $@