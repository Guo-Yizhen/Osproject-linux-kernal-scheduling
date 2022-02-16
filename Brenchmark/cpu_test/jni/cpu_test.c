#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define QUANTITY 100

long double calculation_task(int cq);

int main(int argc, char* argv[])
{
    srand(time(0));

    if(argc!=4){
        printf("ERROR: param not fit\n");
        exit(1);
    }

    int policy,num_childproc,cal_quantity;      //initalize the value
    policy = atoi(argv[1]);
    num_childproc = atoi(argv[2]);
    cal_quantity = atoi(argv[3]);

    int pid;

    struct sched_param param;
    param.sched_priority = sched_get_priority_max(policy);
    sched_setscheduler(0, policy, &param);                   //change its own scheduler

    int* child_pid = malloc(sizeof(int) * num_childproc);
    printf("Start forking children...\n");
    fflush(0); 

    int i;
    for (i = 0; i < num_childproc; i++)
    {
        pid = fork();
        if (pid > 0)
            child_pid[i] = pid; 
        else if (pid == 0) 
        {
            calculation_task(cal_quantity);
            _exit(0);
        } 
        else if (pid < 0) 
            exit(EXIT_FAILURE);
    }

    for (i = 0; i < num_childproc; i++)
        waitpid(child_pid[i], NULL, 0);

    free(child_pid);
}


long double calculation_task(int calculated_quantity){
    long double ans=1,total=0;
    long i;
    for(i = 0; i < calculated_quantity * 100; i++ ){
        ans= (random() / RAND_MAX) * (random() / RAND_MAX);
        total+= ans;
    }
    return total;
}