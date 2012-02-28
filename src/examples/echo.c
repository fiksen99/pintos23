#include <stdio.h>
#include <syscall.h>

void dosomething(void);

int
main (int argc, char **argv)
{/*
  int i;

  for (i = 0; i < argc; i++)
    printf ("%s ", argv[i]);
  printf ("\n");
*///printf("success\n");
	dosomething();
  return EXIT_SUCCESS;
}

void
dosomething(void)
{
	printf("hello, world!\n");
}
