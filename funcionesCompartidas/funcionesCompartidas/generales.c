#include <stdarg.h>
#include <stdlib.h>

void liberador(int nptr, void *fst, ...){

	void *p;
	va_list ptrp;
	va_start(ptrp, fst);

	free(fst);
	for (nptr--; nptr > 0; nptr--){
		p = va_arg(ptrp, void*);
		free(p);
	}
}
