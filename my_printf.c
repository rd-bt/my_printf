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
	if(flag->addr){
		goto asaddr;
	}
	switch(flag->type){
		case EXPR_FLAGTYPE_DOUBLE:
			a->save[index].dbl=va_arg(*a->ap,double);
			break;
		case EXPR_FLAGTYPE_ADDR:
asaddr:
			a->save[index].addr=va_arg(*a->ap,void *);
			break;
		default:
			switch(flag->argsize){
				case 0:
				case sizeof(void *):
					a->save[index].sint=va_arg(*a->ap,intptr_t);
					break;
				case sizeof(int):
					a->save[index].sint=(intptr_t)va_arg(*a->ap,int);
					break;
				default:
					abort();
			}
			break;
	}
	++a->index_old;
	return a->save+index;
}
char wbuf[BUFSIZ];
struct expr_buffered_file printf_buf[1]={{
	.un.writer=(expr_writer)write,
	.fd=STDOUT_FILENO,
	.buf=wbuf,
	.index=0,
	.dynamic=0,
	.length=sizeof(wbuf),
}};
struct mapl2e {
	const char *libc;
	const char *expr;
};
const struct mapl2e maps[]={
	{"hh",":4"},
	{"h",":4"},
	{"",":4"},
#if __SIZEOF_LONG__==8
	{"l",":8"},
#else
	{"l",":4"},
#endif
	{"z",":8"},
	{"t",":8"},
	{"ll",":8"},
	{NULL},
};
size_t convert_from_libc_printf_style(const char *fmt,char *out){
	const char *c;
	size_t d,len;
	char *out0=out;
#define copy_once {*(out++)=*(fmt++);if(!*fmt)goto end;}
	for(;;){
		if(*fmt!='%'){
			copy_once;
			continue;
		}
		copy_once;
		while(memchr("+ -#0=?&i0123456789*.:",*fmt,22))
			copy_once;
		for(c=fmt;;++c){
			if(!*c)
				goto end;
			if(!memchr("hlzt",*c,4)&&expr_writefmts_table_default[(uint8_t)*c])
				break;
		}
		if(memchr("diouxXcfFgGeEaA",*c,15)){
			d=c-fmt;
			for(const struct mapl2e *p=maps;p->libc;++p){
				len=strlen(p->libc);
				if(len!=d)
					continue;
				if(d&&memcmp(p->libc,fmt,d))
					continue;
				fmt=c;
				out=stpcpy(out,p->expr);
				break;
			}
		}
		copy_once;
	}
end:
	*out=0;
	return out-out0;
}
int my_vprintf(const char *fmt,va_list ap){
	struct fmtarg a[1];
	int r;
	char *estyle=malloc(strlen(fmt)*2);
	size_t len=convert_from_libc_printf_style(fmt,estyle);
//	puts(estyle);
	a->ap=&ap;
	a->index_old=-1;
	r=(int)expr_vwritef(estyle,len,linebuf,(intptr_t)printf_buf,getarg,a);
	free(estyle);
	return r;
}
__attribute__((format(printf,1,2))) int my_printf(const char *fmt,...){
	va_list ap;
	int r;
	va_start(ap,fmt);
	r=my_vprintf(fmt,ap);
	va_end(ap);
	return r;
}
int main(int argc,char **argv){
//	size_t count;
//	int arr[13]={5,-467,9996,667,-9,79,376,665,36,95,6467,46466,357767};
	my_printf("e is %lf and -one is %d,two is %zu\n",M_E,-1,2ul);
//	count=5;
	my_printf("%e\n",M_E);
	my_printf("%g\n",M_E);
//	my_printf("%w%&zu\n%-1r%1k%+=.-1j",&count);
//	my_printf("%1T,%13.4k{%d}\n",arr);
//	my_printf("%.*h\n",sizeof(arr),arr);
	my_printf("the string is:%s\n","This is a string");
}
