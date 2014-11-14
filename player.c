/*........................ Include Files ....................................*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <pthread.h>
#include <netdb.h>
#define MAX 1024
#define LEN	1024

struct Potato
{
    int size[MAX];
    int i;
    int hops;
}*potato;

int s;
int player_num,num_of_players,so_reuseaddr=1,end_of_game=0,con=0;
int call=0;
char padosi[LEN],buf[LEN],str[LEN];


void * threadFunc(void * arg)
{
    int new_sock=*(int*)arg;
    int len,connected_to;//2=left , 1=right

    connected_to=con;
    /*printf("Welcome to the thread\n");
    sprintf(buf,"Player %d connected to you now as %d",player_num,connected_to);
    fflush(stdout);
    len = send(new_sock, buf, 64, 0);*/

    /* read and print strings sent over the connection */
    while ( !end_of_game )
    {
        if(call==connected_to)
        {
            if(connected_to==2 && player_num!=0)
            printf("Sending potato to %d\n",(player_num-1));
            else if(connected_to==2 && player_num==0)
            printf("Sending potato to %d\n",(num_of_players-1));
            else if(connected_to==1 && player_num!=(num_of_players-1))
            printf("Sending potato to %d\n",(player_num+1));
            else if(connected_to==1 && player_num==(num_of_players-1))
            printf("Sending potato to 0\n");

            len=send(new_sock, potato, sizeof(struct Potato),MSG_DONTWAIT);
            //printf("Player %d sending potato to player on ",potato->size[potato->i-1]);
            //printf("sending POTATO with %d hops and size %d\n",potato->hops,potato->i);
            call=0;
        }
        len=recv(new_sock, potato, sizeof(struct Potato),MSG_DONTWAIT);

        if(len>0)
        {
            //printf("received POTATO with %d hops and size %d from player %d\n",potato->hops,potato->i,potato->size[potato->i-1]);
            //Modify the potato
            potato->size[potato->i]=player_num;
            potato->i++;
            potato->hops--;
            if(potato->hops==0)
            {
                printf("I'm it\n");//printf("Terminating player %d\n",player_num);
                len = send(s, potato, sizeof(struct Potato), 0);
                end_of_game=1;
            }
            else
            call = rand() % 2 + 1;
        }
        /*len = recv(new_sock, buf, 64, 0);
        if ( len < 0 )
        {
            perror("recv");
            pthread_exit(0);
        }
        buf[len] = '\0';
        if ( !strcmp("close", buf) )
            break;
        else
            printf("%s\n", buf);*/
    }
    close(new_sock);
    pthread_exit(0);
}

