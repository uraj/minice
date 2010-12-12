CFLAGS=-c
CFLAGS+=-Wall
CFLAGS+=-I../src
EMU_OBJS=fetch.o decode.o control.o execute.o memaccess.o writeback.o elfmanip.o vmem.o memory.o
EMU_OBJS_DB=fetch.db decode.db control.db execute.db memaccess.db writeback.db elfmanip.db vmem.db memory.db

emu:../emu
../emu:$(EMU_OBJS) emulator.o debug.o
	gcc $(EMU_OBJS) emulator.o debug.o -o ../emu
fetch.o:../src/pipeline/fetch.c ../src/pipeline/fetch.h ../src/pipeline/globdefs.h
	gcc ../src/pipeline/fetch.c $(CFLAGS) -o $@
control.o:../src/pipeline/control.c ../src/pipeline/control.h ../src/pipeline/globdefs.h
	gcc ../src/pipeline/control.c $(CFLAGS) -o $@
decode.o:../src/pipeline/decode.c ../src/pipeline/decode.h ../src/pipeline/globdefs.h
	gcc ../src/pipeline/decode.c $(CFLAGS) -o $@
execute.o:../src/pipeline/execute.c ../src/pipeline/execute.h ../src/pipeline/globdefs.h
	gcc ../src/pipeline/execute.c $(CFLAGS) -o $@
memaccess.o:../src/pipeline/memaccess.c ../src/pipeline/memaccess.h ../src/pipeline/globdefs.h
	gcc ../src/pipeline/memaccess.c $(CFLAGS) -o $@
writeback.o:../src/pipeline/writeback.c ../src/pipeline/writeback.h ../src/pipeline/globdefs.h
	gcc ../src/pipeline/writeback.c $(CFLAGS) -o $@
elfmanip.o:../src/loader/elfmanip.c ../src/loader/elfmanip.h
	gcc ../src/loader/elfmanip.c $(CFLAGS) -o $@
vmem.o:../src/memory/vmem.c ../src/memory/vmem.h
	gcc ../src/memory/vmem.c $(CFLAGS) -o $@
memory.o:../src/memory/memory.c ../src/memory/memory.h
	gcc ../src/memory/memory.c $(CFLAGS) -o $@
emulator.o:../src/emulator.c
	gcc ../src/emulator.c $(CFLAGS) -o $@
debug.o:../src/debug.c
	gcc ../src/debug.c $(CFLAGS) -o $@

emu-db:../emu-db
../emu-db:$(EMU_OBJS_DB) emulator.db debug.db
	gcc -DDEBUG $(EMU_OBJS_DB) emulator.db debug.db -o ../emu-db
fetch.db:../src/pipeline/fetch.c ../src/pipeline/fetch.h ../src/pipeline/globdefs.h
	gcc -DDEBUG ../src/pipeline/fetch.c $(CFLAGS) -g -o $@
control.db:../src/pipeline/control.c ../src/pipeline/control.h ../src/pipeline/globdefs.h
	gcc -DDEBUG ../src/pipeline/control.c $(CFLAGS) -g -o $@
decode.db:../src/pipeline/decode.c ../src/pipeline/decode.h ../src/pipeline/globdefs.h
	gcc -DDEBUG ../src/pipeline/decode.c $(CFLAGS) -g -o $@
execute.db:../src/pipeline/execute.c ../src/pipeline/execute.h ../src/pipeline/globdefs.h
	gcc -DDEBUG ../src/pipeline/execute.c $(CFLAGS) -g -o $@
memaccess.db:../src/pipeline/memaccess.c ../src/pipeline/memaccess.h ../src/pipeline/globdefs.h
	gcc -DDEBUG ../src/pipeline/memaccess.c $(CFLAGS) -g -o $@
writeback.db:../src/pipeline/writeback.c ../src/pipeline/writeback.h ../src/pipeline/globdefs.h
	gcc -DDEBUG ../src/pipeline/writeback.c $(CFLAGS) -g -o $@
elfmanip.db:../src/loader/elfmanip.c ../src/loader/elfmanip.h
	gcc -DDEBUG ../src/loader/elfmanip.c $(CFLAGS) -g -o $@
vmem.db:../src/memory/vmem.c ../src/memory/vmem.h
	gcc -DDEBUG ../src/memory/vmem.c $(CFLAGS) -g -o $@
memory.db:../src/memory/memory.c ../src/memory/memory.h
	gcc -DDEBUG ../src/memory/memory.c $(CFLAGS) -g -o $@
emulator.db:../src/emulator.c
	gcc -DDEBUG ../src/emulator.c $(CFLAGS) -g -o $@
debug.db:../src/debug.c
	gcc -DDEBUG ../src/debug.c $(CFLAGS) -g -o $@