#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SCHED_NORMAL 0
#define SCHED_FIFO 1
#define SCHED_RR 2
#define SCHED_WRR 6

FILE *file;
char inst[1000];

void TIME_cpu_calculation(int policy)
{
    int num_childproc;
    int calculated_quantity;
    for (calculated_quantity = 1 ; calculated_quantity <= 1000 ; calculated_quantity *=10)
    {
        //calculated_quantity = 2000;
    	for(num_childproc = 10; num_childproc <= 70; num_childproc += 5){
            memset(inst, sizeof(inst), 0);
            printf(" num_childproc= %d  calculated_quantity= %d\n", num_childproc, calculated_quantity);
            sprintf(inst, "time -p ./cpu_test %d %d %d", policy, calculated_quantity,num_childproc);
            file = popen(inst, "r");
            pclose(file);
        }
    }
    for (calculated_quantity = 2000 ; calculated_quantity <= 3000 ; calculated_quantity +=1000){
    	for(num_childproc = 10; num_childproc <= 70; num_childproc += 5){
            memset(inst, sizeof(inst), 0);
            printf(" num_childproc= %d  calculated_quantity= %d\n", num_childproc, calculated_quantity);
            sprintf(inst, "time -p ./cpu_test %d %d %d", policy, calculated_quantity,num_childproc);
            file = popen(inst, "r");
            pclose(file);
        }
    }
}
int main()
{
	printf("==========================CPU test results==========================\n");
	printf("@ NORMAL:\n");
    TIME_cpu_calculation(SCHED_NORMAL);
    printf("@ FIFO:\n");
    TIME_cpu_calculation(SCHED_FIFO);
    printf("@ RR:\n");
    TIME_cpu_calculation(SCHED_RR);
    printf("@ WRR:\n");
    TIME_cpu_calculation(SCHED_WRR);
    printf("====================================================================\n\n");
	return 0;
}