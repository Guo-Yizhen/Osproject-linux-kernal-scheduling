#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(){
    printf("=====================================test_start====================================\n");
    int pid,policy,priority;
    printf("Please input the PID you want to modify:");
    scanf("%d",&pid);

        printf("Please input the schedule policy you want to change to(0-Nomal,1-FIFO,2-RR,6-WRR):");
        scanf("%d",&policy);
        printf("Please input the priority you want to set:");
        scanf("%d",&priority);
        printf("Scheduler changing start for PID: %d \n",pid);
        int pre=sched_getscheduler(pid);
        struct sched_param param;
        param.sched_priority=priority;
        if(sched_setscheduler(pid,policy,&param)==-1){
            printf("ERROR:scheduler changing Failed!\n");
            printf("pid:%d,policy:%d",pid,policy);
            exit(1);
        }
        printf("Scheduler changing Successful!\n");
        printf("Previous scheduler: %d\n",pre);
    

        int cur=sched_getscheduler(pid);
        struct timespec ts;
        sched_rr_get_interval(pid,&ts);
        printf("Current scheduler: %d\ntime slice: %.2lf \n",cur,ts.tv_sec*1000.0+ts.tv_nsec/1000000.0);
    printf("======================================test_end=====================================\n");
}