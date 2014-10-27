#include<string.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<math.h>
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include<fcntl.h>
#include<pthread.h>
#include<semaphore.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <time.h>

sem_t semaphore1,sem_delivery;

#define MAX_BUFFER 50 
#define MY_PORT_NUM 3334
#define CLOCK_PORT 3335
#define LOCALTIME_STREAM 0
#define GMT_STREAM 1 


typedef struct
{
int msg_id;
char message_content[100];
char dest_ip[50][13];
char msg_Status[20];
int msg_Source;
int dest_counter;
int current_clock;
int msg_Type;
int max_clk;
}type_msg;

type_msg *msgStruct[500];

typedef struct node_s {
        type_msg msg_Struct; 
	struct node_s *next;
} NODE;

NODE *list_create(type_msg *temp)
{
        NODE *node;
        //if(!(node=malloc(sizeof(NODE)))) return NULL;
        node=(NODE *)malloc(sizeof(NODE));
        memcpy((void *)&(node->msg_Struct),temp,sizeof(type_msg));

        node->next=NULL;
        return node;
}


NODE *find_node(int number, NODE *head)
{

NODE *temp_node = head; 

while((temp_node->next!=NULL)||(temp_node->msg_Struct.msg_id!=number))
{
   temp_node= temp_node->next;
}

if((temp_node->next!=NULL))
 printf(" SUCCESS NODE FOUND \n");
return temp_node; 

}

int list_remove(NODE *list, NODE *node)
{
        while(list->next && list->next!=node) list=list->next;
        if(list->next) {
                list->next=node->next;
                free(node);
                return 1;
        } else return 0;
}

int addNodeBottom(type_msg *temp, NODE *head)
{
    NODE *current = head;
    NODE *newNode = (NODE *) malloc(sizeof(NODE));
    if (newNode == NULL) {
        printf("malloc failed\n");
        exit(-1);
    }

    memcpy((void *)&(newNode->msg_Struct),temp,sizeof(type_msg));

    newNode->next = NULL;

    while (current->next) {
        current = current->next;
    }
    current->next = newNode;
    return 0;
}

NODE *addNodeTop(type_msg *temp, NODE *head) {
  NODE *current = head;
    NODE *newNode = (NODE *) malloc(sizeof(NODE));
    if (newNode == NULL) {
        printf("malloc failed\n");
        exit(-1);
    }

    memcpy((void *)&(newNode->msg_Struct),temp,sizeof(type_msg));
    newNode->next = head;

    return newNode;
}

/*
void print_freelist(NODE *temp, int c)
{
    int i=0;
  do
    {   for(i=0i<c;i++)
           printf("< %d >",temp->dest_list[i]);
        printf("->");
        if(temp->next!=NULL)
         temp=temp->next;


    }while(temp->next!=NULL);
   printf("%d ",temp->frame_no);
    printf("\n");


}*/



int global_clock=0;
sem_t sem_clock;


char *fgets_wrapper(char *buffer, size_t buflen, FILE *fp)
{
    if (fgets(buffer, buflen, fp) != 0)
    {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n')
            buffer[len-1] = '\0';
        return buffer;
    }
    return 0;
}



#define MAX_PROC 100
#define MAX_DEST 50 

int max_clock_array[MAX_PROC][MAX_DEST];

NODE *temp_head;

