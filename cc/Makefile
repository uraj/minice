CFLAGS=-c
CFLAGS+=-g
CFLAGS+=-Wall
OBJPREFIX=./objs/
MINIC_OBJS=minic_ast.o minic_parser.tab.o minic_typetree.o minic_symtable.o minic_symfunc.o minic_typedef.h minic_scanner.o minic_astcheck.o minic_triargexpr.o minic_triargtable.o minic_basicblock.o minic_varmapping.o minic_flowanalyse.o minic_aliasanalyse.o minic_regalloc.o minic_machinecode.o
minicc:$(MINIC_OBJS)
	gcc $(MINIC_OBJS) -o minicc -lfl
minic_machinecode.o:minic_machinecode.h minic_machinecode.c
	gcc minic_machinecode.c $(CFLAGS) -o $@
minic_flowanalyse.o:minic_flowanalyse.h minic_flowanalyse.c
	gcc minic_flowanalyse.c $(CFLAGS) -o $@
minic_ast.o:minic_ast.h minic_ast.c
	gcc minic_ast.c $(CFLAGS) -o $@
minic_astcheck.o:minic_astcheck.c minic_astcheck.h
	gcc minic_astcheck.c $(CFLAGS) -o $@
minic_typetree.o:minic_typetree.h minic_typetree.c
	gcc minic_typetree.c $(CFLAGS) -o $@
minic_symtable.o:minic_symtable.h minic_symtable.c
	gcc minic_symtable.c $(CFLAGS) -o $@
minic_symfunc.o:minic_symfunc.h minic_symfunc.c
	gcc minic_symfunc.c $(CFLAGS) -o $@
minic_triargexpr.o:minic_triargexpr.h minic_triargexpr.c
	gcc minic_triargexpr.c $(CFLAGS) -o $@
minic_triargtable.o:minic_triargtable.h minic_triargtable.c
	gcc minic_triargtable.c $(CFLAGS) -o $@
minic_aliasanalyse.o:minic_aliasanalyse.h minic_aliasanalyse.c
	gcc minic_aliasanalyse.c $(CFLAGS) -o $@
minic_basicblock.o:minic_basicblock.h minic_basicblock.c
	gcc minic_basicblock.c $(CFLAGS) -o $@
minic_varmapping.o:minic_varmapping.h minic_varmapping.c
	gcc minic_varmapping.c $(CFLAGS) -o $@
minic_regalloc.o:minic_regalloc.h minic_regalloc.c
	gcc minic_regalloc.c $(CFLAGS) -o $@
minic_parser.tab.o:minic_parser.tab.c minic_typedef.h minic_ast.h
	gcc minic_parser.tab.c $(CFLAGS) -o $@
minic_scanner.o:minic_scanner.c minic_typedef.h minic_parser.tab.h
	gcc minic_scanner.c -c -g -o $@
minic_scanner.c:minic_scanner.l minic_parser.tab.h
	flex -o minic_scanner.c minic_scanner.l
minic_parser.tab.h:minic_parser.y
	bison -d minic_parser.y
minic_parser.tab.c:minic_parser.y
	bison -d minic_parser.y
