#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<stdbool.h>
//process control block (PCB)
struct pcb 
{
	unsigned int pid;
	char pname[20];
	unsigned int ptimeleft;
	unsigned int ptimearrival;
    bool pfirsttime; 
};

typedef struct pcb pcb;

//queue node
struct dlq_node 
{
	struct dlq_node *pfwd;
	struct dlq_node *pbck;
	struct pcb *data;
}; 

typedef struct dlq_node dlq_node;

//queue
struct dlq 
{
	struct dlq_node *head;
	struct dlq_node *tail;
};

typedef struct dlq dlq;
 
//function to add a pcb to a new queue node
dlq_node * get_new_node (pcb *ndata) 
{
	if (!ndata)
		return NULL;

	dlq_node *new = malloc(sizeof(dlq_node));
	if(!new)
    {
		fprintf(stderr, "Error: allocating memory\n");exit(1);
	}

	new->pfwd = new->pbck = NULL;
	new->data = ndata;
	return new;
}

//function to add a node to the tail of queue
void add_to_tail (dlq *q, dlq_node *new)
{
	if (!new)
		return;

	if (q->head==NULL)
    {
		if(q->tail!=NULL)
        {
			fprintf(stderr, "DLList inconsitent.\n"); exit(1);
		}
		q->head = new;
		q->tail = q->head;
	}
	else 
    {		
		new->pfwd = q->tail;
		new->pbck = NULL;
		new->pfwd->pbck = new;
		q->tail = new;
	}
}

//function to remove a node from the head of queue
dlq_node* remove_from_head(dlq * const q){
	if (q->head==NULL){ //empty
		if(q->tail!=NULL){fprintf(stderr, "DLList inconsitent.\n"); exit(1);}
		return NULL;
	}
	else if (q->head == q->tail) { //one element
		if (q->head->pbck!=NULL || q->tail->pfwd!=NULL) {
			fprintf(stderr, "DLList inconsitent.\n"); exit(1);
		}

		dlq_node *p = q->head;
		q->head = NULL;
		q->tail = NULL;
	
		p->pfwd = p->pbck = NULL;
		return p;
	}
	else { // normal
		dlq_node *p = q->head;
		q->head = q->head->pbck;
		q->head->pfwd = NULL;
	
		p->pfwd = p->pbck = NULL;
		return p;
	}
}

//function to print our queue
void print_q (const dlq *q) 
{
	dlq_node *n = q->head;
	if (n == NULL)
		return;

	while (n) 
    {
		printf("%s(%d),", n->data->pname, n->data->ptimeleft);
		n = n->pbck;
	}
}

//function to check if the queue is empty
int is_empty (const dlq *q)
{
	if (q->head == NULL && q->tail==NULL)
		return 1;
	else if (q->head != NULL && q->tail != NULL)
		return 0;
	else 
    {
		fprintf(stderr, "Error: DLL queue is inconsistent."); exit(1);
	}
}

//function to sort the queue on completion time
void sort_by_timetocompletion(const dlq *q) 
{ 
	// bubble sort
	dlq_node *start = q->tail;
	dlq_node *end = q->head;
	
	while (start != end) 
    {
		dlq_node *node = start;
		dlq_node *next = node->pfwd;

		while (next != NULL) 
        {
			if (node->data->ptimeleft < next->data->ptimeleft)
            {
				// do a swap
				pcb *temp = node->data;
				node->data = next->data;
				next->data = temp;
			}
			node = next;
			next = node->pfwd;
		}
		end = end ->pbck;
	} 
}

//function to sort the queue on arrival time
void sort_by_arrival_time (const dlq *q) 
{
	// bubble sort
	dlq_node *start = q->tail;
	dlq_node *end = q->head;
	
	while (start != end) 
    {
		dlq_node *node = start;
		dlq_node *next = node->pfwd;

		while (next != NULL) 
        {
			if (node->data->ptimearrival < next->data->ptimearrival)
            {
				// do a swap
				pcb *temp = node->data;
				node->data = next->data;
				next->data = temp;
			}
			node = next;
			next = node->pfwd;
		}
		end = end->pbck;
	}
}

