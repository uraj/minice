#include "minic_asmout.h"
#include <stdlib.h>

extern struct mach_arg null;

static int special_reg_mach_out(int reg_num, FILE * out_buf)
{
	switch(reg_num)
	{
		case 27:
			fprintf(out_buf, "fp");
			return 1;
		case 29:
			fprintf(out_buf, "sp");
			return 1;
		case 30:
			fprintf(out_buf, "lr");
			return 1;
		case 31:
			fprintf(out_buf, "pc");
			return 1;
		default:
			return 0;
	}
}

static void mach_arg_out(struct mach_arg marg1, struct mach_arg marg2, int marg3, enum shift_type shift, FILE * out_buf)
{
    if(marg1.type != Unused)
    {
        if(marg1.type == Mach_Reg)
		{
			fprintf(out_buf, ", ");
			if(!special_reg_mach_out(marg1.reg, out_buf))
				fprintf(out_buf, "r%d", marg1.reg);
		}
        else if(marg1.type == Mach_Imm)
            fprintf(out_buf, ", #%d", marg1.imme);
        else
        {
            printf("Wrong: mach_arg_out()\n");
            exit(1);
        }
    }
    if(marg2.type != Unused)
    {
        if(marg2.type == Mach_Reg)
		{
			fprintf(out_buf, ", ");
			if(!special_reg_mach_out(marg2.reg, out_buf))
				fprintf(out_buf, "r%d", marg2.reg);
		}
        else if(marg2.type == Mach_Imm)
            fprintf(out_buf, ", #%d", marg2.imme);
        else
        {
            printf("Wrong: mach_arg_out()\n");
            exit(1);
        }
    }
    if(shift != NO)
    {
        switch(shift)
        {
            case LL:
                fprintf(out_buf, " << #%d", marg3);
				break;
            case LR:
                fprintf(out_buf, " >> #%d", marg3);
				break;
            case AR:
                fprintf(out_buf, " |> #%d", marg3);
				break;
            case RR:
                fprintf(out_buf, "<>%d", marg3);
				break;
            default:
                break;
        }
    }
    fprintf(out_buf, "\n");
    return;
}

static void mach_mem_arg_out(struct mach_arg marg1, int offset, struct mach_arg marg2, int marg3, enum shift_type shift, enum index_type indexed, FILE *out_buf)
{
    if(marg1.type != Unused)
    {
        if(marg1.type == Mach_Reg)
        {
            fprintf(out_buf, ", [");
			if(!special_reg_mach_out(marg1.reg, out_buf))
				fprintf(out_buf, "r%d", marg1.reg);
			if(indexed != POST)
			{
				if(offset == -1)
					fprintf(out_buf, "-]");
				else if(offset == 1)
					fprintf(out_buf, "+]");
				else
					fprintf(out_buf, "]");
			}
			else
			{
				if(offset == -1)
					fprintf(out_buf, "]-");
				else if(offset == 1)
					fprintf(out_buf, "]+");
				else
					fprintf(out_buf, "]");
			}
        }
		else if(marg1.type == Mach_Label)
			fprintf(out_buf, ", %s", marg1.label);
        else
        {
            printf("Wrong: mach_arg_out()\n");
            exit(1);
        }
    }
    if(marg2.type != Unused)
    {
        if(marg2.type == Mach_Reg)
		{
			fprintf(out_buf, ", ");
			if(!special_reg_mach_out(marg2.reg, out_buf))
				fprintf(out_buf, "r%d", marg2.reg);
		}
        else if(marg2.type == Mach_Imm)
            fprintf(out_buf, ", #%d", marg2.imme);
        else
        {
            printf("Wrong: mach_arg_out()\n");
            exit(1);
        }
    }
    if(shift != NO)
    {
        switch(shift)
        {
            case LL:
                fprintf(out_buf, " << #%d", marg3);
				break;
            case LR:
                fprintf(out_buf, " >> #%d", marg3);
				break;
            case AR:
                fprintf(out_buf, " |> #%d", marg3);
				break;
            case RR:
                fprintf(out_buf, "<>%d", marg3);
				break;
			default:
				break;
        }
    }
    fprintf(out_buf, "\n");
    return;
}