/******************************** CLOCK_THREAD THREAD START*************************/
	void *clock_thread(void * arg)
	{
	
		int ret, listenSock, clockSock,in, flags=0;
                struct sockaddr_in servaddr1;
                struct sctp_sndrcvinfo sndrcvinfo;
                struct sctp_event_subscribe events;
		
		type_msg *temp_msg_struct = (type_msg *) malloc(sizeof(type_msg)); 


		/* Create SCTP TCP-Style Socket */
                listenSock = socket( AF_INET, SOCK_STREAM, IPPROTO_SCTP );

                /* Accept connections from any interface */
                bzero( (void *)&servaddr1, sizeof(servaddr1) );
                servaddr1.sin_family = AF_INET;
                servaddr1.sin_addr.s_addr = htonl( INADDR_ANY );
                servaddr1.sin_port = htons(CLOCK_PORT);

                /* Bind to the wildcard address (all) and MY_PORT_NUM */
                ret = bind( listenSock,(struct sockaddr *)&servaddr1, sizeof(servaddr1) );

                /* Place the server socket into the listening state */
                listen( listenSock, 200 );

		while(1)
		{
		  clockSock = accept(listenSock,(struct sockaddr *)NULL, (int *)NULL);

 		   /* Enable receipt of SCTP Snd/Rcv Data via sctp_recvmsg */
                    memset( (void *)&events, 0, sizeof(events) );
                    events.sctp_data_io_event = 1;
                    setsockopt( clockSock, SOL_SCTP, SCTP_EVENTS,(const void *)&events, sizeof(events) );

                    in = sctp_recvmsg( clockSock, (void *)temp_msg_struct, sizeof(type_msg),(struct sockaddr *)NULL, 0,&sndrcvinfo, &flags );
		      
		     NODE *search_node= find_node(temp_msg_struct->msg_id, temp_head); 

		     sem_wait(&sem_delivery);
		     strncpy(search_node->msg_Struct.msg_Status, "DELIVERABLE", sizeof("DELIVERABLE"));   
		     sem_post(&sem_delivery);

		     printf(" MESSAGE ID %d STATUS MODIFIED TO DELIVERABLE \n", search_node->msg_Struct.msg_id);


		   NODE *x= temp_head;

		   /*while(x->next!=NULL)
		   {
			x= x->next;
			printf(" MESSAGE ID %d FINAL TIME STAMP : %d STATUS %s \n", x->msg_Struct.msg_id, temp_msg_struct->max_clk,x->msg_Struct.msg_Status );
		   }*/
                  printf("\n --------------------------------------------------- \n");
  		    printf(" MESSAGE RECEIVED BY CLOCK_THREAD HAS %d BYTES RECEIVED PROPOSED TIME STAMP IS %d\n", in, temp_msg_struct->max_clk);
		    
		   if(temp_msg_struct->msg_Type==3)
		     {
		     sem_post(&sem_clock);
		      if(global_clock>=temp_msg_struct->current_clock)
		         global_clock = global_clock + 1;
		      else
		         global_clock = temp_msg_struct->current_clock +1; 
		     sem_wait(&sem_clock); 
                     } 
	            printf(" GLOBAL CLOCK UPDATED TO %d \n", global_clock);	
                  printf("\n --------------------------------------------------- \n");
		   close(clockSock); 
		}
	


	}
/******************************** CLOCK_THREAD THREAD END*************************/





