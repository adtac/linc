#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>

using namespace std;

// Define TYPES

#define TNULL 0
#define TINT 1
#define TCH 2
#define TFN 3
#define TARR 4

char RPNCH='~';
char RPNSTR[]="~";

/*
 Structure to contain variables.
 Each variable's data is in [bits]. Members have their usual meaning.
 */

struct variable
{
    variable *next;
    char vname[32];
    int size;
    int type;
    char *bits;
};

/*
Object pointers that contain address to the global variable chain's head.
*/

variable *gstart;
variable *pt;
variable *tmp;
int var_num=0;

/*
Structure to contain functions.
[start] denotes line number at which function starts. [end] denotes line number at which frunction ends.
*/

struct function
{
    char name[32];
    int start;
    int end;
};
function func[1024];
int fc=0;

/*
Structure to contain each line.
*/

struct line
{
    char con[256];
};
char glo_name[1024];

void rpn(char *, char *);
int rpn_ev(char *, variable *);

/*
Set of each type's names: "spacing" strings.
*/

char spacing[][32]={"null", "int", "char", "function"};

/*
Functions to evaluate if a given character is:
    1. An arithematic operator
    2. An alphabet
    3. Special Character (Arithematic + Brackets + Period)
*/

// #1
int is_arith(char ch)
{
    return ch=='+'||ch=='-'||ch=='='||ch==':'||ch=='/'||ch=='*'||ch=='%';
}
// #2
int is_alpha(char ch)
{
    return (ch>='a'&&ch<='z')||(ch>='A'||ch<='Z');
}
// #3
int special_char(char ch)
{
    return is_arith(ch)||ch=='('||ch==')'||ch=='.';
}

/*
Evaluates whether a given char array is a "spacing" string.
*/

int is_spacing(char *p)
{
    int i, flag=1;
    for(i=0;i<sizeof(spacing)/32&&flag;i++)
        flag=strcmp(spacing[i], p);
    if(flag==0)
        return i;
    else
        return -1;
}

/*
Functions to exit out of LINC application with message:
    1. Print reason for exit.
    2. If compilation error, display what error was encountered.
*/

// #1
void close(int r)
{
    switch(r)
    {
        case 0:
            printf("\n\nPress any key to terminate LINC program...");
            break;

        case 1:
            printf("\n\nProgram did not run due to compilation error(s)...");
            break;
    }
}
//#2
void comp_err(int r, char *p)
{
    printf("\n\n");
    switch(r)
    {
        case 1:
            printf("Undefined variable [%s]", p);
            close(1);
            exit(1);
            break;
        case 2:
            printf("Cannot assign [%s] a non-int value", p);
            close(1);
            exit(1);
            break;
        case 3:
            printf("main missing from program", p);
            close(1);
            exit(1);
            break;
        case 4:
            printf("Parameter passing error: %s", p);
            close(1);
            exit(1);
            break;
        case 5:
            printf("Wrong number of arguments for %s", p);
            close(1);
        exit(1);
            break;
    }
}

/*
Function to evaluate the number of lines in source code.
This helps us to create an appropriate memory chunk to store all the lines
Memory storage is preferred because frequent file access is too slow.
*/

int prog_line()
{
    ifstream cprog(glo_name);
    int y=0;
    char p[512];
    while(cprog.getline(p, 512))
    y++;
    cprog.close();
    return y;
}

/*
Function to create a new variable.
The [**start] parameter takes a pointer to the pointer to the head of a chain.
A pointer-to-a-pointer is used because we need to change the passed pointer to the new head.
*/

void new_var(char *cname, int csize, int ctype, variable **start)
{
    pt=*start;
    tmp=NULL;
    while(pt!=NULL)
    {
        tmp=pt;
        pt=pt->next;
    }
    if(tmp==NULL)
        *start=pt=new variable;
    else
    {
        pt=new variable;
        tmp->next=pt;
    }
    pt->next=NULL;
    strcpy(pt->vname, cname);
    pt->type=ctype;
    pt->size=csize;
    pt->bits=new char[csize];

}

/*
Function that matches the passed name to a declared function in the LINC program.
Returns 1 if such a function exists and assigns [*a] and [*b] starting and ending lines respectively.
Returns 0 otherwise.
*/

int find_function(char *p, int *a, int *b)
{
    for(int i=0;i<fc;i++)
    {
        if(strcmp(p, func[i].name)==0)
        {
            *a=func[i].start;
            *b=func[i].end;
            return 1;
        }
    }
    return 0;
}

/*
Function that returns the character held by the variable [*p] in the LINC program.
*/

char get_char_value(char *p, variable *start);

/*
Function that creates global default variables such as new line character.
*/

void build_symbols()
{
    gstart=NULL;
    new_var("endl", sizeof(char), TCH, &gstart);
    *(gstart->bits)='\n';
}

/*
Function that determines whether [*p] is a variable that is defined in the [*start] chain.
Returns variable type
*/

int is_var(char *p, variable *start)
{
    char tt[128];
    int j;
    for(j=0;p[j]&&p[j]!='[';j++)
        tt[j]=p[j];
    tt[j]=0;
    int i;
    variable *s=start;
    for(;s!=NULL;s=s->next)
    {
        if(strcmp(tt, s->vname)==0)
        {
            return s->type;
        }
    }
    return -1;
}

/*
Fucntion that deletes ALL the variables defined in [*start] chain.
This is done so as to free the memory that was obtained from the heap by a local code segment.
*/

