#include "klib.h"
#include <stdarg.h>

void temp_output(int integer){
  if(integer < 0){
    _putc('-');
  }
  char putbuffer[11];
  int r = integer;
  int pointer = 0;
  do{
    putbuffer[pointer++] = r%10 + '0';
    r = r/10;
  }while(r != 0);
  pointer--;
  for(int i = pointer;i >= 0; i-- ){
    _putc(putbuffer[i]);
  }
  //_putc('\n');
} 


#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static int vsnprintf(char *out, size_t n, const char *fmt, va_list ap);
void printNum(int base, unsigned int num){
    if(base <= 0){
        return ;
    }
    char buffer[12];
        int pointer = 0;
        do{
            buffer[pointer++] = "0123456789abcdef"[num%base] ;
            num = num/base;
        }while(num != 0);
        pointer--;
        for(int i = pointer; i >= 0; i--){
            _putc(buffer[i]);
        }
}
void printDeci(int d){
    if(d < 0){
        d = -d;_putc('-');
    }
    
    printNum(10, d);
}

void printOct(unsigned int d){
    printNum(8,d);
}
void printHex(unsigned int d){
    printNum(16,d);
}
void printAddr(unsigned int d){
    _putc('0');
    _putc('x');
    printHex(d);
}

void printStr(char* str){
    while(*str != '\0'){
        _putc(*str);
        str ++;
    }
}


int printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
      
	char flag = 'f';
	for(int i = 0; fmt[i] != '\0'; ++i){
		if( flag == 'f'){
    		if(fmt[i] == '%'){
        	flag = 't';
      		}else{
            	_putc(fmt[i]);
        	}
        }else if(flag == 't'){
           	if(fmt[i] == 'd'){
                printDeci(va_arg(args, int));
			}else if(fmt[i] == 'x'){
                printHex(va_arg(args, unsigned int));
            }else if(fmt[i] == 'o'){
                printOct(va_arg(args, unsigned int));
            }else if(fmt[i]== 'p'){
                printAddr(va_arg(args, unsigned int));
            }else if(fmt[i] == 'c'){
                _putc(va_arg(args, unsigned int));
            }else if(fmt[i] == 's'){
                printStr(va_arg(args, char*));
            }else if(fmt[i] == '%'){
                _putc('%');
			}else{
                _putc('%');
                _putc(fmt[i]);
            }
            flag = 'f';
        }

    }

	return 0;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
    return vsnprintf(out,-1,fmt,ap);

}

int sprintf(char *out, const char *fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    int ret=vsnprintf(out,-1,fmt,ap);
    return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    int ret=vsnprintf(out,n,fmt,ap);
    return ret;
}

static inline int isalpha(char c){
    return (c>='a'&&c<='z')||
           (c>='A'&&c<='Z');
}


inline static int vsnprintf_real(char *out, size_t n, const char *fmt, va_list ap){
#define output(A) \
    if(cnt<n-1){ \
        out[cnt++]=A; \
    }else{ \
        va_end(ap); \
        out[cnt]='\0'; \
        return cnt; \
    }
    size_t cnt=0;
    int i=0;
    const char *p, *sval;
    char fill,num[10];
    int ival,fill_width;
    double dval;
    uintptr_t uval;
    char cval;
    for(p=fmt;*p!='\0';++p){
        if(*p!='%'){
            output(*p);
            continue;
        }
        fill_width=0;
        fill=' ';
        //do {
            if (*p == '0') {
            fill = '0';
            ++p;
            }
            fill_width = 0;
            ++p;
            while (*p >= '0' && *p <= '9') {
                fill_width *= 10;
                fill_width += *p - '0';
                ++p;
            }
            switch (*p) {
                case 'c':
                    cval=va_arg(ap,int);
                    output(cval);
                    break;
                case 'u':
                    uval = va_arg(ap, uint32_t);
                    i = 0;
                    while (uval > 0) {
                        num[i++] = uval % 10 + '0';
                        uval /= 10;
                    }
                    while (fill_width > i) {
                        num[i++] = fill;
                    }
                    if (i == 0) {
                        output('0');
                    } else {
                        while (i > 0) {
                            output(num[--i]);
                        }
                    }
                    break;
                case 'x':
                case 'p':
                    uval = (uintptr_t) va_arg(ap, void * );
                    i = 8;
                    while (i > 0) {
                        output(
                                (uval >> ((sizeof(void *) << 3) - 4)) < 10 ?
                                (uval >> ((sizeof(void *) << 3) - 4)) + '0' :
                                (uval >> ((sizeof(void *) << 3) - 4)) - 10 + 'a');
                        uval <<= 4;
                        --i;
                    }
                    break;
                case 'd':
                    ival = va_arg(ap, int);
                    if (ival < 0) {
                        output('-');
                        ival = -ival;
                    } while (ival > 0) {
                        num[i++] = ival % 10 + '0';
                        ival /= 10;
                    }
                    while (fill_width > i) {
                        num[i++] = fill;
                    }
                    if (i == 0) {
                        output('0');
                    } else {
                        while (i > 0) {
                            output(num[--i]);
                        }
                    }
                    break;
                case 'f':{
                    dval = va_arg(ap, double);
                    double d=1;
                    if(dval>1){
                        while(dval>d)d*=10;
                        d/=10;
                        while(d>0.9){
                            int j = (int)(dval / d);
                            output(j + '0');
                            dval -= j * d;
                            d /= 10;
                        }
                        output('.');
                    }
                    while (d > 0.001) {
                        int j = (int)(dval / d);
                        output(j + '0');
                        dval -= j * d;
                        d /= 10;
                    }
                    break;
                }
                case 's':
                    for (sval = va_arg(ap, char * ); *sval != '\0'; ++sval) {
                        output(*sval);
                    }
                    break;
                default:
                    if (isalpha(*p)) {
                        _putc(*p);
                        for(char* c="Not realized";*c;++c){
                            _putc(*c);
                        }
                        assert(0);
                    }else{
                        output(*p);
                    }
            }
        //}while(0);
    }
    va_end(ap);
    output('\0');
    return cnt-1;
#undef output
}
static int vsnprintf(char *out, size_t n, const char *fmt, va_list ap){
    /*
    static pthread_mutex_t io_lk=PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&io_lk);
    */
    int ret=vsnprintf_real(out,n,fmt,ap);
    /*
    pthread_mutex_unlock(&io_lk);
    */
    return ret;
}

#endif
