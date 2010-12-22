#include "minic_asmout.h"

void mach_arg_out(const struct mach_arg * marg);
void mach_arg_out(const struct mach_arg * marg, enum shift_type shift);

void asm_out(const struct mach_code * mcode, FILE * out_buff)
{
    switch(mcode->op_type)
    {
        case DP:
            switch(mcode->dp_op)
            {
                case AND:
                    fprintf(out_buff, "\tand\tr%d, ", mcode->dest);
                case SUB:
                    fprintf(out_buff, "\tsub\tr%d, ", mcode->dest);
                case ADD:
                    fprintf(out_buff, "\taddr%d, ", mcode->dest);
                case OR:
                    fprintf(out_buff, "\torr%d, ", mcode->dest);
                case XOR:
                    fprintf(out_buff, "\txorr%d, ", mcode->dest);
                case MUL:
                    fprintf(out_buff, "\tmulr%d, ", mcode->dest);
                case MULSL:
                    fprintf(out_buff, "\tmulslr%d, ", mcode->dest);
                case MOV:
                    fprintf(out_buff, "\tmovr%d, ", mcode->dest);
                case MVN:
                    fprintf(out_buff, "\tmvnr%d, ", mcode->dest);
                case MOVCOND:   /* it sucks */
                    ;
            }
            break;
        case CMP:
            switch(mocde->cmp_op)
            {
            }
            break;
        case MEM:
            switch(mcode->mem_op)
            {
            }
            break;
        case BRANCH:
            switch(mcode->branch_op)
            {
            }
            break;
        case LABEL:
            fprintf(out_buff, "%s\n", mcode->label);
            return;
    }
    return;
}
