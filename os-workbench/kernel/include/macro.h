#ifndef __MACROS_H__
#define __MACROS_H__


#define winfo(string) \
    (string), strlen((string))

#define rinfo(string) \
    (string), sizeof((string))

#define WRAP(...) \
    do{ \
      __VA_ARGS__  \
    }while(0)

#define TODO() WRAP(assert(0);)
#define BARRIER() WRAP(assert(0);)

#define MACRO_VALUE(_arg)     #_arg
#define MACRO_SELF(_arg)      _arg
#define MACRO_CONCAT_REAL(_arg1,_arg2) _arg1 ## _arg2
#define MACRO_CONCAT(_arg1,_arg2) MACRO_CONCAT_REAL(_arg1,_arg2)
#define TO_STRING(_arg)     MACRO_VALUE(_arg)

//#define _LOCAL

#ifdef _LOCAL
    #define local_log(...) log(__VA_ARGS__)
#else
    #define local_log(...) 
#endif//_LOCAL

#define LEN(arr) ((sizeof(arr) / sizeof(arr[0])))

#define new(A) (typeof(A)*)pmm->alloc(sizeof(A))

#define min(a,b) ((a)>(b)?(b):(a))


    

#endif//__MACROS_H__