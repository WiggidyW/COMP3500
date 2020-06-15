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

int main(){
  int pid;        /* Process ID                     */

  int *countptr;  /* pointer to the counter         */
  int *interested0;
  int *interested1;
  int *turn;

  int fd;     /* file descriptor to the file "containing" my counter */
  int interested0_fd;
  int interested1_fd;
  int turn_fd;
  int zero = 0; /* a dummy variable containing 0 */

  system("rm -f counter");
  system("rm -f interested0");
  system("rm -f interested1");
  system("rm -f turn");

  /* create a file which will "contain" my shared variable */
  fd = open("counter", O_RDWR | O_CREAT);
  interested0_fd = open("interested0", O_RDWR | O_CREAT);
  interested1_fd = open("interested1", O_RDWR | O_CREAT);
  turn_fd = open("turn", O_RDWR | O_CREAT);

  write(fd, &zero, sizeof(int));
  write(interested0_fd, &zero, sizeof(int));
  write(interested1_fd, &zero, sizeof(int));
  write(turn_fd, &zero, sizeof(int));

  /* map my file to memory */
  countptr = (int *) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  interested0 = (int *) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, interested0_fd, 0);
  interested1 = (int *) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, interested1_fd, 0);
  turn = (int *) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, turn_fd, 0);
 
  if (!countptr || !interested0 || !interested1 || !turn) {
    printf("Mapping failed\n");
    exit(1);
  }
  *turn = 0;
  *countptr = 0;
  *interested0 = 0;
  *interested1 = 0;

  close(fd);
  close(interested0_fd);
  close(interested1_fd);
  close(turn_fd);

  setbuf(stdout,NULL);

  pid = fork();
  if (pid < 0){
    printf("Unable to fork a process\n");
    exit(1);
  }

  if (pid == 0) {
    /* The child increments the counter by two's */
    while (1) {
      *interested0 = 1;
      *turn = 1;
      while (*interested1 == 1 && *turn == 1);
      printf("Child Entered!\n");
      if (*countptr < nloop) {
        add_n(countptr,2);
        printf("Child process -->> counter= %d\n",*countptr);
        *interested0 = 0;
      }
      else {
        *interested0 = 0;
        break;
      }
    }
    close(fd);
    close(interested0_fd);
    close(interested1_fd);
    close(turn_fd);
  }
  else {
    /* The parent increments the counter by twenty's */
    while (1) {
      *interested1 = 1;
      *turn = 0;
      while (*interested0 == 0 && *turn == 0);
      printf("Parent Entered!\n");
      if (*countptr < nloop) {
        add_n(countptr,20);
        printf("Parent process -->> counter = %d\n",*countptr);
        *interested1 = 0;
      }
      else {
        *interested1 = 0;
        break;
      }
    }
    close(fd);
    close(interested0_fd);
    close(interested1_fd);
    close(turn_fd);
  }
}









