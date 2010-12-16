CFLAGS=-c
CFLAGS+=-Wall
CFLAGS+=-I../src
SIM_OBJS=console.o fetch.o decode.o control.o execute.o memaccess.o writeback.o elfmanip.o vmem.o memory.o
SIM_OBJS_DB=console.db fetch.db decode.db control.db execute.db memaccess.db writeback.db elfmanip.db vmem.db memory.db

sim:../sim
../sim:$(SIM_OBJS_DB) simulator.db debug.db
	gcc -DDEBUG $(SIM_OBJS_DB) simulator.db debug.db -o ../sim
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
simulator.db:../src/simulator.c
	gcc -DDEBUG ../src/simulator.c $(CFLAGS) -g -o $@
debug.db:../src/debug.c
	gcc -DDEBUG ../src/debug.c $(CFLAGS) -g -o $@
console.db:../src/console/console.c ../src/console/console.h
	gcc -DDEBUG ../src/console/console.c $(CFLAGS) -g -o $@

sim-re:../sim-re
../sim-re:$(SIM_OBJS) simulator.o debug.o
	gcc $(SIM_OBJS) simulator.o debug.o -O2 -o ../sim-re
fetch.o:../src/pipeline/fetch.c ../src/pipeline/fetch.h ../src/pipeline/globdefs.h
	gcc ../src/pipeline/fetch.c $(CFLAGS) -O2 -o $@
control.o:../src/pipeline/control.c ../src/pipeline/control.h ../src/pipeline/globdefs.h
	gcc ../src/pipeline/control.c $(CFLAGS) -O2 -o $@
decode.o:../src/pipeline/decode.c ../src/pipeline/decode.h ../src/pipeline/globdefs.h
	gcc ../src/pipeline/decode.c $(CFLAGS) -O2 -o $@
execute.o:../src/pipeline/execute.c ../src/pipeline/execute.h ../src/pipeline/globdefs.h
	gcc ../src/pipeline/execute.c $(CFLAGS) -O2 -o $@
memaccess.o:../src/pipeline/memaccess.c ../src/pipeline/memaccess.h ../src/pipeline/globdefs.h
	gcc ../src/pipeline/memaccess.c $(CFLAGS) -O2 -o $@
writeback.o:../src/pipeline/writeback.c ../src/pipeline/writeback.h ../src/pipeline/globdefs.h
	gcc ../src/pipeline/writeback.c $(CFLAGS) -O2 -o $@
elfmanip.o:../src/loader/elfmanip.c ../src/loader/elfmanip.h
	gcc ../src/loader/elfmanip.c $(CFLAGS) -O2 -o $@
vmem.o:../src/memory/vmem.c ../src/memory/vmem.h
	gcc ../src/memory/vmem.c $(CFLAGS) -O2 -o $@
memory.o:../src/memory/memory.c ../src/memory/memory.h
	gcc ../src/memory/memory.c $(CFLAGS) -O2 -o $@
simulator.o:../src/simulator.c
	gcc ../src/simulator.c $(CFLAGS) -O2 -o $@
debug.o:../src/debug.c
	gcc ../src/debug.c $(CFLAGS) -O2 -o $@
console.o:../src/console/console.c ../src/console/console.h
	gcc ../src/console/console.c $(CFLAGS) -O2 -o $@
