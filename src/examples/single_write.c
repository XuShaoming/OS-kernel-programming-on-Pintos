#include <syscall.h>

int
main (int argc, char **argv)
{
  char *buffer = "Hello world";
  write(1, buffer, 11);

  return EXIT_SUCCESS;
}