/******************************** M-RECEIVE THREAD *************************/

	void *m_recV(void *arg)
	{
		int t = (intptr_t) arg;

		int ret, listenSock, connSock,in, flags=0;
		struct sockaddr_in servaddr1;
		struct sctp_sndrcvinfo sndrcvinfo;
		struct sctp_event_subscribe events;
		char buffer1[MAX_BUFFER+1];

		int local_clock=0;
		
		type_msg *temp_msg_struct = (type_msg *) malloc(sizeof(type_msg)); 
		
		/* Create SCTP TCP-Style Socket */
		listenSock = socket( AF_INET, SOCK_STREAM, IPPROTO_SCTP );

		/* Accept connections from any interface */
		bzero( (void *)&servaddr1, sizeof(servaddr1) );
		servaddr1.sin_family = AF_INET;
		servaddr1.sin_addr.s_addr = htonl( INADDR_ANY );
		servaddr1.sin_port = htons(MY_PORT_NUM);

		/* Bind to the wildcard address (all) and MY_PORT_NUM */
		ret = bind( listenSock,(struct sockaddr *)&servaddr1, sizeof(servaddr1) );

		/* Place the server socket into the listening state */
		listen( listenSock, 200 );

		/* Server loop... */
		  while( 1 ) {

		    /* Await a new client connection */
		    connSock = accept( listenSock,(struct sockaddr *)NULL, (int *)NULL );

		    /* New client socket has connected */
		  
		    /* Enable receipt of SCTP Snd/Rcv Data via sctp_recvmsg */
		    memset( (void *)&events, 0, sizeof(events) );
		    events.sctp_data_io_event = 1;
		    setsockopt( connSock, SOL_SCTP, SCTP_EVENTS,(const void *)&events, sizeof(events) );
         
		    in = sctp_recvmsg( connSock, (void *)temp_msg_struct, sizeof(type_msg),(struct sockaddr *)NULL, 0,&sndrcvinfo, &flags );
		    
		    
		    sem_wait(&sem_delivery);
		    strncpy( temp_msg_struct->msg_Status, "UNDELIVERABLE", sizeof("UNDELIVERABLE"));
		    sem_post(&sem_delivery);
		    
		   

		     NODE *x= temp_head;


		     sem_post(&sem_clock);
		         temp_msg_struct->max_clk= global_clock;	     
		         global_clock++;
		      if(global_clock>=temp_msg_struct->current_clock)
		         global_clock = global_clock + 1;
		      else
		         global_clock = temp_msg_struct->current_clock +1; 

		         temp_msg_struct->current_clock= global_clock;
			 addNodeBottom(temp_msg_struct, temp_head);
		     sem_wait(&sem_clock); 
		         
                  
		  
		  printf("\n --------------------------------------------------- \n");
		   while(x->next!=NULL)
		   {
			x= x->next;
			printf("  MESSAGES RECEIVED TILL NOW HAS TIMESTAMP %d : STATUS %s \n", x->msg_Struct.current_clock, temp_msg_struct->msg_Status);
		   }
                  printf("\n --------------------------------------------------- \n");
                    
		    
                  printf("\n --------------------------------------------------- \n");
		    printf(" MESSAGE RECEIVED IS %s %d BYTES\n", temp_msg_struct->message_content, in);
		   
		   
		    printf(" CURRENT CLOCK VALUE UPDATED AFTER RECEIVE VALUE IS %d \n ", global_clock);
                    
                  printf(" \n--------------------------------------------------- \n");
		    temp_msg_struct->msg_Type= 2;
		    ret = sctp_sendmsg( connSock,(void *)temp_msg_struct, sizeof(type_msg),NULL, 0, 0, 0, LOCALTIME_STREAM, 0, 0 );
		    
		    
		    /* Close the client connection */
		    close( connSock );

		  }
		     sem_post(&sem_clock);
		      global_clock++;
		     sem_wait(&sem_clock); 


	}// END OF RECEIVE THREAD

/********************************** END OF RECEIVE THREAD ****************************/