//function to tokenize the one row of data
pcb* tokenize_pdata (char *buf) 
{
	pcb* p = (pcb*) malloc(sizeof(pcb));
	if(!p)
    { 
        fprintf(stderr, "Error: allocating memory.\n"); exit(1);
    }

	char *token = strtok(buf, ":\n");
	if(!token)
    { 
        fprintf(stderr, "Error: Expecting token pname\n"); exit(1);
    }  
	strcpy(p->pname, token);

	token = strtok(NULL, ":\n");
	if(!token)
    { 
        fprintf(stderr, "Error: Expecting token pid\n"); exit(1);
    }  
	p->pid = atoi(token);

	token = strtok(NULL, ":\n");
	if(!token)
    { 
        fprintf(stderr, "Error: Expecting token duration\n"); exit(1);
    } 
	 
	p->ptimeleft= atoi(token);

	token = strtok(NULL, ":\n");
	if(!token)
    { 
        fprintf(stderr, "Error: Expecting token arrival time\n"); exit(1);
    }  
	p->ptimearrival = atoi(token);
    p->pfirsttime = true;

	token = strtok(NULL, ":\n");
	if(token)
    { 
        fprintf(stderr, "Error: Oh, what've you got at the end of the line?\n");exit(1);
    } 

	return p;
}

void sched_FIFO(dlq *const p_fq, int *p_time)
{
    int NoOfProcesses = 0; //To keep a check for the no of processes for our metrics
    double Throughput = 0; //Throughput initialization
    double AverageTurnaroundTime = 0; //Average Turnaround Time initialization
    double TotalTurnaroundTime = 0;  //Total Turnaround Time initialization
    double AverageResponseTime = 0;  //Average Response Time initialization
    double TotalResponseTime = 0;   //Total Response Time initialization
    struct pcb *Current = NULL;    //Current process
    dlq ReadyQueue;               //Queue where the prcess arrives and waits for its turn 
    ReadyQueue.head = NULL;
    ReadyQueue.tail = NULL;
    dlq_node *head = p_fq->head;  //Head of the queue to keep track of moving from one process to another
    int EndTime = 0;              // Used this for calculating time for the whole schdeule
    int size = 0;      

    dlq_node *n = p_fq->head;    //Used this to calculate end time
    EndTime = n->data->ptimearrival;
    if (n == NULL)
        return;
    while (n) 
    {
        size = size + 1;
        EndTime = EndTime + n->data->ptimeleft;
        n = n->pbck;
    }                             //Now we have the end time for our schedule
    for(int i = 1; i<EndTime + 1; i++){     //Looping through the whole schedule using the endtime
        if(Current!=NULL){                  //If the current process is not null
            *(p_time) = *(p_time) + 1;      //Incrementing the time
            if(Current->ptimeleft == 0){    //If the time left for the current process is 0
                if(ReadyQueue.head!=NULL){  //If the ready queue is not empty
                    Current=ReadyQueue.head->data; //Current process becomes the head of the ready queue
                    NoOfProcesses = NoOfProcesses+1; //Incrementing the no of processes
                if(Current->pfirsttime == true){     //This is a check to calculate response time metric
                        Current->pfirsttime = false;
                        TotalResponseTime = (double) TotalResponseTime + (*(p_time) - Current->ptimearrival) - 1;
                        printf("%s%s%f\n",Current->pname," Response Time = ", TotalResponseTime);
                        TotalResponseTime = 0;
                }
                    remove_from_head(&ReadyQueue);     //Removing the head of the ready queue since it has now become current
                } 
            } 

            if(head->data->ptimearrival==i-1)          //if the head, which is at the head of p_fq, becomes equal to the current sys time
            {
                add_to_tail(&ReadyQueue, get_new_node(head->data)); //Adding the head of p_fq to the tail of the ready queue
                remove_from_head(p_fq);                            //Removing the head of p_fq
                if(is_empty(p_fq)!=1){                            //If p_fq is not empty
                    head = p_fq->head;                           //Head moves to the next element of p_fq as we removed head
                }
                printf("%d:",*p_time);
                printf("%s(%d):",Current->pname,Current->ptimeleft);
                print_q(&ReadyQueue);
                printf("%s",":");
                printf("\n");  
            }
            else{
                printf("%d:",*p_time);
                printf("%s(%d):",Current->pname,Current->ptimeleft);
                if(is_empty(&ReadyQueue)==1){
                    printf("%s\n","empty:");
                }
                else{
                    print_q(&ReadyQueue);
                    printf("%s",":");
                    printf("\n"); 
                }          
            }
            if(Current->ptimeleft == 1){  //This is the check for turnaround time, this is where the execution time becomes zero so we need it for our calculation
            TotalTurnaroundTime = (double) TotalTurnaroundTime + (*(p_time) - Current->ptimearrival);
            }
            Current->ptimeleft = Current->ptimeleft - 1; 
        }
        else{ //If the current process is null
            *(p_time) = *(p_time) + 1; //Incrementing the time
            if(head->data->ptimearrival==i){ //If the process at the head of p_fq's time arrival becomes equal to the current time
                Current = head->data; //Current process becomes the head of p_fq
                NoOfProcesses = NoOfProcesses+1; //Incrementing the no of processes
                remove_from_head(p_fq);
                head = p_fq->head;
                printf("%d:",*p_time);
                printf("idle:");
                printf("%s\n","empty:");
                if(Current->pfirsttime == true){ //This is a check to calculate response time metric
                        Current->pfirsttime = false;
                        TotalResponseTime = 0;
                        printf("%s%s%f\n",Current->pname," Response Time = ", TotalResponseTime);
                        TotalResponseTime = 0;
                }
            }
            else if(head->data->ptimearrival==0){ //Condtion to check when the process is arriving at time 0
                Current = head->data;
            if(Current->pfirsttime == true){ //This is a check to calculate response time metric
                        Current->pfirsttime = false;
                        TotalResponseTime = 0;
                        printf("%s%s%f\n",Current->pname," Response Time = ", TotalResponseTime);
                        TotalResponseTime = 0;
                }
                remove_from_head(p_fq);
                NoOfProcesses = NoOfProcesses+1;
                head = p_fq->head;
                printf("%d:",*p_time);
                printf("%s:",Current->pname);
                printf("%s\n","empty:");
                Current->ptimeleft = Current->ptimeleft - 1;
            }
            else{
                printf("%d:",*p_time);
                printf("idle:");
                printf("%s\n","empty:");
            }  
        }
    }
    //Metrics Calculation
    Throughput = (double)NoOfProcesses/EndTime;
    AverageTurnaroundTime = (double) TotalTurnaroundTime/NoOfProcesses;
    AverageResponseTime = (double) TotalResponseTime/NoOfProcesses;
    printf("%s%d\n","No Of Processess = ", NoOfProcesses);
    printf("%s%f\n","Throughput = ", Throughput);
    printf("%s%f\n","Turnaround Time = ", AverageTurnaroundTime);
    printf("%s%f","Response Time = ", AverageResponseTime);
}