void delete_var(variable *start)
{
    variable *s=start;
    variable *tmp;
    for(;s!=NULL;s=tmp)
    {
        tmp=s->next;
        delete s;
    }
}

/*
Function used to determine what sort of variable is being defined in line [*p]
Used during declarations.
Returns the thype of variable defined.
*/

int what_defn(char *p)
{
    char temp[64];
    int j;
    for(j=0;(p[j]>='a'&&p[j]<='z')||p[j]=='_';j++)
        temp[j]=p[j];
    int q=0;
    if(p[j]=='[')
        q=TARR;
    temp[j]='\0';
    for(j=0;j<sizeof(spacing)/32;j++)
    {
        if(strcmp(temp, spacing[j])==0)
            return q+j;
    }
    return -1;
}

/*
Function used to determine if the line [*p] is a function definition
Returns 1 if yes
Returns 0 otherwise
*/

int is_function(char *p)
{
    int flag=0;
    for(int i=0;p[i]&&flag!=1;i++)
    {
        if(is_arith(p[i]))
            flag=-1;
        if(p[i]=='('&&flag==0)
            flag=1;
    }
    return flag==1;
}

char pre_func[][32]={"print", "terminate", "input_int", "if", "while", "return", "gets"};
int num_pre_func=sizeof(pre_func)/32;

int what_function(char *p)
{
    char temp[64];
    int j;
    for(j=0;p[j]!='(';j++)
        temp[j]=p[j];
    temp[j]='\0';
    for(j=0;j<num_pre_func;j++)
    {
        if(strcmp(temp, pre_func[j])==0)
            return j;
    }
    return j;
}

/*
Set of keywords recognised.
*/

/*
Error messages that say what error was encountered during compilation (for CE)
*/

char errors[][1024]={"Unbalanced ( ) paranthesis",   //1001
                     "Missing/Extraneous \" character",            //1002
                     "Missing/Extraneous \' character"             //1003
                     };

char *error(int code)
{
    return errors[code-1001];
}

/*
Function used to determine if a particular code segment [*p] is syntactically correct.
Returns 1 if yes
Returns 0 otherwise
*/

int is_syn_corr_func(char *p)
{
    int brack=0, i;
    for(i=0;p[i];i++)
    {
        if(p[i]=='(')
            brack++;
        if(p[i]==')')
            brack--;
    }
    if(brack!=0)
        return 1001;
    for(i=0;p[i];i++)
    {
        if(p[i]=='\"')
            brack=!brack;
    }
    if(brack!=0)
        return 1002;
    for(i=0;p[i];i++)
    {
        if(p[i]=='\'')
            brack=!brack;
    }
    if(brack!=0)
        return 1003;
    return 0;
}

/*
Function used to determine if a line [*p] is: defintion of a variable.
Returns data type if yes
Returns 0 otherwise
*/

int is_defn(char *p)
{
    char tt[128];
    int j;
    for(j=0;(p[j]>='a'&&p[j]<='z')||p[j]=='_';j++)
        tt[j]=p[j];
    tt[j]=0;
    char t[1024];
    sscanf(tt, "%s", t);
    return is_spacing(tt);
}

/*
Function used to determine if a line [*p] is: assignment of a variable.
Returns data type if yes
Returns -1 otherwise
*/

int is_assignment(char *p, variable *start)
{
    int i;
    char temp[32];
    for(i=0;p[i]&&!special_char(p[i]);i++)
        temp[i]=p[i];
    temp[i]=0;
    int y=is_var(temp, start);
    if((p[i]==':'||p[i]=='=')&&y!=-1)
        return y;
    else
        return -1;
}

/*
Function used to get the value stored by an integer variable [*p] in chain [*start].
Returns value if such a var exists.
Returns -1 otherwise.
*/

int get_int_value(char *p, variable *start)
{
    int i, flag=0;
    variable *s=start;
    for(;s!=NULL;s=s->next)
    {
        if(strcmp(p, s->vname)==0)
        {
            if(s->type==TINT)
            {
                int y;
                memcpy(&y, s->bits, s->size);
                return y;
            }
        }
    }
    return -1;
}

/*
Function that returs the size of a cvariable [*p] in chain [*start].
*/

int vsize(char *p, variable *start)
{
    int i, flag=0;
    variable *s=start;
    for(;s!=NULL;s=s->next)
    {
        if(strcmp(p, s->vname)==0)
        {
            return s->size;
        }
    }
    return -1;
}

/*
Function used to set the value of an integer variable [*p] in chain [*start] to [*value].
*/

void set_int_value(char *p, int val, variable *start)
{
    int i, flag=0;
    variable *s=start;
    for(;s!=NULL;s=s->next)
    {
        if(strcmp(p, s->vname)==0)
        {
            if(s->type!=TINT)
                comp_err(2, p);
            memcpy(s->bits, &val, sizeof(val));
        }
    }
}

/*
Function used to copy the data of variable [*s] in chain [*start] to [*d] in chain [*od].
*/

void copy_var(char *d, variable *od, char *s, variable *start)
{
    variable *id=od;
    variable *is=start;
    for(;id!=NULL;id=id->next)
    {
        if(strcmp(d, id->vname)==0)
            break;
    }
    for(;is!=NULL;is=is->next)
    {
        if(strcmp(s, is->vname)==0)
            break;
    }
    memcpy(id->bits, is->bits, is->size);
}