main (int argc, char *argv[])
{

    pthread_t thr;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    int s_left,new_s_left,s_right,new_s_right, p,rc, len, port,port_left,port_right;
    char host[LEN];
    struct hostent *hp, *hp_left, *hp_right,*hp_self;
    struct sockaddr_in server,right_neighbor,left_neighbor,right_initiator,left_initiator;
    potato = (struct Potato *)malloc(1*sizeof(struct Potato));

    /* read host and port number from command line */
    if ( argc != 3 )
    {
        fprintf(stderr, "Usage: %s <host-name> <port-number>\n", argv[0]);
        exit(1);
    }

    /* fill in hostent struct for master*/
    hp = gethostbyname(argv[1]);
    if ( hp == NULL )
    {
        fprintf(stderr, "host not found\n");
        exit(1);
    }
    port = atoi(argv[2]);

    /* create and connect to a socket */

    /* use address family INET and STREAMing sockets (TCP) */
    s = socket(AF_INET, SOCK_STREAM, 0);
    if ( s < 0 )
    {
        perror("socket:");
        exit(s);
    }
    setsockopt(s,SOL_SOCKET,SO_REUSEADDR,
    &so_reuseaddr,
    sizeof so_reuseaddr);

    /* set up the address and port */
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    memcpy(&server.sin_addr, hp->h_addr_list[0], hp->h_length);

    /* connect to socket at above addr and port */
    rc = connect(s, (struct sockaddr *)&server, sizeof(server));
    if ( rc < 0 )
    {
        perror("connect:");
        exit(rc);
    }

    len = recv(s, str, sizeof(str), 0);//get the player ID
    player_num=atoi(str);
    printf("Connected as player %d\n",player_num);
    strcpy(str,"ACK player number");
    len = send(s, str, strlen(str), 0);//send acknowledgement

    len = recv(s, str, sizeof(str), 0);//get the total player count
    num_of_players=atoi(str);
    strcpy(str,"ACK player count");
    len = send(s, str, strlen(str), 0);//send acknowledgement

    if(player_num!=0)
    {
        len = recv(s, padosi, sizeof(padosi), 0); //get the left neighbor info for everyone

        /* fill in hostent struct for self */
        gethostname(host, sizeof(host));
        hp_self = gethostbyname(host);
        if ( hp_self == NULL )
        {
            fprintf(stderr, "host not found (%s)\n", host);
            exit(1);
        }

        if(!strcmp(padosi,"localhost"))
        strcpy(padosi,hp_self->h_name);
        strcpy(str,"ACK left neighbor info");
        len = send(s, str, strlen(str), 0);//send acknowledgement
    }

    if(player_num==0)//first player listens on 2 ports--send the info to master
    {
        len = recv(s, str, 64, 0);

        /* fill in hostent struct for self */
        gethostname(host, sizeof(host));
        hp_self = gethostbyname(host);
        if ( hp_self == NULL )
        {
            fprintf(stderr, "host not found (%s)\n", host);
            exit(1);
        }

         port_right = port+player_num+99;
        /* use address family INET and STREAMing sockets (TCP) */
        s_right = socket(AF_INET, SOCK_STREAM, 0);
        if ( s_right < 0 )
        {
            perror("socket:");
            exit(s_right);
        }
        setsockopt(s_right,SOL_SOCKET,SO_REUSEADDR,
        &so_reuseaddr,
        sizeof so_reuseaddr);

        do
        {
            /* set up the address and port */
            right_initiator.sin_family = AF_INET;
            right_initiator.sin_port = htons(port_right);
            memcpy(&right_initiator.sin_addr, hp_self->h_addr_list[0], hp_self->h_length);

            /* bind socket s_right to address server */
            rc = bind(s_right, (struct sockaddr *)&right_initiator, sizeof(right_initiator));
            if ( rc < 0 )
            {
               port_right++;
            }
        }while(rc<0);

        rc = listen(s_right, 5);
        if ( rc < 0 )
        {
            perror("listen:");
            exit(rc);
        }

        sprintf(str,"%d",port_right);
        len = send(s, str, 64, 0);

        len = recv(s, str, 64, 0);//Now left port

        /* fill in hostent struct for self */
        gethostname(host, sizeof(host));
        hp_self = gethostbyname(host);
        if ( hp_self == NULL )
        {
            fprintf(stderr, "host not found (%s)\n", host);
            exit(1);
        }

        port_left = port+num_of_players+99;
        /* use address family INET and STREAMing sockets (TCP) */
        s_left = socket(AF_INET, SOCK_STREAM, 0);
        if ( s_left < 0 )
        {
            perror("socket:");
            exit(s_left);
        }
        setsockopt(s_left,SOL_SOCKET,SO_REUSEADDR,
        &so_reuseaddr,
        sizeof so_reuseaddr);

        do
        {
            /* set up the address and port */
            left_initiator.sin_family = AF_INET;
            left_initiator.sin_port = htons(port_left);
            memcpy(&left_initiator.sin_addr, hp_self->h_addr_list[0], hp_self->h_length);

            /* bind socket s_left to address server */
            rc = bind(s_left, (struct sockaddr *)&left_initiator, sizeof(left_initiator));
            if ( rc < 0 )
            {
               port_left++;
            }
        }while(rc<0);

        sprintf(str,"%d",port_left);
        len = send(s, str, 64, 0);

        rc = listen(s_left, 5);
        if ( rc < 0 )
        {
            perror("listen:");
            exit(rc);
        }


        len = sizeof(right_initiator);
        p = accept(s_right, (struct sockaddr *)&right_neighbor, &len);//you send to your right neigbor through this socket
        if ( p < 0 )
        {
            perror("bind:");
            exit(rc);
        }

        con=1;
        //printf("Right neighbor connected\n");
        fflush(stdout);
        pthread_create(&thr,&attr,threadFunc,&p);

        len = sizeof(left_initiator);
        p = accept(s_left, (struct sockaddr *)&left_neighbor, &len);
        if ( p < 0 )
        {
            perror("bind:");
            exit(rc);
        }

        con=2;
        //printf("Left neighbor connected\n");
        fflush(stdout);
        pthread_create(&thr,&attr,threadFunc,&p);
    }

    if(player_num!=0 && player_num!=(num_of_players-1))//each player connects to left neighbor and listens to right neighbor
    {
        /* fill in hostent struct for left neighbor*/
        hp_left = gethostbyname(padosi);
        if ( hp_left == NULL )
        {
            fprintf(stderr, "host not found\n");
            exit(1);
        }


        len = recv(s, str, 64, 0);
        port_left=atoi(str);
        /* create and connect to a socket */

        /* use address family INET and STREAMing sockets (TCP) */
        s_left = socket(AF_INET, SOCK_STREAM, 0);
        if ( s_left < 0 )
        {
            perror("socket:");
            exit(s_left);
        }
	setsockopt(s_left,SOL_SOCKET,SO_REUSEADDR,
    	&so_reuseaddr,
    	sizeof so_reuseaddr);

        /* set up the address and port */
        left_neighbor.sin_family = AF_INET;
        left_neighbor.sin_port = htons(port_left);
        memcpy(&left_neighbor.sin_addr, hp_left->h_addr_list[0], hp_left->h_length);

        /* connect to socket at above addr and port */
        rc = connect(s_left, (struct sockaddr *)&left_neighbor, sizeof(left_neighbor));
        if ( rc < 0 )
        {
            perror("connect:");
            exit(rc);
        }

        con=2;
        //printf("connected to left neighbor\n");
        pthread_create(&thr,&attr,threadFunc,&s_left);

        /*NOW WE LISTEN TO OUR RIGHT NEIGHBOR*/

        /* fill in hostent struct for self */
        gethostname(host, sizeof(host));
        hp_self = gethostbyname(host);
        if ( hp_self == NULL )
        {
            fprintf(stderr, "host not found\n");
            exit(1);
        }

         port_right = port_left+player_num+99;
        /* use address family INET and STREAMing sockets (TCP) */
        s_right = socket(AF_INET, SOCK_STREAM, 0);
        if ( s_right < 0 )
        {
            perror("socket:");
            exit(s_right);
        }
        setsockopt(s_right,SOL_SOCKET,SO_REUSEADDR,
    	&so_reuseaddr,
    	sizeof so_reuseaddr);

        do
        {
            /* set up the address and port */
            right_initiator.sin_family = AF_INET;
            right_initiator.sin_port = htons(port_right);
            memcpy(&right_initiator.sin_addr, hp_self->h_addr_list[0], hp_self->h_length);

            /* bind socket s to address server */
            rc = bind(s_right, (struct sockaddr *)&right_initiator, sizeof(right_initiator));
            if ( rc < 0 )
            {
               port_right++;
            }
        }while(rc<0);

        sprintf(str,"%d",port_right);
        len = send(s, str, 64, 0);

        rc = listen(s_right, 5);
        if ( rc < 0 )
        {
            perror("listen:");
            exit(rc);
        }


        len = sizeof(right_initiator);
        p = accept(s_right, (struct sockaddr *)&right_neighbor, &len);
        if ( p < 0 )
        {
            perror("bind:");
            exit(rc);
        }
        con=1;
        //printf("Right neighbor connected\n");
        pthread_create(&thr,&attr,threadFunc,&p);

    }

    if(player_num==(num_of_players-1))//recieve the port on which the first player is already listening to the last player
    {
        /* fill in hostent struct for left neighbor*/
        hp_left = gethostbyname(padosi);
        if ( hp_left == NULL )
        {
            fprintf(stderr, "host not found\n");
            exit(1);
        }

        len = recv(s, str, 64, 0);
        port_left=atoi(str);
        /* create and connect to a socket */

        /* use address family INET and STREAMing sockets (TCP) */
        s_left = socket(AF_INET, SOCK_STREAM, 0);
        if ( s_left < 0 )
        {
            perror("socket:");
            exit(s_left);
        }
        setsockopt(s_left,SOL_SOCKET,SO_REUSEADDR,
    	&so_reuseaddr,
    	sizeof so_reuseaddr);

        /* set up the address and port */
        left_neighbor.sin_family = AF_INET;
        left_neighbor.sin_port = htons(port_left);
        memcpy(&left_neighbor.sin_addr, hp_left->h_addr_list[0], hp_left->h_length);

        /* connect to socket at above addr and port */
        rc = connect(s_left, (struct sockaddr *)&left_neighbor, sizeof(left_neighbor));
        if ( rc < 0 )
        {
            perror("connect:");
            exit(rc);
        }//printf("connected to left neighbor\n");
        con=2;
        pthread_create(&thr,&attr,threadFunc,&s_left);
        strcpy(str,"ACK");
        len = send(s, str, strlen(str), 0);//send acknowledgement

        /*NOW WE CONNECT TO OUR RIGHT NEIGHBOR*/

        len = recv(s, padosi, 64 , 0); //get the right neighbor(first player) info

	/* fill in hostent struct for self */
        gethostname(host, sizeof(host));
        hp_self = gethostbyname(host);
        if ( hp_self == NULL )
        {
            fprintf(stderr, "host not found (%s)\n", host);
            exit(1);
        }

        if(!strcmp(padosi,"localhost"))
        strcpy(padosi,hp_self->h_name);
        strcpy(str,"ACK");
        len = send(s, str, strlen(str), 0);//send acknowledgement

        /* fill in hostent struct for right neighbor*/
        hp_right = gethostbyname(padosi);
        if ( hp_right == NULL )
        {
            fprintf(stderr, "host not found\n");
            exit(1);
        }

        len = recv(s, str, 64, 0);
        port_right=atoi(str);
        /* create and connect to a socket */

        /* use address family INET and STREAMing sockets (TCP) */
        s_right = socket(AF_INET, SOCK_STREAM, 0);
        if ( s_right < 0 )
        {
            perror("socket:");
            exit(s_right);
        }
        setsockopt(s_right,SOL_SOCKET,SO_REUSEADDR,
    	&so_reuseaddr,
    	sizeof so_reuseaddr);

        /* set up the address and port */
        right_neighbor.sin_family = AF_INET;
        right_neighbor.sin_port = htons(port_right);
        memcpy(&right_neighbor.sin_addr, hp_right->h_addr_list[0], hp_right->h_length);

        /* connect to socket at above addr and port */
        rc = connect(s_right, (struct sockaddr *)&right_neighbor, sizeof(right_neighbor));
        if ( rc < 0 )
        {
            perror("connect:");
            exit(rc);
        }
        con=1;
        //printf("connected to right neighbor\n");
        pthread_create(&thr,&attr,threadFunc,&s_right);
        strcpy(str,"ACK");
        len = send(s, str, strlen(str), 0);//send acknowledgement

    }

        //START RECIEVING THE POTATO NOW THAT CONNECTION IS COMPLETE

        //printf("I am listening for a potato from master\n");
        while(!end_of_game)
        {
            len = recv(s, potato, sizeof(struct Potato), MSG_DONTWAIT);
            if(len>=0)
            break;
        }
        //printf("Potato recieved with %d hops and %d bytes\n",potato->hops,len);
        //fflush(stdout);
        if(potato->hops==0)
        {
            end_of_game=1;
            close(s);
            exit(0);
        }

        //Modify the potato
        potato->size[potato->i]=player_num;
        potato->i++;
        potato->hops--;
        if(potato->hops==0)
        {
            printf("I'm it\n");//printf("sending POTATO back to Master\n",str);
            len = send(s, potato, sizeof(struct Potato), 0);
            end_of_game=1;
            close(s);
            exit(0);
        }


        call=rand()%2 + 1;
        //printf("I am listening for a  potato from master\n");

        /* read a string from the terminal and send on socket */
        while(!end_of_game)
        {

            len = recv(s, potato, sizeof(struct Potato), MSG_DONTWAIT);
            /*if(len>=0)
            {
                printf("Potato recieved with %d hops and %d bytes\n",potato->hops,len);
                fflush(stdout);
            }*/
            if(potato->hops==0)
                end_of_game=1;
        }


    /* when finished sending, tell host you are closing and close */

    close(s);
    exit(0);
}

/*........................ end of speak.c ...................................*/