void sched_SJF(dlq *const p_fq, int *p_time)
{
    int NoOfProcesses = 0;
    double Throughput = 0;
    double AverageTurnaroundTime = 0; 
    double TotalTurnaroundTime = 0;
    double AverageResponseTime = 0;
    double TotalResponseTime = 0; 
    struct pcb *Current = NULL;
    dlq ReadyQueue;
    ReadyQueue.head = NULL;
    ReadyQueue.tail = NULL;
    dlq_node *head = p_fq->head;
    int EndTime = 0;
    int size = 0;
    dlq_node *n = p_fq->head;
    EndTime = n->data->ptimearrival;
    if (n == NULL)
        return;
    while (n) 
    {
        size = size + 1;
        EndTime = EndTime + n->data->ptimeleft;
        n = n->pbck;
    }
    for(int i = 1; i<EndTime + 1; i++){
        if(Current!=NULL){
            *(p_time) = *(p_time) + 1; 
            if(Current->ptimeleft == 0){
                if(ReadyQueue.head!=NULL){
                    Current=ReadyQueue.head->data;
                    NoOfProcesses = NoOfProcesses+1;
                    if(Current->pfirsttime == true){
                        Current->pfirsttime = false;
                        TotalResponseTime = (double) TotalResponseTime + (*(p_time) - Current->ptimearrival) - 1;
                        // printf("%s%f\n","Response Time = ", TotalResponseTime);
                        // TotalResponseTime =0;
                    }
                    remove_from_head(&ReadyQueue);
                }
            } 
            if(head->data->ptimearrival==i-1)
            {
                add_to_tail(&ReadyQueue, get_new_node(head->data));
                sort_by_timetocompletion(&ReadyQueue); //Everything is same as FIFO except that where we have added to the tail in Ready Queue, we sort it by time to completion
                remove_from_head(p_fq);
                if(is_empty(p_fq)!=1){
                    head = p_fq->head;
                }
                printf("%d:",*p_time);
                printf("%s:",Current->pname);
                print_q(&ReadyQueue);
                printf("%s",":");
                printf("\n");  
            }
            else{
                printf("%d:",*p_time);
                printf("%s:",Current->pname);
                if(is_empty(&ReadyQueue)==1){
                    printf("%s\n","empty:");
                }
                else{
                    print_q(&ReadyQueue);
                    printf("%s",":");
                    printf("\n"); 
                }          
            }
            if(Current->ptimeleft == 1){
                TotalTurnaroundTime = (double) TotalTurnaroundTime + (*(p_time) - Current->ptimearrival);
            }
            Current->ptimeleft = Current->ptimeleft - 1; 
        }
        else{
            *(p_time) = *(p_time) + 1;
            if(head->data->ptimearrival==i){
                Current = head->data;
                NoOfProcesses = NoOfProcesses+1;
                if(Current->pfirsttime == true){
                    Current->pfirsttime = false;
                    TotalResponseTime = 0;
                    // printf("%s%f\n","Response Time = ", TotalResponseTime);
                    // TotalResponseTime = 0;
                }
                remove_from_head(p_fq);
                head = p_fq->head;
                printf("%d:",*p_time);
                printf("idle:");
                printf("%s\n","empty:");
            }
            else if(head->data->ptimearrival==0){
                Current = head->data;
                NoOfProcesses = NoOfProcesses+1;
                if(Current->pfirsttime == true){
                    Current->pfirsttime = false;
                    TotalResponseTime = 0;
                    printf("%s%f\n","Response Time = ", TotalResponseTime);
                }
                remove_from_head(p_fq);
                head = p_fq->head;
                printf("%d:",*p_time);
                printf("%s:",Current->pname);
                printf("%s\n","empty:");
                Current->ptimeleft = Current->ptimeleft - 1;
            }
            else{
                printf("%d:",*p_time);
                printf("idle:");
                printf("%s\n","empty:");
            }  
        }
    }
    Throughput = (double)NoOfProcesses/EndTime;
    AverageTurnaroundTime = (double) TotalTurnaroundTime/NoOfProcesses;
    AverageResponseTime = (double) TotalResponseTime/NoOfProcesses;
    printf("%s%f\n","Throughput = ", Throughput);
    printf("%s%f\n","Turnaround Time = ", AverageTurnaroundTime);
    printf("%s%f","Response Time = ", AverageResponseTime);
}

