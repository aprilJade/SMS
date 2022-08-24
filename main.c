#include <stdio.h>

void __attribute__((constructor)) a( void )
{
    printf("I miss you Lorthlorien ever beauty.\n");
}

void __attribute__((constructor)) b( void )
{
    printf("Bombadil, where have you been in the morning?\n");
}

void __attribute__((destructor)) c( void )
{
    printf("Mithlandir, help me!\n");
}

void main(void)
{
    printf("hello\n");
}