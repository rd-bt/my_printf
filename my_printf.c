/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <alloca.h>
#include "expr.h"
ssize_t linebuf(intptr_t fd,const void *buf,size_t size){
	return expr_buffered_write_flushat((struct expr_buffered_file *)fd,buf,size,'\n');
}
struct fmtarg {
	va_list *ap;
	ptrdiff_t index_old;
	union expr_argf save[64];
};
static const union expr_argf *getarg(ptrdiff_t index,const struct expr_writeflag *flag,void *addr){
	struct fmtarg *restrict a=addr;
	if(index==a->index_old)
		return a->save+index;
	if(index!=a->index_old+1){
		if((size_t)index<64&&a->index_old>=index){
			return a->save+index;
		}
		return NULL;
	}
	switch(flag->argsize){
		case 0:
		case 8:
			switch(flag->type){
				case EXPR_FLAGTYPE_DOUBLE:
					a->save[index].dbl=va_arg(*a->ap,double);
					break;
				default:
					a->save[index].addr=va_arg(*a->ap,void *);
					break;
			}
			break;
		case sizeof(int):
			a->save[index].uint=(uintptr_t)va_arg(*a->ap,unsigned int);
			break;
		default:
			abort();
	}
	++a->index_old;
	return a->save+index;
}
struct expr_buffered_file printf_buf[1]={{
	.un.writer=(expr_writer)write,
	.fd=STDOUT_FILENO,
	.buf=NULL,
	.index=0,
	.dynamic=BUFSIZ,
	.length=0,
}};
int my_vprintf(const char *fmt,va_list ap){
	struct fmtarg a[1];
	int r;
	a->ap=&ap;
	a->index_old=-1;
	r=(int)expr_vwritef(fmt,strlen(fmt),linebuf,(intptr_t)printf_buf,getarg,a);
	return r;
}
int my_printf(const char *fmt,...){
	va_list ap;
	int r;
	va_start(ap,fmt);
	r=my_vprintf(fmt,ap);
	va_end(ap);
	return r;
}
int main(int argc,char **argv){
	size_t count;
	int arr[13]={5,-467,9996,667,-9,79,376,665,36,95,6467,46466,357767};
	my_printf("e is %f and one is %id,two is %u\n",M_E,1,2ul);
	count=5;
	my_printf("%e\n",M_E);
	my_printf("%g\n",M_E);
	my_printf("%w%&u\n%-1r%1k%+#=.-1j",&count);
	my_printf("%1T,%13.4k{%d}\n",arr);
	my_printf("%.*h\n",sizeof(arr),arr);
	my_printf("the string is:%s\n","This is a string");
}