/*
Function used to set the value of an character  variable [*p] in chain [*start] to [*val].
*/

void set_char_value(char *p, char val, variable *start)
{
    int i, flag=0;
    variable *s=start;
    for(;s!=NULL;s=s->next)
    {
        if(strcmp(p, s->vname)==0)
        {
            if(s->type!=TCH)
                comp_err(2, p);
            memcpy(s->bits, &val, sizeof(val));
        }
    }
}

/*
Function used to set the value of an string (char array) variable [*p] in chain [*start] to [*val].
*/

void set_str_value(char *p, char *val, variable *start)
{
    int i, flag=0;
    variable *s=start;
    for(;s!=NULL;s=s->next)
    {
        if(strcmp(p, s->vname)==0)
        {
            if(s->type!=TCH+TARR)
                comp_err(2, p);
            strcpy(s->bits, val);
        }
    }
}

/*
Function used to set the value of an integer variable [*p] of an array element in chain [*start] to [*val].
*/

void set_arr_value(char *p, int val, variable *start)
{
    char tt[128];
    int j;
    for(j=0;p[j]&&p[j]!='[';j++)
        tt[j]=p[j];
    tt[j]=0;
    variable *s=start;
    for(;s!=NULL;s=s->next)
    {
        if(strcmp(tt, s->vname)==0)
        {
            if(s->type<TARR)
                comp_err(2, p);
            int j;
            char temp[64], temp2[128];
            for(j=strlen(s->vname)+1;p[j]!=']';j++)
                temp[j-(strlen(s->vname)+1)]=p[j];
            temp[j-(strlen(s->vname)+1)]=0;
            rpn(temp, temp2);
            int r=rpn_ev(temp2, start);
            if(s->type==TCH+TARR)
                memcpy(s->bits+r*sizeof(char), &val, sizeof(char));
            if(s->type==TINT+TARR)
                memcpy(s->bits+r*sizeof(int), &val, sizeof(int));
        }
    }
}

/*
Function used to get the value of an integer variable [*p] of an array element in chain [*start].
Returns value if found.
*/

int get_arr_value(char *p, variable *start)
{
    char tt[128];
    int j;
    for(j=0;p[j]&&p[j]!='[';j++)
        tt[j]=p[j];
    tt[j]=0;
    variable *s=start;
    for(;s!=NULL;s=s->next)
    {
        if(strcmp(tt, s->vname)==0)
        {
            if(s->type==TINT+TARR)
            {
                char temp[64], temp2[128];
                for(j=strlen(s->vname)+1;p[j]!=']';j++)
                    temp[j-(strlen(s->vname)+1)]=p[j];
                temp[j-(strlen(s->vname)+1)]=0;
                rpn(temp, temp2);
                int r=rpn_ev(temp2, start);
                int y;
                memcpy(&y, s->bits+(r*sizeof(int)), sizeof(int));
                return y;
            }
            if(s->type==TCH+TARR)
            {
                char temp[64], temp2[128];
                for(j=strlen(s->vname)+1;p[j]!=']';j++)
                    temp[j-(strlen(s->vname)+1)]=p[j];
                temp[j-(strlen(s->vname)+1)]=0;
                rpn(temp, temp2);
                int r=rpn_ev(temp2, start);
                return s->bits[r*sizeof(char)];
            }
        }
    }
}

char get_char_value(char *p, variable *start)
{
    variable *s=start;
    for(;s!=NULL;s=s->next)
    {
        if(strcmp(p, s->vname)==0)
        {
            return s->bits[0];
        }
    }
    return -1;
}

/*
Function to print the contents of a string (char array) [*p] in chain [*start]
*/

char print_str(char *p, variable *start)
{
    variable *s=start;
    for(;s!=NULL;s=s->next)
    {
        if(strcmp(p, s->vname)==0)
        {
            if(s->type!=TCH+TARR)
                comp_err(2, p);
            printf("%s", s->bits);
        }
    }
    return -1;
}

/*
Function to copy the value of string (char array) [*q] in chain [*start] to [*p]
*/

char copy_str(char *p, char *q, variable *start)
{
    variable *s=start;
    for(;s!=NULL;s=s->next)
    {
        if(strcmp(p, s->vname)==0)
        {
            if(s->type!=TCH+TARR)
                comp_err(2, p);
            strcpy(q, s->bits);
        }
    }
    return -1;
}

/*
Function that returns the priority of a particular operator:
    '+' and '-' have equal priority
    '/', '*' and '%' have equal priority and greater priority than '+' and '-'
*/

int power_arith(char s)
{
    if(s=='/'||s=='*'||s=='%')
        return 2;
    if(s=='+'||s=='-')
        return 1;
}

/*
Fucntion that converts a string [*xin] to a RPN string [*xout]
*/