void mcode_out(const struct mach_code * mcode, FILE * out_buf)
{
    switch(mcode->op_type)
    {
        case DP:
            switch(mcode->dp_op)
            {
                case AND:
                    fprintf(out_buf, "\tand\t");
                    break;
                case SUB:
                    fprintf(out_buf, "\tsub\t");
                    break;
                case RSUB:
                    fprintf(out_buf, "\trsb\t");
                    break;
                case ADD:
					fprintf(out_buf, "\tadd\t");
                    break;
                case OR:
                    fprintf(out_buf, "\tor\t");
                    break;
                case XOR:
                    fprintf(out_buf, "\txor\t");
                    break;
                case MUL:
                    fprintf(out_buf, "\tmul\t");
                    break;
                case MOV:
                    fprintf(out_buf, "\tmov\t"); 
					break;
                case MVN:
                    fprintf(out_buf, "\tmvn\t");
                    break;
                case MOVCOND:   /* it sucks */
                    ;
            }
			if(!special_reg_mach_out(mcode->dest, out_buf))
				fprintf(out_buf, "r%d", mcode->dest);
            mach_arg_out(mcode->arg1, mcode->arg2, mcode->arg3, mcode->shift, out_buf);
            break;
        case CMP:
            switch(mcode->cmp_op)
            {
                case CMPSUB_A:
                    fprintf(out_buf, "\tcmpsub.a");
                    break;
            }
            mach_arg_out(mcode->arg1, mcode->arg2, mcode->arg3, mcode->shift, out_buf); 
            break;
        case MEM:
            switch(mcode->mem_op)
            {
                case LDW:
                    fprintf(out_buf, "\tldw");
                    break;
                case STW:
                    fprintf(out_buf, "\tstw");
                    break;
                case LDB:
                    fprintf(out_buf, "\tldb");
                    break;
                case STB:
                    fprintf(out_buf, "\tstb");
                    break;
            }
            if(mcode->indexed == PREW || mcode->indexed == POST)
                fprintf(out_buf, ".w");
            fprintf(out_buf, "\t");
			if(!special_reg_mach_out(mcode->dest, out_buf))
				fprintf(out_buf, "r%d", mcode->dest);
			mach_mem_arg_out(mcode->arg1, mcode->offset, mcode->arg2, mcode->arg3, mcode->shift, mcode->indexed, out_buf);
            break;
        case BRANCH:
            switch(mcode->branch_op)
            {
                case JUMP:
                    fprintf(out_buf, "\tjump\t");	
					if(!special_reg_mach_out(mcode->dest, out_buf))
						fprintf(out_buf, "r%d", mcode->dest);
					fprintf(out_buf, "\n");
                    break;
                case B:
                    if(mcode->link)
                        fprintf(out_buf, "\tb.l\t%s\n", mcode->dest_label);
                    else
                        fprintf(out_buf, "\tb\t%s\n", mcode->dest_label);
                    break;
                case BCOND:
                    switch(mcode->cond)
                    {    
                        case Mach_EQ:
                            fprintf(out_buf, "\tbeq");
                            break;
                        case Mach_NE:
                            fprintf(out_buf, "\tbne");
                            break;
                        case Mach_SL:
                            fprintf(out_buf, "\tbslt");
                            break;
                        case Mach_SG:
                            fprintf(out_buf, "\tbsgt");
                            break;
                        case Mach_EL:
                            fprintf(out_buf, "\tbsle");
                            break;
                        case Mach_EG:
                            fprintf(out_buf, "\tbsge");
                            break;
                    }
                    fprintf(out_buf, "\t%s\n", mcode->dest_label);
                    break;
            }
            break;
        case LABEL:
            fprintf(out_buf, "%s\n", mcode->label);
            return;
    }
    return;
}