void sched_STCF(dlq *const p_fq, int *p_time){
    int NoOfProcesses = 0;
    double Throughput = 0;
    double AverageTurnaroundTime = 0; 
    double TotalTurnaroundTime = 0;
    double AverageResponseTime = 0;
    double TotalResponseTime = 0; 
    struct pcb *Current = NULL;
    dlq ReadyQueue;
    ReadyQueue.head = NULL;
    ReadyQueue.tail = NULL;
    dlq_node *head = p_fq->head;
    int EndTime = 0;
    int size = 0;
    dlq_node *n = p_fq->head;
    EndTime = n->data->ptimearrival;
    if (n == NULL)
        return;
    while (n) 
    {
        size = size + 1;
        EndTime = EndTime + n->data->ptimeleft;
        n = n->pbck;
    }
    for(int i = 1; i<EndTime + 1; i++){
        if(Current!=NULL){
            *(p_time) = *(p_time) + 1; 
            if(Current->ptimeleft == 0){
                if(ReadyQueue.head!=NULL){
                    Current=ReadyQueue.head->data;
                    NoOfProcesses = NoOfProcesses+1;
                    if(Current->pfirsttime == true){
                        Current->pfirsttime = false;
                        TotalResponseTime = (double) TotalResponseTime + (*(p_time) - Current->ptimearrival) - 1;
                        // printf("%s%f\n","Response Time = ", TotalResponseTime);
                        // TotalResponseTime = 0;
                    }
                    remove_from_head(&ReadyQueue);
                }
            } 
            if(head->data->ptimearrival==i-1)
            {
                 //The only difference between SJF and STCF code is that we have added a check for comparing, if less, Current is replaced, if not head gets added to the tail of ready queue
                if(head->data->ptimeleft<Current->ptimeleft){
                    add_to_tail(&ReadyQueue, get_new_node(Current));
                    sort_by_timetocompletion(&ReadyQueue);
                    Current = head->data;
                    if(Current->pfirsttime == true){
                        Current->pfirsttime = false;
                        TotalResponseTime = (double) TotalResponseTime + (*(p_time) - Current->ptimearrival) - 1;
                        // printf("%s%f\n","Response Time = ", TotalResponseTime);
                        //TotalResponseTime = 0;
                    }
                    remove_from_head(p_fq);
                    if(is_empty(p_fq)!=1){
                        head = p_fq->head;
                    }
                }
                else{
                    add_to_tail(&ReadyQueue, get_new_node(head->data));
                    sort_by_timetocompletion(&ReadyQueue);
                    remove_from_head(p_fq);
                    if(is_empty(p_fq)!=1){
                        head = p_fq->head;
                    }
                } 

                printf("%d:",*p_time);
                printf("%s:",Current->pname);
                print_q(&ReadyQueue);
                printf("%s",":");
                printf("\n");  
            }
            else{
                printf("%d:",*p_time);
                printf("%s:",Current->pname);
                if(is_empty(&ReadyQueue)==1){
                    printf("%s\n","empty:");
                }
                else{
                    print_q(&ReadyQueue);
                    printf("%s",":");
                    printf("\n"); 
                }          
            }
            if(Current->ptimeleft == 1){
                TotalTurnaroundTime = (double) TotalTurnaroundTime + (*(p_time) - Current->ptimearrival);
            }
            Current->ptimeleft = Current->ptimeleft - 1; 
        }
        else{
            *(p_time) = *(p_time) + 1;
            if(head->data->ptimearrival==i){
                Current = head->data;
                remove_from_head(p_fq);
                head = p_fq->head;
                printf("%d:",*p_time);
                printf("idle:");
                printf("%s\n","empty:");
                NoOfProcesses = NoOfProcesses+1;
                if(Current->pfirsttime == true){
                    Current->pfirsttime = false;
                    TotalResponseTime = 0;
                    // printf("%s%f\n","Response Time = ", TotalResponseTime);
                    //TotalResponseTime = 0;
                }
            }
            else if(head->data->ptimearrival==0){
                Current = head->data;
                NoOfProcesses = NoOfProcesses+1;
                if(Current->pfirsttime == true){
                    Current->pfirsttime = false;
                    TotalResponseTime = 0;
                    // printf("%s%f\n","Response Time = ", TotalResponseTime);
                    //TotalResponseTime = 0;
                }
                remove_from_head(p_fq);
                head = p_fq->head;
                printf("%d:",*p_time);
                printf("%s:",Current->pname);
                printf("%s\n","empty:");
                Current->ptimeleft = Current->ptimeleft - 1;
            }
            else{
                printf("%d:",*p_time);
                printf("idle:");
                printf("%s\n","empty:");
            }  
        }
    }
    Throughput = (double)NoOfProcesses/EndTime;
    AverageTurnaroundTime = (double) TotalTurnaroundTime/NoOfProcesses;
    AverageResponseTime = (double) TotalResponseTime/NoOfProcesses;
    printf("%s%d\n","No Of Processess = ", NoOfProcesses);
    printf("%s%f\n","Throughput = ", Throughput);
    printf("%s%f\n","Turnaround Time = ", AverageTurnaroundTime);
    printf("%s%f","Response Time = ", AverageResponseTime);
}