/********************************** START OF M_SEND THREAD ****************************/

     void *m_senD(void *arg)
      {
          int t = (intptr_t) arg;
          int r ,connSock1,clkSoc, ret, in , flags=0;
	  time_t currentTime;
	  struct sctp_sndrcvinfo sndrcvinfo;
	  struct sctp_event_subscribe events;
	  struct sockaddr_in servaddr;
	  char buffer1[MAX_BUFFER+1];
          char *ip_addr[2]={"10.176.67.65","10.176.67.66"};
	  int count_msg=0;
	  int local_clock=0, max_clock=0;
	  int recv_flag=0, skip=0;
	  type_msg *temp_msg_struct = (type_msg *)malloc(sizeof(type_msg));
	  
	  while(1){
		  sem_wait(&semaphore1);
                  printf(" --------------------------------------------------- \n");
                  printf(" --------------------------------------------------- \n");
		  printf("Semaphore posted\n");
		  printf("total destinations, this message to be sent is %d \n", msgStruct[count_msg]->dest_counter);
		  printf("message to be sent is %s \n", msgStruct[count_msg]->message_content);
                  printf(" --------------------------------------------------- \n");
                  printf(" --------------------------------------------------- \n");
		  
		  for (r = 0 ; r < msgStruct[count_msg]->dest_counter ; r++) {
		  /* Create an SCTP TCP-Style Socket */
		  connSock1 = socket( AF_INET, SOCK_STREAM, IPPROTO_SCTP );

		  /* Specify the peez endpoint to which we'll connect */
		  bzero( (void *)&servaddr, sizeof(servaddr) );
		  servaddr.sin_family = AF_INET;
		  servaddr.sin_port = htons(MY_PORT_NUM);
                  printf(" --------------------------------------------------- \n");
		  printf("destination %d's IP address is %s \n", r, msgStruct[count_msg]->dest_ip[r]);
		  servaddr.sin_addr.s_addr = inet_addr(msgStruct[count_msg]->dest_ip[r]);

		  /* Connect to the server */
		  connect( connSock1, (struct sockaddr *)&servaddr, sizeof(servaddr) );

                    msgStruct[count_msg]->msg_Type=1; 
		     
		     sem_post(&sem_clock);
                      msgStruct[count_msg]->current_clock = global_clock; 
		     sem_wait(&sem_clock); 
                   
		    ret = sctp_sendmsg( connSock1,(void *)msgStruct[count_msg], sizeof(type_msg),NULL, 0, 0, 0, LOCALTIME_STREAM, 0, 0 );
	            
                    
		    /* Enable receipt of SCTP Snd/Rcv Data via sctp_recvmsg */
		    memset( (void *)&events, 0, sizeof(events) );
		    events.sctp_data_io_event = 1;
		    setsockopt( connSock1, SOL_SCTP, SCTP_EVENTS,(const void *)&events, sizeof(events) );

		    in = sctp_recvmsg( connSock1, (void *)temp_msg_struct, sizeof(type_msg),(struct sockaddr *)NULL, 0,&sndrcvinfo, &flags );
		     
		     sem_post(&sem_clock);
		      
		      if(global_clock>=temp_msg_struct->current_clock)
		         global_clock = global_clock + 1;
		      else
		         global_clock = temp_msg_struct->current_clock +1; 
		     
		     sem_wait(&sem_clock); 
                               

		    printf("RECEIVED PROPOSED TIME STAMP FROM %s IS %d TYPE IS %d\n",msgStruct[count_msg]->dest_ip[r], temp_msg_struct->max_clk, temp_msg_struct->msg_Type);
                  printf(" --------------------------------------------------- \n");
		    
		    if(max_clock<= temp_msg_struct->max_clk)
                       max_clock=  temp_msg_struct->max_clk;  		    
  		    /* Close our socket and exit */
  		    close(connSock1);
	}

		/**************************** BRAODCAST MAX CLOCK VALUE ************************/
		msgStruct[count_msg]->max_clk= max_clock;
		     
		     sem_post(&sem_clock);
		      global_clock++;
                      temp_msg_struct->current_clock = global_clock; 
		     sem_wait(&sem_clock); 

                  printf(" --------------------------------------------------- \n");
		  printf(" CLOCK UPDATED FOR BROADCASTING %d \n", msgStruct[count_msg]->max_clk);	
                  printf(" --------------------------------------------------- \n");
		    
		    //sleep(2); 
	       
	       for (r = 0 ; r < msgStruct[count_msg]->dest_counter ; r++) {
                  /* Create an SCTP TCP-Style Socket */
                  clkSoc = socket( AF_INET, SOCK_STREAM, IPPROTO_SCTP );

                  /* Specify the peez endpoint to which we'll connect */
                  bzero( (void *)&servaddr, sizeof(servaddr) );
                  servaddr.sin_family = AF_INET;
                  servaddr.sin_port = htons(CLOCK_PORT);
                  printf(" --------------------------------------------------- \n");
		  printf("destination %d's IP address is %s \n", r, msgStruct[count_msg]->dest_ip[r]);
                  servaddr.sin_addr.s_addr = inet_addr(msgStruct[count_msg]->dest_ip[r]);

                  /* Connect to the server */
                  connect( clkSoc, (struct sockaddr *)&servaddr, sizeof(servaddr) );
		 	
		  msgStruct[count_msg]->msg_Type=3;
                  ret = sctp_sendmsg( connSock1,(void *)msgStruct[count_msg], sizeof(type_msg),NULL, 0, 0, 0, LOCALTIME_STREAM, 0, 0 );
		     
                  
		  printf(" MAX CLOCK VALUE %d BROADCASTED TO %s \n", msgStruct[count_msg]->max_clk, msgStruct[count_msg]->dest_ip[r]); 
                  printf(" --------------------------------------------------- \n");
	 	
		close(clkSoc);
	 }

	count_msg++;
      }// END OF WHILE LOOP
      }// END OF SEND THREAD

