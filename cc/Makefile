CFLAGS=-c
CFLAGS+=-g -O0
CFLAGS+=-Wall
OBJPREFIX=./objs/
CC=cc
MINIC_OBJS=minic_ast.o minic_parser.tab.o minic_typetree.o minic_symtable.o minic_symfunc.o minic_scanner.o minic_astcheck.o minic_triargexpr.o minic_triargtable.o minic_basicblock.o minic_varmapping.o minic_flowanalyse.o minic_aliasanalyse.o minic_regalloc.o minic_machinecode.o minic_asmout.o minic_peephole.o
minicc:$(MINIC_OBJS)
	$(CC) $(MINIC_OBJS) -o minicc -lfl
minic_peephole.o: minic_peephole.h minic_peephole.c minic_machinecode.h minic_asmout.h
	$(CC) minic_peephole.c $(CFLAGS) -o $@
minic_asmout.o: minic_asmout.h minic_asmout.c minic_machinecode.h
	$(CC) minic_asmout.c $(CFLAGS) -o $@
minic_machinecode.o:minic_machinecode.h minic_machinecode.c  minic_asmout.h minic_basicblock.h minic_flowanalyse.h minic_aliasanalyse.h minic_varmapping.h minic_regalloc.h minic_typedef.h
	$(CC) minic_machinecode.c $(CFLAGS) -o $@
minic_flowanalyse.o:minic_flowanalyse.h minic_flowanalyse.c minic_basicblock.h minic_symtable.h minic_triargexpr.h
	$(CC) minic_flowanalyse.c $(CFLAGS) -o $@
minic_ast.o:minic_ast.h minic_ast.c minic_typedef.h
	$(CC) minic_ast.c $(CFLAGS) -o $@
minic_astcheck.o:minic_astcheck.c minic_astcheck.h minic_ast.h minic_typedef.h minic_typetree.h minic_symtable.h minic_symfunc.h
	$(CC) minic_astcheck.c $(CFLAGS) -o $@
minic_typetree.o:minic_typetree.h minic_typetree.c minic_typedef.h
	$(CC) minic_typetree.c $(CFLAGS) -o $@
minic_symtable.o:minic_symtable.h minic_symtable.c minic_typedef.h minic_typetree.h
	$(CC) minic_symtable.c $(CFLAGS) -o $@
minic_symfunc.o:minic_symfunc.h minic_symfunc.c minic_symtable.h minic_typedef.h
	$(CC) minic_symfunc.c $(CFLAGS) -o $@
minic_triargexpr.o:minic_triargexpr.h minic_triargexpr.c minic_ast.h minic_typetree.h minic_flowanalyse.h
	$(CC) minic_triargexpr.c $(CFLAGS) -o $@
minic_triargtable.o:minic_triargtable.h minic_triargtable.c minic_triargexpr.h
	$(CC) minic_triargtable.c $(CFLAGS) -o $@
minic_aliasanalyse.o:minic_aliasanalyse.h minic_aliasanalyse.c
	$(CC) minic_aliasanalyse.c $(CFLAGS) -o $@
minic_basicblock.o:minic_basicblock.h minic_basicblock.c minic_triargexpr.h minic_triargtable.h minic_varmapping.h
	$(CC) minic_basicblock.c $(CFLAGS) -o $@
minic_varmapping.o:minic_varmapping.h minic_varmapping.c minic_symtable.h minic_triargexpr.h minic_flowanalyse.h
	$(CC) minic_varmapping.c $(CFLAGS) -o $@
minic_regalloc.o:minic_regalloc.h minic_regalloc.c minic_varmapping.h minic_flowanalyse.h
	$(CC) minic_regalloc.c $(CFLAGS) -o $@
minic_parser.tab.o:minic_parser.tab.c
	$(CC) minic_parser.tab.c $(CFLAGS) -o $@
minic_scanner.o:minic_scanner.c minic_typedef.h minic_parser.tab.h
	$(CC) minic_scanner.c -c -g -o $@
minic_scanner.c:minic_scanner.l minic_parser.tab.h
	flex -o minic_scanner.c minic_scanner.l
minic_parser.tab.h:minic_parser.y minic_typedef.h minic_ast.h minic_typetree.h minic_symtable.h minic_symfunc.h minic_astcheck.h minic_triargexpr.h minic_basicblock.h minic_aliasanalyse.h minic_regalloc.h minic_machinecode.h
	bison -d minic_parser.y
minic_parser.tab.c:minic_parser.y minic_typedef.h minic_ast.h minic_typetree.h minic_symtable.h minic_symfunc.h minic_astcheck.h minic_triargexpr.h minic_basicblock.h minic_aliasanalyse.h minic_regalloc.h minic_machinecode.h
	bison -d minic_parser.y
