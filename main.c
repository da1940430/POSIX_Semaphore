#include "semun.h"              /* Definition of semun union */
#include "svshm_xfr.h"
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>

void handler(int);
void* thread_write(void *);
void* thread_read(void *);

//global variable
int shmid;
struct shmseg *shmp;
pthread_t buff_writer, buff_reader;
sem_t *r_sem;
sem_t *w_sem;


int main(int argc, char *argv[]){
    struct sigaction sa;
    sa.sa_handler=handler;
    sa.sa_flags=0;
    sigaction(SIGINT, &sa, NULL);

    r_sem = sem_open("r_semaphore", O_CREAT, O_RDWR, 0);
    w_sem = sem_open("w_semaphore", O_CREAT, O_RDWR, 0);
    sem_post(r_sem);    //add 1
    sem_post(w_sem);

    if(r_sem ==SEM_FAILED || w_sem == SEM_FAILED)
        errExit("sem_open error");

    /* Create shared memory; attach at address chosen by system */

    shmid = shmget(SHM_KEY, sizeof(struct shmseg), IPC_CREAT | OBJ_PERMS);
    if (shmid == -1)
        errExit("shmget");

    shmp = shmat(shmid, NULL, 0);
    if (shmp == (void *) -1)
        errExit("shmat");
    shmp->cnt=-1;   //let cnt!=0, due to cnt==0 read_thread would break

    pthread_create(&buff_writer, NULL, thread_write, NULL);
    pthread_create(&buff_reader, NULL, thread_read, NULL);
    pthread_join(buff_writer,NULL);
    pthread_join(buff_reader,NULL);


    exit(EXIT_SUCCESS);
}

void handler(int sig){
    if(sig == SIGINT){
        if (shmdt(shmp) == -1)
            errExit("shmdt");
        if (shmctl(shmid, IPC_RMID, 0) == -1)
            errExit("shmctl");
        if(sem_close(r_sem) == -1){
            errExit("sem_close");
        }
        if(sem_close(w_sem) == -1){
            errExit("sem_close");
        }
        if(sem_unlink("r_semaphore") == -1){
            errExit("sem_unlink");
        }
        if(sem_unlink("w_semaphore") == -1){
            errExit("sem_unlink");
        }
    }
    exit(EXIT_SUCCESS);
}

void* thread_write(void *arg){
    int bytes, xfrs;

    for (xfrs = 0, bytes = 0; ; xfrs++, bytes += shmp->cnt) {
        if(sem_wait(w_sem)==-1){
            perror("thread_write() sem_wait error");
            pthread_exit(NULL);
        }
        shmp->cnt = read(STDIN_FILENO, shmp->buf, BUF_SIZE);
        if (shmp->cnt == -1)
            pthread_exit(NULL);

        if(sem_post(r_sem)==-1){
            perror("thread_write() sem_post error");
            pthread_exit(NULL);
        }
        if (shmp->cnt == 0)
            break;
    }
}

void* thread_read(void *arg){
    int bytes, xfrs;
    for(xfrs=0, bytes=0; ;xfrs++, bytes+=shmp->cnt){
        if(sem_wait(r_sem)==-1){
            perror("thread_read() sem_wait error");
            pthread_exit(NULL);
        }
        if (shmp->cnt == 0)                     /* Writer encountered EOF */
            break;
        bytes += shmp->cnt;
        if (write(STDOUT_FILENO, shmp->buf, shmp->cnt) != shmp->cnt)
            fatal("partial/failed write");

        if(sem_post(w_sem)==-1){
            perror("thread_read() sem_post error");
            pthread_exit(NULL);
        }
    }
}