void rpn(char *xin, char *xout)
{
    char temp[1024]="0";
    if(!(xin[0]>='0'&&xin[0]<='9')&&!is_arith(xin[0]))
        strcat(temp, "+");
    strcat(temp, xin);
    strcpy(xin, temp);
    char ops[256]="";
    int top=-1;
    int k, i;
    xout[0]='[';
    xout[1]=0;
    char tp[2];
    char var[32];
    for(i=0;xin[i];)
    {
        if(is_arith(xin[i]))
        {
            if(top==-1)
                ops[++top]=xin[i++];
            else
            {
                while(top>=0&&ops[top]!='('&&ops[top]!=')'&&power_arith(ops[top])>=power_arith(xin[i]))
                {
                    tp[0]=ops[top--];
                    tp[1]=0;
                    strcat(xout, tp);
                    strcat(xout, RPNSTR);
                }
                ops[++top]=xin[i++];
            }
        }
        else if(xin[i]=='(')
        {
            ops[++top]=xin[i++];
        }
        else if(xin[i]==')')
        {
            while(top>=0&&ops[top]!='(')
            {
                tp[0]=ops[top--];
                tp[1]=0;
                strcat(xout, tp);
                strcat(xout, RPNSTR);
            }
            i++;
        }
        else
        {
            k=i;
            int brack=0;
            for(;xin[i];i++)
            {
                if(xin[i]=='(')
                    brack++;
                if(xin[i]==')')
                    brack--;
                if(is_arith(xin[i])&&brack==0)
                    break;
                var[i-k]=xin[i];
            }
            var[i-k]=0;
            strcat(xout, var);
            strcat(xout, RPNSTR);
        }
    }
    while(top>=0)
    {
        tp[0]=ops[top--];
        tp[1]=0;
        if(tp[0]!='('&&tp[0]!=')')
        {
            strcat(xout, tp);
            strcat(xout, RPNSTR);
        }
    }
    strcat(xout, "]");
}

/*
The Brain: int function(int, int, variable *);

Function that executes actual code.
This is the core of the language. This processes all the lines and perform the tasks.
It is a recursive function that calls itself if execution head needs to jump to a different line.
The first 2 parameters are the start and end line of the segment of code to execute.
The third parameter passes the variable chain to the new segment.
*/

int function(int, int, variable *);

line *lines;

/*
Fucntion that is called before calling a LINC function.
This creates all the parameter variables of a LINC function from the call in [*ln] whose variable chain is [*start].
*/

variable *parameterize(char *ln, int a, variable *start)
{
    int i, k, u;
    char temp3[1024];
    for(i=0;ln[i]!='(';i++)
        temp3[i]=ln[i];
    temp3[i]=0;
    char temp2[1024];
    i++;
    char fln[1024];
    strcpy(fln, lines[a].con);
    for(k=0;fln[k]!='(';k++);
    int brack=1;
    k++;
    variable *fstart=NULL;
    while(fln[k]&&ln[i])
    {
        char temp[1024];
        int y=i;
        int t=0;
        int brack=0;
        for(;ln[i]&&ln[i]!=','&&ln[i]!='[';i++)
        {
            if(ln[i]=='(')
                brack++;
            if(brack==0&&ln[i]==')')
                break;
            if(ln[i]==')')
                brack--;
            temp2[i-y]=ln[i];
        }
        temp2[i-y]=0;
        int iv=is_var(temp2, start), isi=0, v;
        if(iv==-1)
        {
            char rp[1024];
            rpn(temp2, rp);
            v=rpn_ev(rp, start);
            //printf("%s = %d+%d = %d\n", rp, get_int_value("a", start), get_int_value("b"), v);
            isi=1;
        }
        u=k;
        for(;fln[k]!=' '&&fln[k]!='[';k++)
            temp[k-u]=fln[k];
        temp[k-u]=0;
        int ty=is_spacing(temp);
        if(fln[k]=='[')
        {
            ty+=TARR;
            k+=2;
        }
        k++;
        ty--;
        if(!(isi==1||iv==ty))
        {
            char err[1024];
            sprintf(err, "%s is %d, %s is %d", temp, ty, temp2, iv);
            comp_err(4, err);
        }
        u=k;
        for(;fln[k]!=','&&fln[k]!=')';k++)
            temp[k-u]=fln[k];
        temp[k-u]=0;
        k++;
        i++;
        if(!isi)
        {
            new_var(temp, vsize(temp2, start), ty, &fstart);
            copy_var(temp, fstart, temp2, start);
        }
        else
        {
            new_var(temp, ty==TCH?sizeof(char):sizeof(int), ty, &fstart);
            if(ty==TCH)
                set_char_value(temp, v, fstart);
            else if(ty==TINT)
                set_int_value(temp, v, fstart);
        }
    }
    if(fln[k]!=ln[i])
    {
        comp_err(5, temp3);
    }
    return fstart;
}

/*
Fucntion that evaluates a RPN string.
The variables in the string must be present in the [*start] chain.
Returns the evaluated value.
*/

