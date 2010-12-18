CFLAGS=-c
CFLAGS+=-Wall
CFLAGS+=-Isrc
OBJ_DIR=objs/
SIM_OBJS=$(OBJ_DIR)console.o $(OBJ_DIR)fetch.o $(OBJ_DIR)decode.o $(OBJ_DIR)control.o $(OBJ_DIR)execute.o $(OBJ_DIR)memaccess.o $(OBJ_DIR)writeback.o $(OBJ_DIR)elfmanip.o $(OBJ_DIR)vmem.o $(OBJ_DIR)memory.o $(OBJ_DIR)simulator.o
SIM_OBJS_DB=$(OBJ_DIR)console.db $(OBJ_DIR)fetch.db $(OBJ_DIR)decode.db $(OBJ_DIR)control.db $(OBJ_DIR)execute.db $(OBJ_DIR)memaccess.db $(OBJ_DIR)writeback.db $(OBJ_DIR)elfmanip.db $(OBJ_DIR)vmem.db $(OBJ_DIR)memory.db $(OBJ_DIR)simulator.db

sim:$(SIM_OBJS_DB)
	gcc -DDEBUG $(SIM_OBJS_DB) -o $@
$(OBJ_DIR)fetch.db:src/pipeline/fetch.c src/pipeline/fetch.h src/pipeline/globdefs.h
	gcc -DDEBUG src/pipeline/fetch.c $(CFLAGS) -g -o $@
$(OBJ_DIR)control.db:src/pipeline/control.c src/pipeline/control.h src/pipeline/globdefs.h
	gcc -DDEBUG src/pipeline/control.c $(CFLAGS) -g -o $@
$(OBJ_DIR)decode.db:src/pipeline/decode.c src/pipeline/decode.h src/pipeline/globdefs.h
	gcc -DDEBUG src/pipeline/decode.c $(CFLAGS) -g -o $@
$(OBJ_DIR)execute.db:src/pipeline/execute.c src/pipeline/execute.h src/pipeline/globdefs.h
	gcc -DDEBUG src/pipeline/execute.c $(CFLAGS) -g -o $@
$(OBJ_DIR)memaccess.db:src/pipeline/memaccess.c src/pipeline/memaccess.h src/pipeline/globdefs.h
	gcc -DDEBUG src/pipeline/memaccess.c $(CFLAGS) -g -o $@
$(OBJ_DIR)writeback.db:src/pipeline/writeback.c src/pipeline/writeback.h src/pipeline/globdefs.h
	gcc -DDEBUG src/pipeline/writeback.c $(CFLAGS) -g -o $@
$(OBJ_DIR)elfmanip.db:src/loader/elfmanip.c src/loader/elfmanip.h
	gcc -DDEBUG src/loader/elfmanip.c $(CFLAGS) -g -o $@
$(OBJ_DIR)vmem.db:src/memory/vmem.c src/memory/vmem.h
	gcc -DDEBUG src/memory/vmem.c $(CFLAGS) -g -o $@
$(OBJ_DIR)memory.db:src/memory/memory.c src/memory/memory.h
	gcc -DDEBUG src/memory/memory.c $(CFLAGS) -g -o $@
$(OBJ_DIR)simulator.db:src/simulator.c
	gcc -DDEBUG src/simulator.c $(CFLAGS) -g -o $@
$(OBJ_DIR)console.db:src/console/console.c src/console/console.h
	gcc -DDEBUG src/console/console.c $(CFLAGS) -g -o $@

sim-re:$(SIM_OBJS)
	gcc $(SIM_OBJS) -O2 -o sim-re
$(OBJ_DIR)fetch.o:src/pipeline/fetch.c src/pipeline/fetch.h src/pipeline/globdefs.h
	gcc src/pipeline/fetch.c $(CFLAGS) -O2 -o $@
$(OBJ_DIR)control.o:src/pipeline/control.c src/pipeline/control.h src/pipeline/globdefs.h
	gcc src/pipeline/control.c $(CFLAGS) -O2 -o $@
$(OBJ_DIR)decode.o:src/pipeline/decode.c src/pipeline/decode.h src/pipeline/globdefs.h
	gcc src/pipeline/decode.c $(CFLAGS) -O2 -o $@
$(OBJ_DIR)execute.o:src/pipeline/execute.c src/pipeline/execute.h src/pipeline/globdefs.h
	gcc src/pipeline/execute.c $(CFLAGS) -O2 -o $@
$(OBJ_DIR)memaccess.o:src/pipeline/memaccess.c src/pipeline/memaccess.h src/pipeline/globdefs.h
	gcc src/pipeline/memaccess.c $(CFLAGS) -O2 -o $@
$(OBJ_DIR)writeback.o:src/pipeline/writeback.c src/pipeline/writeback.h src/pipeline/globdefs.h
	gcc src/pipeline/writeback.c $(CFLAGS) -O2 -o $@
$(OBJ_DIR)elfmanip.o:src/loader/elfmanip.c src/loader/elfmanip.h
	gcc src/loader/elfmanip.c $(CFLAGS) -O2 -o $@
$(OBJ_DIR)vmem.o:src/memory/vmem.c src/memory/vmem.h
	gcc src/memory/vmem.c $(CFLAGS) -O2 -o $@
$(OBJ_DIR)memory.o:src/memory/memory.c src/memory/memory.h
	gcc src/memory/memory.c $(CFLAGS) -O2 -o $@
$(OBJ_DIR)simulator.o:src/simulator.c
	gcc src/simulator.c $(CFLAGS) -O2 -o $@
$(OBJ_DIR)console.o:src/console/console.c src/console/console.h
	gcc src/console/console.c $(CFLAGS) -O2 -o $@