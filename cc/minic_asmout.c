#include "minic_asmout.h"
#include <stdlib.h>

extern struct mach_arg null;

static void mach_arg_out(struct mach_arg marg1, struct mach_arg marg2, int marg3, enum shift_type shift, FILE * out_buf)
{
    if(marg1.type != Unused)
    {
        if(marg1.type == Mach_Reg)
            fprintf(out_buf, ", r%d", marg1.reg);
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
            fprintf(out_buf, ", r%d", marg2.reg);
        else if(marg2.type == Mach_Imm)
            fprintf(out_buf, ", #%d", marg2.imme);
        else
        {
            printf("Wrong: mach_arg_out()\n");
            exit(1);
        }
    }
    if(marg3 != 0)
    {
        switch(shift)
        {
            case LL:
                fprintf(out_buf, " << #%d", marg3);
            case LR:
                fprintf(out_buf, " >> #%d", marg3);
            case AR:
                fprintf(out_buf, " |> #%d", marg3);
            case RR:
                fprintf(out_buf, "<>%d", marg3);
            default:
                ;
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
                    fprintf(out_buf, "\tand\tr%d", mcode->dest);
                    break;
                case SUB:
                    fprintf(out_buf, "\tsub\tr%d", mcode->dest);
                    break;
                case RSUB:
                    fprintf(out_buf, "\trsb\tr%d", mcode->dest);
                    break;
                case ADD:
                    fprintf(out_buf, "\tadd\tr%d", mcode->dest);
                    break;
                case OR:
                    fprintf(out_buf, "\tor\tr%d", mcode->dest);
                    break;
                case XOR:
                    fprintf(out_buf, "\txor\tr%d", mcode->dest);
                    break;
                case MUL:
                    fprintf(out_buf, "\tmul\tr%d", mcode->dest);
                    break;
                case MOV:
                    fprintf(out_buf, "\tmov\tr%d", mcode->dest);
                    break;
                case MVN:
                    fprintf(out_buf, "\tmvn\tr%d", mcode->dest);
                    break;
                case MOVCOND:   /* it sucks */
                    ;
            }
            mach_arg_out(mcode->arg1, mcode->arg2, mcode->arg3, mcode->shift, out_buf);
            break;
        case CMP:
            switch(mcode->cmp_op)
            {
                case CMPSUB_A:
                    fprintf(out_buf, "\tcmpsub.a\tr%d", mcode->dest);
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
                    fprintf(out_buf, "\tstw\tr%d", mcode->dest);
                    break;
                case LDB:
                    fprintf(out_buf, "\tldb\tr%d", mcode->dest);
                    break;
                case STB:
                    fprintf(out_buf, "\tstb\tr%d", mcode->dest);
                    break;
            }
            if(mcode->indexed == 1)
                fprintf(out_buf, ".w");
            fprintf(out_buf, "\tr%d", mcode->dest);
            if(mcode->offset == 0)
                fprintf(out_buf, " [r%d], #%d\n", mcode->arg1.reg, mcode->arg3);
            else
            {    
                if(mcode->indexed == 2)
                {
                    
                    if(mcode->offset == 1)
                        fprintf(out_buf, " [r%d+], #%d\n", mcode->arg1.reg, mcode->arg3);
                    else if(mcode->offset == -1)
                        fprintf(out_buf, " [r%d-], #%d\n", mcode->arg1.reg, mcode->arg3);
                }
                else if(mcode->indexed == 1)
                {
                    if(mcode->offset == 1)
                        fprintf(out_buf, " [r%d]+, #%d\n", mcode->arg1.reg, mcode->arg3);
                    else if(mcode->offset == -1)
                        fprintf(out_buf, " [r%d]-, #%d\n", mcode->arg1.reg, mcode->arg3);
                }
            }
            break;
        case BRANCH:
            switch(mcode->branch_op)
            {
                case JUMP:
                    fprintf(out_buf, "\tjump\tr%d\n", mcode->dest);
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