int rpn_ev(char *p, variable *start)
{
    int st[128];
    int top=-1;
    int k, i, a, b;
    char temp[1024];
    for(i=1;p[i]!=']';i++)
    {
        if(p[i]=='(')
            i++;
        else if(is_arith(p[i]))
        {
            a=st[top--];
            b=st[top--];
            switch(p[i])
            {
                case '+':
                    st[++top]=a+b;
                    break;
                case '-':
                    st[++top]=-a+b;
                    break;
                case '*':
                    st[++top]=a*b;
                    break;
                case '/':
                    st[++top]=b/a;
                    break;
                case '%':
                    st[++top]=b%a;
                    break;
            }
            i++;
        }
        else if(p[i]>='0'&&p[i]<='9')
        {
            int num=0;
            for(;p[i]!=RPNCH;i++)
            {
                num*=10;
                num+=p[i]-'0';
            }
            st[++top]=num;
        }
        else if(is_alpha(p[i]))
        {
            int u=i;
            k=i;
            for(;p[i]!=RPNCH;i++)
                temp[i-k]=p[i];
            temp[i-k]=0;
            i=k;
            char temp2[1024];
            for(;p[i]!=RPNCH&&p[i]!='('&&p[i]!='[';i++)
                temp2[i-k]=p[i];
            temp2[i-k]=0;
            for(;p[i]!=RPNCH;i++);
            int a, b;
            if(is_var(temp2, start)!=-1)
            {
                if(is_var(temp2, start)<TARR)
                    st[++top]=get_int_value(temp2, start);
                else
                    st[++top]=get_arr_value(temp, start);
            }
            else if(find_function(temp2, &a, &b))
            {
                variable *u=parameterize(temp, a-2, start);
                st[++top]=function(a, b, u);
            }
            else if(temp2[0]=='\'')
                st[++top]=temp[1];
            else
            {
                comp_err(1, temp);
            }
        }
        else;
    }
    return st[0];
}

/*
Fucntion that evaluates an if segment.
Multiple such segments comprise a <if> statement.
Each such statement is evaluated and assigned a value 1 if true, 0 if false.
OR is translated to addition and AND is translated to multiplication.
*/

char evaluate(char *p, variable *start)
{
    int i, j;
    for(i=0;p[i];i++)
    {
        if(p[i]=='<'||p[i]=='>'||p[i]=='='||p[i]=='!')
            break;
    }
    char temp[1024];
    for(j=0;j<i;j++)
        temp[j]=p[j];
    temp[j]=0;
    char temp2[1024];
    rpn(temp, temp2);
    int lhs=rpn_ev(temp2, start);
    int what;
    if(p[i]=='='&&p[i+1]=='=')
        what=1;
    if(p[i]=='!'&&p[i+1]=='=')
        what=2;
    if(p[i]=='<'&&p[i+1]=='=')
        what=3;
    if(p[i]=='>'&&p[i+1]=='=')
        what=4;
    if(p[i]=='<'&&p[i+1]!='=')
        what=5;
    if(p[i]=='>'&&p[i+1]!='=')
        what=6;
    for(;p[i];i++)
    {
        if(!(p[i]=='<'||p[i]=='>'||p[i]=='='||p[i]=='!'))
            break;
    }
    for(j=i;p[j];j++)
        temp[j-i]=p[j];
    temp[j-i]=0;
    rpn(temp, temp2);
    int rhs=rpn_ev(temp2, start);
    switch(what)
    {
        case 1:
            return lhs==rhs;
        case 2:
            return lhs!=rhs;
        case 3:
            return lhs<=rhs;
        case 4:
            return lhs>=rhs;
        case 5:
            return lhs<rhs;
        case 6:
            return lhs>rhs;
    }
}

/*
Fucntion that determines if [*p] is a number.
*/

int is_num(char *p)
{
    int flag=1;
    for(int i=0;flag&&p[i];i++)
        flag=p[i]>='0'&&p[i]<='9';
    return flag;
}

/*
Fucntion that returns the int value of [*p]
*/

int num(char *p)
{
    int num=0;
    for(int i=0;p[i];i++)
    {
        num*=10;
        num+=p[i]-'0';
    }
    return num;
}

