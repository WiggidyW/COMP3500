#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int nloop = 50;

/**********************************************************\
 * Function: increment a counter by some amount one by one *
 * argument: ptr (address of the counter), increment       *
 * output  : nothing                                       *
 **********************************************************/
void add_n(int *ptr, int increment){
  int i,j;
  for (i=0; i < increment; i++){
    *ptr = *ptr + 1;
    for (j=0; j < 1000000;j++);
  }
}

/**********************************************************\
 * Function: set value to non-atomic value atomically      *
 * argument: ptr (address of the value), n                 *
 * output  : nothing                                       *
 **********************************************************/
void set_n(int *ptr, int n) {
  *ptr = n;
  if (*ptr != n) {
    set_n(ptr, n);
  }
}

int main(){
  typedef int array[4];

  int pid;        /* Process ID                     */
  int fd;     /* file descriptor to the file "containing" my counter */
  array *countptr;  /* pointer to the counter         */
  array zero = {0, 0, 0, 0}; /* a dummy variable containing 0 */

  system("rm -f counter");

  /* create a file which will "contain" my shared variable */
  fd = open("counter",O_RDWR | O_CREAT);
  write(fd,&zero,sizeof(array));

  /* map my file to memory */
  countptr = (array *) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if (!countptr) {
    printf("Mapping failed\n");
    exit(1);
  }
  *countptr[0] = 0; // counter
  *countptr[1] = 0; // interested0
  *countptr[2] = 0; // interested1

  close(fd);

  setbuf(stdout,NULL);

  pid = fork();
  if (pid < 0){
    printf("Unable to fork a process\n");
    exit(1);
  }

  int counter; // value for storing counter intermittently

  if (pid == 0) {
    /* The child increments the counter by two's */
    while (1) {
      *countptr[1]/*interested0*/ = 1;
      set_n(countptr[3]/*turn*/, 1);
      if (
        (*countptr[1]/*interested0*/, *countptr[2]/*interested1*/) == (1, 0) ||
        (*countptr[1]/*interested0*/, *countptr[3]/*turn*/) == (1, 0)
      ) {
        counter = *countptr[0];
        if (counter < nloop) {
          add_n(&counter, 2);
          printf("Child process -->> counter= %d\n", counter);
          *countptr[0] = counter;
          *countptr[1]/*interested0*/ = 0;
        } else {
          *countptr[1]/*interested0*/ = 0;
          break;
        }
      }
    }
    close(fd);
  }
  else {
    /* The parent increments the counter by twenty's */
    while (1) {
      *countptr[2]/*interested1*/ = 1;
      set_n(countptr[3]/*turn*/, 0);
      if (
        (*countptr[2]/*interested1*/, *countptr[1]/*interested0*/) == (1, 0) ||
        (*countptr[2]/*interested1*/, *countptr[3]/*turn*/) == (1, 1)
      ) {
        counter = *countptr[0];
        if (counter < nloop) {
          add_n(&counter, 20);
          printf("Parent process -->> counter = %d\n", counter);
          *countptr[0] = counter;
          *countptr[2]/*interested1*/ = 0;
        } else {
          *countptr[2]/*interested1*/ = 0;
          break;
        }
      }
    }
    close(fd);
  }
}