/********************************** END OF M_SEND THREAD ****************************/


/********************************************* START OF MAIN ******************************************/
void main ()
{

int pipe_1[2], pipe_2[2], pipe_3[2];

pipe(pipe_1);
pipe(pipe_2);
pipe(pipe_3);


/**************************** APPLICATION PROCESS ***********************/

	if(fork()==0)
	{
	   char input[10]; 
           char output[500];
           int firsttime=0;
	   int no_of_msgs=0;
	   int i=0, send_stop_flag=0;
           NODE min;
	   int random=0;
	   int income_Array[2];
           int max_mes=0;
	   int first=0;
	   while(1)
         {
	 
	   if(first==0)
	   {
           read(pipe_2[0], income_Array, sizeof(income_Array));
	   first=1;
	   }

	   (income_Array[0]>=income_Array[1])?(max_mes=income_Array[0]):(max_mes=income_Array[1]);
	  
	   /*printf(" ENTER COMMAND -- m_receive or m_send \n");
	   fflush(stdin);
             */
	    
	    printf(" MAX VALUE BETWEEN %d ([0]) and %d ([1])is %d \n",  income_Array[0], income_Array[1], max_mes);

	   for(random=0; random<income_Array[1]; random++)
	     {
	     sleep(5);
             strncpy(input, "m_send", sizeof("m_send"));
             write(pipe_1[1],input,sizeof(input));
	     }
	     /*
	   scanf("%s",input);
	   if((strcmp(input,"m_receive")!=0)&&(strcmp(input,"m_send")!=0))
	     {
                  printf(" PLEASE ENTER IN PROPER FORMAT \n");
		  fflush(stdin);
		  printf(" ENTER EITHER m_send or m_receive \n");
		  fflush(stdin);
		  scanf("%s",input);
	     }
            

          if((strcmp(input,"m_send")==0))
	     {
              write(pipe_1[1],input,sizeof(input));
	     }*/
           
           //if((strcmp(input,"m_receive")==0))
             for(random=0; random<income_Array[0]; random++)
	     {
	       
              strncpy(input, "m_receive", sizeof("m_receive"));
              write(pipe_1[1],input,sizeof(input));
              
	      read(pipe_3[0], &min, sizeof(NODE));
              
	      if(min.msg_Struct.msg_Type==5)
	      {
		printf(" !!!!!!!!!!!! %s !!!!!!!!!!!! \n", min.msg_Struct.message_content);
	      }
	      else{
	     printf("APPLICATION RECEIVED %d AS MIN CLOCK VALUE AND MESSAGE IS %s\n", min.msg_Struct.max_clk, min.msg_Struct.message_content);
	      }
	      fflush(stdin);
             } 
          } // END OF WHILE LOOP
       }//END OF APPLICATION PROCESS	

/**************************** CONTROL  PROCESS ***********************/

       else
        {
          char inp_received[10];
          int recv_flag=0;

	  pthread_t tid[3];
          sem_init( &semaphore1 ,0 , 0); 
	  sem_init(&sem_clock,0,0);
	  sem_init(&sem_delivery,0,1);
          
           /******* **************************** PARSING INPUT FILE ************************* ********************/

		FILE *ip1;
		FILE *input;

		input = fopen("input.txt","r+");

		char ip_s[13],ip_r[13];
		char line_input[500][100];
		char *pch;
		int  x=0,temp=0,temp1=0,sender=0;
		char hostname[1024];
		     hostname[1023] = '\0';
		char destination[MAX_PROC][MAX_DEST];
		int  i1=0,j=0,k=0, dest=0;
		char message[1024]= "\0";
                int s_No=0, firsttime=0;
		int line_counter=0;
		int multicast_counter[50]={0};
		int source=0, source_counter=0;
		int temp_var=0;
		gethostname(hostname, 1023);
		pch = strtok (hostname,".");
		
		int total_source=0;

		char *myIP = (char*) malloc(15);
		char *myName= (char*) malloc(5);
		int myNumber=0;
    		strncpy(myName, pch+3,2);
		sscanf(myName ,"%d",&myNumber );


		ip1= fopen("ip_address.txt","r+");
		for(temp1=0;temp1<myNumber;temp1++)fgets(myIP, 15, ip1);

		printf("My Machine Name is %d myIP is %s \n", myNumber, myIP );
	      
		/*********************************** PARSE INPUT ***************************/
		while(fgets_wrapper(line_input,100, input))
		{
                  
		  pch = strtok (line_input," ");
		  while (pch != NULL)
		  {
		    if(firsttime==0)
		    {
		     sscanf(pch,"%d",&s_No);
                     firsttime=1;
                     printf("------- %d ---------\n", s_No);
		    }
		    if(i1==1)
		    {
			sscanf(++pch,"%d",&sender);
			j++;
		    }
		    if(strcmp(pch,"to")==0)
		       k=1;

		    while((k==1)&&(strcmp(pch,":")!=0))
		     {
		       pch = strtok (NULL," ");
		       if(strcmp(pch,":")==0)
		       {
			 pch = strtok (NULL, " ");
			 k=2;
			 break;
		       }
		  
		     printf("pch is %s \n", pch);
                     
		     sscanf(++pch,"%d", &temp_var);
		     if(myNumber==temp_var)
		       source_counter++;
		     
		     --pch; 

		     strcpy(destination[dest],pch);
		     dest++;
		     }
		    if(k==2)
		    {
		      strcat(message,pch);
		      strcat(message," ");
		    }
		    pch = strtok (NULL, " ");
		    i1++;

		  }
		  
		  ip1= fopen("ip_address.txt","r+");
		   for(temp=0;temp<sender;temp++)
		       fgets_wrapper(ip_s, 14, ip1);
			
		   if(strncmp(myIP,ip_s,12)==0)
		      {
		      source= sender; 
		      
		      multicast_counter[source]++;
		      msgStruct[multicast_counter[source]-1]= (type_msg *) malloc ( sizeof(type_msg) );   
		      msgStruct[multicast_counter[source]-1]->msg_id = s_No;
		      msgStruct[multicast_counter[source]-1]->msg_Source = source; 

		      printf(" IP ADDRESS FOUND !!!!!!!!! %d \n",  multicast_counter[sender]); 
		    /*  not_found++;
		      }
		       
		   if(not_found!=0)
		   {*/
		   for(k=0;k<dest;k++)
		   {
		   ip1= fopen("ip_address.txt","r+");
		   sscanf(destination[k]+1, "%d", &x);
		   
		   for(temp=0;temp<x;temp++)
		        fgets_wrapper(ip_r, 14, ip1);
		   
                  printf("\n --------------------------------------------------- \n");
		   printf(" SENDER PROCESS IS %d (IP: %s ) DESTINATION: %s (IP: %s) \n", source,ip_s, destination[k],ip_r);
                  printf("\n --------------------------------------------------- \n");
		   
		   strcpy(msgStruct[multicast_counter[source]-1]->dest_ip[k],ip_r);
			
	           
 		   }
		   printf(" Message to be sent is : %s \n", message);
		   msgStruct[multicast_counter[source]-1]->dest_counter = dest;
		   strncpy(msgStruct[multicast_counter[source]-1]->message_content,message, strlen(message));
		   
		  }
		  
		  strcpy(message,"\0");
		  i1=0;
		  k=0;
		  dest=0;
		  firsttime=0;

	     }

    /******* **************************** END OF PARSING INPUT FILE ************************* ********************/
           printf("%d------------  \n",multicast_counter[source]);
           printf("%d------------  \n",multicast_counter[source]-1);
           
	   total_source = multicast_counter[source]; 
	   
	   printf(" %d found %d times as destination and %d times as source \n", myNumber, source_counter,total_source);

	  type_msg  *temp_msg = (type_msg *)malloc(sizeof(type_msg));
	  temp_msg->max_clk=-1;

	  temp_head= list_create(temp_msg);


	  pthread_create(&tid[0], NULL , m_recV , NULL);
	  pthread_create(&tid[1], NULL, m_senD, NULL);
	  pthread_create(&tid[2], NULL, clock_thread, NULL);

	  int m_send_count=0;
	  int loop=0;
	  int min_list=0, min_pos=0, counter=0;
	  int min, m=0;
          int mesArray[2];
          FILE *file; 
	  char fileName[10]; 
	  char file_entry[10];

	  mesArray[0]= source_counter;
	  mesArray[1]= total_source; 

	      sprintf(fileName,"file%d.txt",myNumber);

	      file= fopen(fileName,"w+");
	     
	     while(1)
		  {
		   printf("CONTROL PROCESS WAITING FOR INPUT\n");
	           	
		   if(loop==0)
		    {
		       write(pipe_2[1], mesArray, sizeof(mesArray));
		       loop=1;
		    }
		    
		    if((total_source<=0)&&(source_counter<=0))
		       {

			  printf("\n --------------------------------------------------- \n");
			  printf("\n --------------------------------------------------- \n");
			  printf("\n -----BOTH SOURCE AND RECEIVE ARE ZERO-------------- \n");
			  printf("\n --------------------------------------------------- \n");
			fclose(file);
			  printf("\n --------------------------------------------------- \n");
			  printf("\n --------------------------------------------------- \n");
			  printf("\n ------------------FILE CLOSED---------------------- \n");
			  printf("\n --------------------------------------------------- \n");
		          break; 
		       }	

            else
		   
		{


		   read(pipe_1[0], inp_received, sizeof(inp_received));

		   printf(" MESSAGE RECEIVED FROM APPLICATION PROCESS IS %s \n", inp_received); 
		   fflush(stdin);
		   
		   if((strcmp(inp_received,"m_send")==0))
		     {
			     m_send_count++;
			     total_source--;
			    printf("*************************** %d MORE MESSAGES TO BE SENT************* \n", total_source);
			     if(m_send_count>multicast_counter[source])
			       printf( "!!!!!!!!!!! SORRY NO MESSAGES TO SEND !!!!!!!!!!! \n");

			     else{
			  printf("\n --------------------------------------------------- \n");
			     printf(" COMMAND RECEIVED TO SEND A MESSAGE \n");
			     fflush(stdin);
			     sem_post(&sem_clock);
			      global_clock++;
			     sem_wait(&sem_clock); 
			     printf(" SEND : LOCAL CLOCK UPDATED TO %d \n", global_clock);
			  printf("\n --------------------------------------------------- \n");
			     sem_post(&semaphore1);
			     }
		    }
			   
		   if((strcmp(inp_received,"m_receive")==0))
	     {

		     source_counter--;
		     printf("*************************** %d MORE MESSAGES TO BE DELIVERED************* \n", source_counter);

                     printf("\n --------------------------------------------------- \n");
		      printf(" COMMAND RECEIVED TO DELIVER A MESSAGE TO APPLICATION \n");
		      fflush(stdin);
	 	     sleep(2);    
                     if(temp_head->next==NULL)
		       {
			 NODE *empty_node = (NODE *)malloc(sizeof(NODE));

			 empty_node->msg_Struct.msg_Type=5;
			 strcpy(empty_node->msg_Struct.message_content, "SORRY NO MESSAGES TO DELIVER");

			 write(pipe_3[1], empty_node, sizeof(NODE));
		       }
	     else
		{
		     sem_post(&sem_clock);
		      global_clock++;
		     sem_wait(&sem_clock);

		     NODE *temp1 = temp_head;
		     NODE *min_node; 
		     temp1= temp1->next;
		     min= temp1->msg_Struct.max_clk;
		     while(temp1->next!=NULL)
		     {

		        if(temp1->msg_Struct.max_clk<=min)
			{
			  min = temp1->msg_Struct.max_clk; 
		          min_pos = counter;
			}
			counter++;
		        temp1= temp1->next;
		     }
			temp1 = temp_head->next;
		     for(m=0; m<min_pos;m++)
		       {
			temp1= temp1->next; 
		       }               
		      min_node= temp1;
		     
		      while(1)
		      {
		      sem_wait(&sem_delivery);
		      if(strcmp(min_node->msg_Struct.msg_Status, "DELIVERABLE")==0)
		      {
			/*if(line_counter==5)
			 {
		        sprintf(file_entry,"\n<M%d:SENDER %d> ", min_node->msg_Struct.msg_id, min_node->msg_Struct.msg_Source); 	 
                        printf("\n --------------------------------------------------- \n");
			printf(" FILE ENTRY TO BE WRITTEN IS %s \n", file_entry);
                        printf("\n --------------------------------------------------- \n");
		      
			    fwrite(file_entry, sizeof(char), strlen(file_entry), file );
			    line_counter=0;
			 }*/
			//else 
			{
		        sprintf(file_entry,"<M%d:SENDER %d>\n", min_node->msg_Struct.msg_id, min_node->msg_Struct.msg_Source); 	 
                        printf("\n --------------------------------------------------- \n");
			printf(" FILE ENTRY TO BE WRITTEN IS %s \n", file_entry);
                        printf("\n --------------------------------------------------- \n");
		     
				
		        fwrite(file_entry, sizeof(char), strlen(file_entry), file );
		        //line_counter++;

			}
		        strcpy(file_entry, "\0");

		         printf(" MIN VALUE FOUND IN LIST IS %d and %d \n", min_node->msg_Struct.max_clk, min);
		         counter=0;
                         printf("\n --------------------------------------------------- \n");
		         if(min!=-1)
		         write(pipe_3[1],min_node,sizeof(NODE));

		         else
		            printf(" !!!!!!!!!!!!!!!!!!!!!!!! NOTHING TO RETURN !!!!!!!!!!!!!!!!!!!!!!!! \n");
		    
		        
			  list_remove(temp_head,min_node); 
             		
		        NODE *x= temp_head;

		   while(x->next!=NULL)
		   {
			x= x->next;
			printf(" NEW TIME STAMP VALUES IN LIST ARE %d \n", x->msg_Struct.max_clk);
		   }
			printf(" RECEIVE : LOCAL CLOCK UPDATED TO %d \n", global_clock);
		  
		    sem_post(&sem_delivery);
		    break; 
		  }
		  
		  else
		    printf(" !!!!!!!!!!!!!!!!!!! CANNOT DELIVER ANY MESSAGES ...ONLY UNDELIVERABLE MESSAGES IN QUEUE!!!!!!!!!!!!!!!!!! \n");
		} 
		  }// END OF DELIVERABLE-ELSE
		  }// END OF ELSE
	      }//M_RECEIVE END
	   } 
          }// END OF WHILE LOOP
        } // END OF CONTROL PROCESS

