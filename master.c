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

struct hostent *hp, *ihp, *first, *last, *temp;
struct sockaddr_in server, client;
int num_of_players;
char buf[MAX];
char host[1024];
int s, p, fp, rc, len, port,i,flag,right_port,left_port,so_reuseaddr=1;
int rand_num=-1,end_game=0;



struct Potato
{
  int size[MAX];
  int i;
  int hops;
}*potato;

void srand(unsigned int seed);

void * threadFunc(void * arg)
{
  int new_sock=*(int*)arg;
  struct hostent *player;
  int player_num;

  if(i==num_of_players-1)
  {
    first=(struct hostent *)malloc(1*sizeof(struct hostent));
    first=gethostbyaddr((char *)&client.sin_addr,sizeof(struct in_addr), AF_INET);
  }

  player=gethostbyaddr((char *)&client.sin_addr,sizeof(struct in_addr), AF_INET);
  player_num=num_of_players-1-i;
  printf("player %d is on %s\n", player_num, player->h_name);
  i--;

  sprintf(buf,"%d",player_num);
  len = send(new_sock, buf, 64, 0);//send the player number info to the player
  len = recv(new_sock, buf, 64, 0);

  sprintf(buf,"%d",num_of_players);
  len = send(new_sock, buf, 64, 0);//send the total number of player info
  len = recv(new_sock, buf, 64, 0);

  if(player_num!=0)//send the left neighbor info to the players
  {
    strcpy(buf,temp->h_name);
    len = send(new_sock, buf, 64, 0);
    len = recv(new_sock, buf, 64, 0);
  }
  temp = gethostbyaddr((char *)&client.sin_addr,sizeof(struct in_addr), AF_INET);


  if(player_num==0)//first player listens on 2 ports--get the info
  {
    len = send(new_sock,buf, 64, 0);
    len = recv(new_sock, buf, 64, 0);
    right_port=atoi(buf);
    len = send(new_sock, buf, 64, 0);
    len = recv(new_sock, buf, 64, 0);
    left_port=atoi(buf);
  }
  if(player_num!=0 && player_num!=(num_of_players-1))//send each player the port on which the left neighbor is listening and get the port on which it is listening to the right neighbor
  {
    sprintf(buf,"%d",right_port);
    len = send(new_sock, buf, 64, 0);
    len = recv(new_sock, buf, 64, 0);
    right_port=atoi(buf);
  }
  if(player_num==(num_of_players-1))//send the port on which the first player is already listening to the last player
  {
    sprintf(buf,"%d",right_port);
    len = send(new_sock, buf, 64, 0);
    len = recv(new_sock, buf, 64, 0);
    len = send(new_sock, first->h_name, 64, 0);//need to send 1st player info to last
    len = recv(new_sock, buf, 64, 0);
    sprintf(buf,"%d",left_port);
    len = send(new_sock, buf, 64, 0);
    len = recv(new_sock, buf, 64, 0);
  }
  /* read and print strings sent over the connection */
  while ( !end_game )
  {
    while(rand_num>=-1)
    {
      if(potato->hops==0 && rand_num!=-1)
      {
        len = send(new_sock, potato, sizeof(struct Potato), 0);
        rand_num=-2;
        break;
        //printf("Potato Launched with %d hops %d bytes!!!\n",potato->hops,len);
      }
      if(rand_num==player_num)
      {
        len = send(new_sock, potato, sizeof(struct Potato), 0);
        rand_num=-2;
        //printf("Potato Launched with %d hops %d bytes!!!\n",potato->hops,len);
      }
    }

    len = recv(new_sock, potato, sizeof(struct Potato), MSG_DONTWAIT);

    if(len>=0)
    {
      //printf(">>Connection to Player %d terminated by Master\n",player_num);
      end_game=1;
      break;
    }
    if(end_game)
    len = send(new_sock, potato, sizeof(struct Potato), 0);
  }
  close(new_sock);
  flag--;
  pthread_exit(0);
}

int main (int argc, char *argv[])
{
  pthread_t thr;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
  time_t t;
  potato = (struct Potato *)malloc(1*sizeof(struct Potato));
  /* read port number from command line */
  if ( argc < 4 )
  {
    fprintf(stderr, "Usage: %s <port-number> <number-of-players> <hops>\n", argv[0]);
    exit(1);
  }
  port = atoi(argv[1]);

  num_of_players = atoi(argv[2]);
  if(num_of_players<2)
  {
    printf("Number of players has to be greater than 1\n");
    return 1;
  }
  flag=num_of_players;

  potato->hops=atoi(argv[3]);
  if(potato->hops<0)
  {
    printf("Number of hops has to be non-negative\n");
    return 1;
  }
  potato->i=1;

  /* fill in hostent struct for self */
  gethostname(host, sizeof(host));
  hp = gethostbyname(host);
  if ( hp == NULL )
  {
    fprintf(stderr, "%s: host not found (%s)\n", argv[0], host);
    exit(1);
  }

  /* open a socket for listening
  * 4 steps:
  *	1. create socket
  *	2. bind it to an address/port
  *	3. listen
  *	4. accept a connection
  */

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

    /* bind socket s to address server */
    rc = bind(s, (struct sockaddr *)&server, sizeof(server));
    if ( rc < 0 )
    {
      perror("bind:");
      exit(rc);
    }

    rc = listen(s, 5);
    if ( rc < 0 )
    {
      perror("listen:");
      exit(rc);
    }

    i=num_of_players-1;
    int temp=i;

    printf("Potato Master on %s\n",hp->h_name);
    printf("Players = %d\n",num_of_players);
    printf("Hops = %d\n",potato->hops);

    /* accept connections */
    while (1)
    {
      while(temp>=0)
      {
        len = sizeof(server);
        p = accept(s, (struct sockaddr *)&client, &len);
        if ( p < 0 )
        {
          perror("bind:");
          exit(rc);
        }

        pthread_create(&thr,&attr,threadFunc,&p);
        temp--;
      }

      //send the potato to a randomly picked player
      if(i==-1)
      {
        //sleep(1);

        srand(5);
        rand_num = rand() % num_of_players;
        if(potato->hops>0)
        printf("All players present, sending potato to player %d\n",rand_num);
        else
        break;
        i--;
      }
      if(!flag)
      {
        printf("Trace of potato:\n");
        for(len=1;len<potato->i;len++)
        {
          printf("%d",potato->size[len]);
          if(len!=potato->i-1)
          printf(",");
        }
        break;
      }

    }
    close(s);
    return 0;
  }

  /*........................ end of listen.c ..................................*/
