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

  return 0;
}

int sprintf(char *out, const char *fmt, ...) {
  return 0;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  return 0;
}

#endif