void sched_RR(dlq *const p_fq, int *p_time)
{
    int NoOfProcesses = 0;
    double Throughput = 0;
    double AverageTurnaroundTime = 0; 
    double TotalTurnaroundTime = 0;
    double AverageResponseTime = 0;
    double TotalResponseTime = 0; 
    struct pcb *Current = NULL;
    dlq ReadyQueue;
    ReadyQueue.head = NULL;
    ReadyQueue.tail = NULL;
    dlq_node *head = p_fq->head;
    int EndTime = 0;
    int size = 0;
    dlq_node *n = p_fq->head;
    EndTime = n->data->ptimearrival;
    if (n == NULL)
        return;
    while (n) 
    {
        size = size + 1;
        EndTime = EndTime + n->data->ptimeleft;
        n = n->pbck;
    }
    for(int i = 1; i<EndTime + 1; i++){
        if(Current!=NULL){
            *(p_time) = *(p_time) + 1; 
            if(Current->ptimeleft == 0){
                if(ReadyQueue.head!=NULL){
                    Current=ReadyQueue.head->data;
                NoOfProcesses = NoOfProcesses+1;
                if(Current->pfirsttime == true){
                        Current->pfirsttime = false;
                        TotalResponseTime = (double) TotalResponseTime + (*(p_time) - Current->ptimearrival) - 1;
                }
                    remove_from_head(&ReadyQueue);
                }
            }
            else{ //This is the difference in code b/w FIFO and RR where we have added a check to shift between the current process and the process at the head of ReadyQueue
                if(is_empty(&ReadyQueue)!=1){
                    add_to_tail(&ReadyQueue, get_new_node(Current));
                    Current = ReadyQueue.head->data;
                    
                if(Current->pfirsttime == true){
                        Current->pfirsttime = false;
                        TotalResponseTime = (double) TotalResponseTime + (*(p_time) - Current->ptimearrival) - 1;
                        // printf("%s%f\n","Response Time = ", TotalResponseTime);
                        //TotalResponseTime = 0;
                }
                    remove_from_head(&ReadyQueue);
                }
            } 

            if(head->data->ptimearrival==i-1)
            {
                add_to_tail(&ReadyQueue, get_new_node(head->data));
                remove_from_head(p_fq);
                if(is_empty(p_fq)!=1){
                    head = p_fq->head;
                }
                printf("%d:",*p_time);
                printf("%s:",Current->pname);
                print_q(&ReadyQueue);
                printf("%s",":");
                printf("\n");  
            }
            else{
                printf("%d:",*p_time);
                printf("%s:",Current->pname);
                if(is_empty(&ReadyQueue)==1){
                    printf("%s\n","empty:");
                }
                else{
                    print_q(&ReadyQueue);
                    printf("%s",":");
                    printf("\n"); 
                }          
            }
            if(Current->ptimeleft == 1){
            TotalTurnaroundTime = (double) TotalTurnaroundTime + (*(p_time) - Current->ptimearrival);
            }
            Current->ptimeleft = Current->ptimeleft - 1; 
        }
        else{
            *(p_time) = *(p_time) + 1;
            if(head->data->ptimearrival==i){
                Current = head->data;
                NoOfProcesses = NoOfProcesses+1;
                if(Current->pfirsttime == true){
                        Current->pfirsttime = false;
                        TotalResponseTime = 0;
                }
                remove_from_head(p_fq);
                head = p_fq->head;
                printf("%d:",*p_time);
                printf("idle:");
                printf("%s\n","empty:");
                
            }
            else if(head->data->ptimearrival==0){
                Current = head->data;
                NoOfProcesses = NoOfProcesses+1;
                if(Current->pfirsttime == true){
                        Current->pfirsttime = false;
                        TotalResponseTime = 0;
                        // printf("%s%f\n","Response Time = ", TotalResponseTime);
                        //TotalResponseTime = 0;
                }
                remove_from_head(p_fq);
                head = p_fq->head;
                printf("%d:",*p_time);
                printf("%s:",Current->pname);
                printf("%s\n","empty:");
                Current->ptimeleft = Current->ptimeleft - 1;
            }
            else{
                printf("%d:",*p_time);
                printf("idle:");
                printf("%s\n","empty:");
            }  
        }
    }
    Throughput = (double)NoOfProcesses/EndTime;
    AverageTurnaroundTime = (double) TotalTurnaroundTime/NoOfProcesses;
    AverageResponseTime = (double) TotalResponseTime/NoOfProcesses;
    printf("%s%d\n","NOP = ", NoOfProcesses);
    printf("%s%f\n","Throughput = ", Throughput);
    printf("%s%f\n","Turnaround Time = ", AverageTurnaroundTime);
    printf("%s%f","Response Time = ", AverageResponseTime);
}

int main()
{
    /* Enter your code here. Read input from STDIN. Print output to STDOUT */
    int N = 0;
    char tech[20]={'\0'};
    char buffer[100]={'\0'};
    scanf("%d", &N);
    scanf("%s", tech);
     
    dlq queue;
    queue.head = NULL;
    queue.tail = NULL;
    for(int i=0; i<N; ++i)
    {   
        scanf("%s\n", buffer); 
        pcb *p = tokenize_pdata(buffer);
        add_to_tail (&queue, get_new_node(p) );  
    }
    //print_q(&queue);
    unsigned int system_time = 0;
    sort_by_arrival_time (&queue);
    //print_q (&queue);
    
    // run scheduler
    if(!strncmp(tech,"FIFO",4))
        sched_FIFO(&queue, &system_time);
    else if(!strncmp(tech,"SJF",3))
        sched_SJF(&queue, &system_time);
    else if(!strncmp(tech,"STCF",4))
        sched_STCF(&queue, &system_time);
    else if(!strncmp(tech,"RR",2))
        sched_RR(&queue, &system_time);
    else
        fprintf(stderr, "Error: unknown POLICY\n");
    return 0;
}