int function(int st, int end, variable *start)
{
    int ind;
    int i, j, k;
    char temp[1024], temp2[1024], temp3[1024], ln[256], printer[256];
    for(i=st;i<end;)
    {
        strcpy(ln, lines[i].con);
        if(is_function(ln))     // Is the current line a function ?
        {
            int ftype=what_function(ln);    // What type of function ?
            switch(ftype)
            {
                // print(...) function
                case 0:
                {
                    int quo=0, brack=1;
                    for(j=6;brack>0;j++)
                    {
                        if(quo==1&&ln[j]!='\"')
                        {
                            printer[ind++]=ln[j];
                        }
                        else if(ln[j]=='\"')
                        {
                            if(quo==0)
                            {
                                ind=0;
                                quo=1;
                            }
                            else
                            {
                                quo=0;
                                printer[ind]='\0';
                                printf("%s", printer);
                            }
                        }
                        else if(ln[j]=='(')
                            brack++;
                        else if(ln[j]==')')
                            brack--;
                        else if(ln[j]==' ')
                            continue;
                        else if(ln[j]=='.')
                            continue;
                        else if(quo==0)
                        {
                            k=j;
                            for(;ln[j]!='.'&&ln[j]!=')';j++)
                                printer[j-k]=ln[j];
                            printer[j-k]='\0';
                            j--;
                            int qw=is_var(printer, start);
                            if(qw==TINT)
                                printf("%d", get_int_value(printer, start));
                            else if(qw==TCH)
                                printf("%c", get_char_value(printer, start));
                            else if(qw==TINT+TARR)
                                printf("%d", get_arr_value(printer, start));
                            else if(qw==TCH+TARR)
                                print_str(printer, start);
                            else
                                comp_err(1, printer);
                        }
                        else ;
                    }
                }
                i++;
                break;

                // terminate() function
                case 1:
                {
                    close(0);
                    return 0;
                }

                // input_int(...) function
                case 2:
                {
                    for(j=10;ln[j]!=')';j++)
                        temp[j-10]=ln[j];
                    temp[j-10]=0;
                    int num;
                    scanf("%d", &num);
                    set_int_value(temp, num, start);
                }
                i++;
                break;

                // Handle if statements
                case 3:
                {
                    int brack=1, co=0;
                    for(j=3;;j++)
                    {
                        if(ln[j]=='(')
                        {
                            temp2[co++]=ln[j];
                            brack++;
                        }
                        else if(ln[j]==')')
                        {
                            brack--;
                            if(brack<=0)
                                break;
                            if(ln[j-1]==')')
                            {
                                temp2[co++]=')';
                                continue;
                            }
                            k=j-1;
                            for(;ln[k]!='(';k--);
                            k++;
                            int s=k;
                            for(;ln[k]!=')';k++)
                                temp[k-s]=ln[k];
                            temp[k-s]=0;
                            temp2[co++]='0'+evaluate(temp, start);
                            temp2[co++]=')';
                        }
                        else if(ln[j]=='&'||ln[j]=='|')
                        {
                            temp2[co++]=ln[j]=='&'?'*':'+';
                            j++;
                        }
                        else ;
                    }
                    temp2[co-1]='\0';
                    char temp3[1024];
                    rpn(temp2, temp3);
                    int res=rpn_ev(temp3, start), beg, over;
                    if(res)
                    {
                        k=i+1;
                        int dbrack=0;
                        do
                        {
                            if(strcmp(lines[k].con, "}")==0)
                                dbrack--;
                            if(strcmp(lines[k].con, "{")==0)
                                dbrack++;
                            k++;
                        }while(dbrack>0);
                        k--;
                        char hash[32];
                        function(i+2, k, start);
                        if(strcmp(lines[k+1].con, "else")==0)
                        {
                            k=k+2;
                            dbrack=0;
                            do
                            {
                                if(strcmp(lines[k].con, "}")==0)
                                    dbrack--;
                                if(strcmp(lines[k].con, "{")==0)
                                    dbrack++;
                                k++;
                            }while(dbrack>0);
                            k--;
                        }
                        i=k;
                    }
                    else
                    {
                        k=i+1;
                        int dbrack=0;
                        do
                        {
                            if(strcmp(lines[k].con, "}")==0)
                                dbrack--;
                            if(strcmp(lines[k].con, "{")==0)
                                dbrack++;
                            k++;
                        }while(dbrack>0);
                        k--;
                        if(strcmp(lines[k+1].con, "else")==0)
                        {
                            k=k+2;
                            i=k+1;
                            dbrack=0;
                            do
                            {
                                if(strcmp(lines[k].con, "}")==0)
                                    dbrack--;
                                if(strcmp(lines[k].con, "{")==0)
                                    dbrack++;
                                k++;
                            }while(dbrack>0);
                            k--;
                            char hash[32];
                            function(i, k, start);
                        }
                        i=k;
                    }
                }
                i++;
                break;

                //Handle while statements
                case 4:
                {
                    char temp3[1024];
                    int res;
                    int gk=i+1;
                    int dbrack=0;
                    do
                    {
                        if(strcmp(lines[gk].con, "}")==0)
                            dbrack--;
                        if(strcmp(lines[gk].con, "{")==0)
                            dbrack++;
                        gk++;
                    }while(dbrack>0);
                    gk--;
                    do
                    {
                        int co=0;
                        int brack=1;
                        for(j=6;;j++)
                        {
                            if(ln[j]=='(')
                            {
                                temp2[co++]=ln[j];
                                brack++;
                            }
                            else if(ln[j]==')')
                            {
                                brack--;
                                if(brack<=0)
                                    break;
                                if(ln[j-1]==')')
                                {
                                    temp2[co++]=')';
                                    continue;
                                }
                                k=j-1;
                                for(;ln[k]!='(';k--);
                                k++;
                                int s=k;
                                for(;ln[k]!=')';k++)
                                    temp[k-s]=ln[k];
                                temp[k-s]=0;
                                temp2[co++]='0'+evaluate(temp, start);
                                temp2[co++]=')';
                            }
                            else if(ln[j]=='&'||ln[j]=='|')
                            {
                                temp2[co++]=ln[j]=='&'?'*':'+';
                                j++;
                            }
                            else ;
                        }
                        temp2[co-1]='\0';
                        rpn(temp2, temp3);
                        res=rpn_ev(temp3, start);
                        if(res)
                        {
                            char hash[32];
                            function(i+2, gk, start);
                        }
                    }while(res);
                    i=gk;
                }
                i++;
                break;

                // return() function: Exit from a current function
                case 5:
                {
                    for(j=7;ln[j]!=')';j++)
                        temp[j-7]=ln[j];
                    temp[j-7]=0;
                    if(is_var(temp, start)!=-1)
                    {
                        int xp=get_int_value(temp, start);
                        delete_var(start);
                        return xp;
                    }
                    else if(is_num(temp))
                    {
                        delete_var(start);
                        return num(temp);
                    }
                }

                // gets(...) function
                case 6:
                {
                    for(j=5;ln[j]!=')';j++)
                        temp[j-5]=ln[j];
                    temp[j-5]=0;
                    gets(temp2);
                    set_str_value(temp, temp2, start);
                }
                i++;
                break;

                //Handles all other custom fucntions defined in LINC program
                default:
                    for(j=0;ln[j]!='(';j++)
                        temp[j]=ln[j];
                    temp[j]=0;
                    int a, b;
                    if(find_function(temp, &a, &b))
                    {
                        variable *u=parameterize(ln, a-2, start);
                        function(a, b, u);
                    }
                    else
                    {
                        comp_err(4, temp);
                    }
                    i++;
                    break;
            }
        }
        else if(is_assignment(ln, start)!=-1)   // Is the current line an variable assignment ?
        {
            for(j=0;ln[j]!='['&&ln[j]!='=';j++)
                temp[j]=ln[j];
            temp[j]=0;
            int y=is_assignment(ln, start);     // What type of assignment ?
            switch(y)
            {
                case TINT:      // Assign int var a value
                {
                    for(j=0;ln[j]!='=';j++)
                        temp[j]=ln[j];
                    temp[j]=0;
                    j++;
                    k=j;
                    for(;ln[j];j++)
                        temp2[j-k]=ln[j];
                    temp2[j-k]=0;
                    set_int_value(temp, rpn_ev(temp2, start), start);
                }
                break;

                case TCH:      // Assign char var a value
                {
                    for(j=0;ln[j]!='=';j++)
                        temp[j]=ln[j];
                    temp[j]=0;
                    j+=2;
                    k=j;
                    for(;ln[j];j++)
                        temp2[j-k]=ln[j];
                    temp2[j-k]=0;
                    set_int_value(temp, rpn_ev(temp2, start), start);
                }
                break;

                case (TINT+TARR):      // Assign int array var element a value
                {
                    for(j=0;ln[j]!='=';j++)
                        temp[j]=ln[j];
                    temp[j]=0;
                    j++;
                    k=j;
                    for(;ln[j];j++)
                        temp2[j-k]=ln[j];
                    temp2[j-k]=0;
                    set_arr_value(temp, rpn_ev(temp2, start), start);
                }
                break;

                case (TCH+TARR):      // Assign char array var a value
                {
                    for(j=0;ln[j]!=':'&&ln[j]!='=';j++)
                        temp[j]=ln[j];
                    temp[j]=0;
                    if(ln[j]=='=')   // Assign the string a value
                    {
                        j++;
                        k=j;
                        for(;ln[j];j++)
                            temp2[j-k]=ln[j];
                        temp2[j-k]=0;
                        set_arr_value(temp, rpn_ev(temp2, start), start);
                    }
                    else            // Assign an element a value
                    {
                        char ctemp[1024*10]="";
                        int l=strlen(ln);
                        while(j<l)
                        {
                            j++;
                            int f=0;
                            if(ln[j]=='\"')
                            {
                                j++;
                                f=1;
                            }
                            k=j;
                            for(;ln[j]!='\"'&&ln[j]!='+';j++)
                                temp2[j-k]=ln[j];
                            temp2[j-k]=0;
                            if(f)
                            {
                                strcat(ctemp, temp2);
                                j++;
                            }
                            else
                            {
                                strcpy(temp3, "");
                                copy_str(temp2, temp3, start);
                                strcat(ctemp, temp3);
                            }
                        }
                        set_str_value(temp, ctemp, start);
                    }

                }
                break;
            }
            i++;
        }
        else if(is_defn(ln))    // Is the current line a variable declaration ?
        {
            int y=what_defn(ln);    // What type of variable is being defined ?
            switch(y)
            {
                case TINT:
                {
                    for(j=4;ln[j]&&!is_arith(ln[j]);j++)
                        temp[j-4]=ln[j];
                    temp[j-4]=0;
                    if(is_arith(ln[j]))
                    {
                        j++;
                        k=j;
                        for(;ln[j];j++)
                            temp2[j-k]=ln[j];
                        temp2[j-k]=0;
                        int num=rpn_ev(temp2, start);
                        new_var(temp, sizeof(int), y, &start);
                        set_int_value(temp, num, start);
                    }
                    else
                    {
                        int num=0;
                        new_var(temp, sizeof(int), y, &start);
                    }
                }
                break;

                case (TINT+TARR):
                {
                    for(j=4;ln[j]!=']';j++)
                        temp[j-4]=ln[j];
                    temp[j-4]=0;
                    rpn(temp, temp2);
                    int r=rpn_ev(temp2, start);
                    j+=2;
                    k=j;
                    for(;ln[j];j++)
                        temp[j-k]=ln[j];
                    temp[j-k]=0;
                    new_var(temp, sizeof(int)*r, y, &start);
                }
                break;

                case (TCH+TARR):
                {
                    for(j=5;ln[j]!=']';j++)
                        temp[j-5]=ln[j];
                    temp[j-5]=0;
                    rpn(temp, temp2);
                    int r=rpn_ev(temp2, start);
                    j+=2;
                    k=j;
                    for(;ln[j]&&ln[j]!=':';j++)
                        temp[j-k]=ln[j];
                    temp[j-k]=0;
                    new_var(temp, sizeof(char)*r, y, &start);
                    if(ln[j]==':')
                    {
                        char ctemp[1024*10]="";
                        int l=strlen(ln);
                        while(j<l)
                        {
                            j++;
                            int f=0;
                            if(ln[j]=='\"')
                            {
                                j++;
                                f=1;
                            }
                            k=j;
                            for(;ln[j]!='\"'&&ln[j]!='+';j++)
                                temp2[j-k]=ln[j];
                            temp2[j-k]=0;
                            if(f)
                            {
                                strcat(ctemp, temp2);
                                j++;
                            }
                            else
                            {
                                strcpy(temp3, "");
                                copy_str(temp2, temp3, start);
                                strcat(ctemp, temp3);
                            }
                        }
                        set_str_value(temp, ctemp, start);
                    }
                }
                break;

                case TCH:
                {
                    for(j=5;ln[j]&&!is_arith(ln[j]);j++)
                        temp[j-5]=ln[j];
                    temp[j-5]=0;
                    if(is_arith(ln[j]))
                    {
                        j++;
                        k=j;
                        for(;ln[j];j++)
                            temp2[j-k]=ln[j];
                        temp2[j-k]=0;
                        int num=rpn_ev(temp2, start);
                        new_var(temp, sizeof(char), y, &start);
                        set_char_value(temp, num, start);
                    }
                    else
                    {
                        int num=0;
                        new_var(temp, sizeof(char), y, &start);
                    }
                }
                break;
            }
            i++;
        }
    }
}

int main(int argc, char *argv[])
{
    build_symbols();                        // Build all the global variables
    int prog_lines;
    char *prog_code;
    int i, j, k;
    char temp[256], temp2[256];
    int flag, d;
    ifstream prog;

    // Open source file and read content to memory

    if(argc!=2)
    {
        int t=1;
        while(t)
        {
            printf("Enter linc source file name: ");
            gets(glo_name);
            ifstream tmp(glo_name);
            if(!tmp)
                printf("No such file: %s\n", glo_name);
            else
            {
                tmp.close();
                t=0;
            }
        }
    }
    else
        strcpy(glo_name, argv[1]);
    prog.open(glo_name);
    lines=new line[prog_lines=prog_line()];
    for(i=0;!prog.eof();i++)
    {
        printf("");
        prog.getline(lines[i].con, 256);
        if(strlen(lines[i].con)==0)
            i--;
    }
    prog.close();

    // Clean the code:
    //      1. Remove trailing tabs, spaces
    //      2. Remove spaces/tabs after special characters

    for(i=0;i<prog_lines;i++)
    {
        d=0;
        while((lines+i)->con[d]=='\t'||(lines+i)->con[d]==' ')
            d++;
        flag=0;
        for(j=d;(lines+i)->con[j];j++)
        {
            if((lines+i)->con[j]=='\"'||(lines+i)->con[j]=='\'')
            {
                flag=!flag;
                temp[j-d]=(lines+i)->con[j];
                continue;
            }
            if((((lines+i)->con[j-1]==' '||(lines+i)->con[j-1]==','||special_char ((lines+i)->con[j-1]))||((lines+i)->con[j+1]==' '||(lines+i)->con[j+1]==','||special_char ((lines+i)->con[j+1])))&&((lines+i)->con[j]==' '||(lines+i)->con[j]=='\t')&&flag==0)
                d++;
            else
            {
                    temp[j-d]=(lines+i)->con[j];
            }
        }
        temp[j-d]=0;
        strcpy((lines+i)->con, temp);
    }

    char printer[256], ln[256];
    int comp_flag=0;

    // Check if code is syntactically correct:
    //      1. Unbalanced brackets, quotes
    //      2. Invalid arithmetic expressions

    for(i=0;i<prog_lines;i++)
    {
        strcpy(ln, lines[i].con);
        if(is_function(ln))
        {
            if(is_syn_corr_func(ln))
            {
                comp_flag=1;
                printf("Line %d : In function %s\t%s\n", i+1, pre_func[what_function(ln)], error(is_syn_corr_func(ln)));
            }
        }
        else if(is_defn(ln))
        {
            int k;
            for(j=0;ln[j];j++)
            {
                if(ln[j]=='=')
                {
                    j++;
                    k=j;
                    for(;ln[j];j++)
                        temp[j-k]=ln[j];
                    temp[j-k]=0;
                    ln[k]=0;
                    rpn(temp, temp2);
                    strcat(ln, temp2);
                    strcpy(lines[i].con, ln);
                }
            }
        }
    }
    for(i=0;i<prog_lines;i++)
    {
        int q=0;
        int k=0;
        for(j=0;lines[i].con[j];j++)
        {
            if(lines[i].con[j]=='\'')
            {
                char t[10];
                sprintf(t, "%d", int(lines[i].con[j+1]));
                strcat(temp, t);
                k+=strlen(t);
                j+=3;
            }
            else
            {
                temp[k++]=lines[i].con[j];
                temp[k]=0;
            }
        }
    }
    if(comp_flag)
    {
        close(1);
        return 0;
    }

    // Index all the functions including <main>

    for(i=0;i<prog_lines;i++)
    {
        if(is_defn(lines[i].con)==4)
        {
            for(j=9;lines[i].con[j]!=' ';j++)
                temp2[j-9]=lines[i].con[j];
            temp2[j-9]=0;
            k=++j;
            for(;lines[i].con[j]!='(';j++)
                func[fc].name[j-k]=lines[i].con[j];
            func[fc].name[j-k]=0;
            int gk=i+1;
            int dbrack=0;
            do
            {
                if(strcmp(lines[gk].con, "}")==0)
                    dbrack--;
                if(strcmp(lines[gk].con, "{")==0)
                    dbrack++;
                gk++;
            }while(dbrack>0);
            gk--;
            func[fc].start=i+2;
            func[fc].end=i+gk;
            fc++;
        }
    }

    // Execute <main>
    int _st, _end;
    if(find_function("main", &_st,& _end))
        function(_st, _end, gstart);
    else
        comp_err(3, "");
}
