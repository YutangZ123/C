#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include<string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include<sys/time.h>
#include <arpa/inet.h>
#include<pthread.h>
#include <netinet/in.h>

struct linkid{
    int count;
    int soc;
    struct linkid *next;
};

struct rec{
    char msg[30];
    int soc;
    int count;
    char filename[20];
    int size;
    char buffer[1024];
};

struct snd{
    char msg[30];
    int soc;
    int count;
    char filename[20];
    int size;
    char buffer[1024];
};

struct Memeber{
    int cnt;
    int count[10];
};

struct message{
    int type;
    char chat_message[256];
    char file_data[1024];
};

struct timeval timecnt;
struct Memeber meber;
int clientcount =0;
pthread_t id;
struct linkid *linkhead = NULL;
struct rec resmsg;
struct snd sndmsg;
int man =0;
fd_set read_set;
int sockfd, new_fd, max_fd;
struct sockaddr_in server_addr;
struct sockaddr_in client_addr;
int sin_size, portnumber;
void *sendmessage(void *args);
void filedeal (int count);
void filesend(int count);
void fileend(int count);
void *createclienct(void *args);
void newclientTIP(int count, char *data);
void sendonline (struct linkid *pt, int soc);
void SENDMSG(struct linkid *pt, int count);
void delectclient(int count);
void *handle_client(void *client_socket);

//monitor readability
int SELECT(struct linkid *head){
    FD_ZERO(&read_set);
    int i;
    max_fd=0;
    for (i=0; i<man; i++){
        FD_SET(head->soc, &read_set);
        if (head->soc>max_fd){
            max_fd=head->soc;
        }
        head = head->next;
    }
    head =linkhead;
    if(select(max_fd+1, &read_set, NULL, NULL,&timecnt)>0)
        return 1;
    else return 0;
}

void sendonline (struct linkid*pt, int soc){
    pt = linkhead;
    int i;
    for (i=0; i<man;i++){
        meber.count[i]=pt->count;
        fprintf(stderr,"\n%d online account",meber.count[i]);
        pt=pt->next;
    }
    meber.cnt =man;
    fprintf(stderr,"\n%d online user", meber.cnt);
    if(write(soc, &meber, sizeof(struct Memeber))<=0){
        fprintf(stderr,"\n online information send fail",NULL);
    }else{
        fprintf(stderr, "\n online information sent",NULL);
    }
}

//message sent response
void SENDMSG(struct linkid *pt, int count){
    pt = linkhead;
    int j;
    for (j=0;j<man; j++){
        strcpy(sndmsg.msg, resmsg.msg);
        sndmsg.count = count;
        if(write(pt->soc, &sndmsg, sizeof(struct snd))>0){
            fprintf(stderr,"\n server return successfully:%s, id=%d",sndmsg.msg,pt->soc);
        }else{
            fprintf(stderr,"\n server send fail:%s",strerror(errno));
            pt=pt->next;
        }
    }
}

//delect offline user
void delectclient(int count){
    struct linkid *dt, *df;
    close(count);
    dt=linkhead;
    int i;
    for (i=0;i<man;i++){
        if((count==dt->soc)&&(i==0)){
            linkhead=dt->next;
            --man;
            break;
        }
        df=dt;
        dt=dt->next;
        if ((count==dt->soc)&&(i==(man-1))){
            dt=NULL;
            --man;
            break;
        }
        if (count==dt->soc){
            df->next=dt->next;
            --man;
            break;
        }
    }
}

//send message
void *sendmessage(void *args){
    struct linkid *phead, *pt;
    int i;
    int j;
    int k;
    int flag = 0;
    while(1){
        phead=linkhead;
        pt = linkhead;
        if(SELECT(phead)){
            for(k=0;k<man;k++){
                if(FD_ISSET(phead->soc, &read_set)){
                    if (read(phead->soc, &resmsg, sizeof(struct rec))>0){
                        fprintf(stderr,"\n server received: %s, id=%d", resmsg.msg, phead->soc);
                        if(strcmp(resmsg.msg,"exit")==0){
                            fprintf(stderr, "\n exit signal received, soc=%d close",phead->soc);
                            delectclient(phead->soc);//delete user information(links)
                            char out[10]="OUT";
                            newclientTIP(phead->count,out);//offline reminder
                            break;
                        } else if(strcmp(resmsg.msg,"checkonline")==0){
                            sendonline(pt,phead->soc);//send online user information
                            break;
                        } else if(strcmp(resmsg.msg,"deal")==0){
                            filedeal(resmsg.count);//send deal
                            break;
                        } else if (strcmp(resmsg.msg,"SEND")==0){
                            filesend(resmsg.count);//send file information
                            break;
                        }else if (strcmp(resmsg.msg,"fileend")==0){
                            fileend(resmsg.count);//send end mark
                            break;
                        }else{
                            SENDMSG(pt, resmsg.count);//return received message
                        }
                    }else{
                        fprintf(stderr,"\n receive error %s",strerror(errno));
                        break;
                    }
                }
                phead=phead->next;
            }
        }
    }
}

//send file deal
void filedeal(int count){
    struct linkid *pt=linkhead;
    int i=0;
    for (i=0; i<man; i++){
        if (count ==pt->count){
            strcpy(sndmsg.msg, resmsg.msg);
            strcpy(sndmsg.filename,resmsg.filename);
            if(write(pt->soc,&sndmsg,sizeof(struct snd))>0){
                fprintf(stderr,"\n server return file deal success, signal:%s, name:%s", sndmsg.msg,sndmsg.filename);
            }else{
                fprintf(stderr, "\n server send fail:%s", strerror(errno));
            }
            break;
        }
        pt=pt->next;
    }
}

//sending file signal
void filesend(int count){
    struct linkid *pt= linkhead;
    int i =0;
    for (i=0; i<man;i++){
        if(count==pt->count){
        strcpy(sndmsg.msg, resmsg.msg);
        sndmsg.size=resmsg.size;
        strcpy(sndmsg.buffer, resmsg.buffer);
        if(write(pt->soc,&sndmsg,sizeof(struct snd))>0){
            fprintf(stderr,"\n server return file data success, signal:%s", sndmsg.msg);
        }else{
            fprintf(stderr,"\n server return failed:%s",strerror(errno));
        }
        break;
    }
    pt=pt->next;
    }
}

//file send finish
void fileend(int count){
    struct linkid *pt = linkhead;
    int i=0;
    for(i=0;i<man;i++){
        if(count==pt->count){
            strcpy(sndmsg.msg, resmsg.msg);
            if(write(pt->soc,&sndmsg,sizeof(struct snd))>0){
                fprintf(stderr,"\n server return file finish, signal:%s",sndmsg.msg);
            }else{
                fprintf(stderr,"\n server send fail:%s", strerror(errno));
            }
            break;
        }
        pt=pt->next;
    }
}

//online/offline notice
void newclientTIP(int count, char *data){
    char ccount[20]={'\0'};
    struct linkid *pt = NULL;
    pt = linkhead;
    int i;
    if (strcmp(data,"NEW")==0){
        sprintf(ccount, "new user %d is online", count);
    }else if (strcmp(data,"OUT")==0){
        sprintf(ccount, "user %d is offline", count);
    }
    strcpy(sndmsg.msg, ccount);
    for(i=0; i<man;i++){
        if(write(pt->soc,&sndmsg,sizeof(struct snd))>0){
            fprintf(stderr,"\n server return success:%s, id=%d",sndmsg.msg, pt->soc);
        }else {
            fprintf(stderr,"\n server sent fail:%s",strerror(errno));
        }
        pt=pt->next;
    }
}

//new user link list
void *createclienct(void *args){
    struct linkid *p, *pt, *phead=NULL;
    p= (struct linkid*)malloc(sizeof(struct linkid));
    p->count=clientcount;
    p->soc= new_fd;
    int i;
    if(linkhead==NULL){
        linkhead=p;
        linkhead->next=NULL;
    }else{
        phead = linkhead;
        for (i=0; i<man-1; i++){
            phead=phead->next;
        }
        phead->next=p;
        p->next=NULL;
    }
    ++man;
    char new[10]="NEW";
    fprintf(stderr,"\n user %d registered; SOC=%d; clientcount=%d",p->count,p->soc, clientcount);
    newclientTIP(p->count, new);
}

//create new thread to handle each client's message
void *handle_client(void *client_socket){
    int sock = *(int*)client_socket;
    free(client_socket);
    pthread_exit(NULL);
}


int main(int argc, char *argv[]){
    timecnt.tv_usec = 0;//initialize select wait time as 0
    timecnt.tv_sec = 0;

    if(argc!=2){
        fprintf(stderr, "usage:%s portnumber\a\n", argv[0]);
        exit(1);
    }
    if((portnumber = atoi(argv[1]))<=0){
        fprintf(stderr,"usage:%s portnumber\a\n", argv[1]);
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd<0){
        perror("Socket creation failed");
        exit(1);
    }

    //server fill sockaddr
    bzero(&server_addr, sizeof(struct sockaddr_in));
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_port=htons(portnumber);

    //server bind sockfd
    if(bind(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1){
        fprintf(stderr,"Bind error:%s\n\a",strerror(errno));
        close(sockfd);
        exit(1);
    }

    //server listen sockfd
    if(listen(socket, 5)==-1){
        fprintf(stderr, "listen error:%s\n\a", strerror(errno));
        close(sockfd);
        exit(1);
    }

    sin_size=sizeof(struct sockaddr_in);
    int flag =0;
    if (pthread_create(&id, NULL, send, NULL)==0){
        fprintf(stderr, "\n user send and receive threads created", NULL);
    }

    //block server until user connect
    while(1){
        if((new_fd=accept(sockfd,(struct sockaddr *)(&client_addr),&sin_size))!=-1){
            fprintf(stderr, "\n client connected", NULL);
        }else{
            fprintf(stderr, "accept error:%s\n\a", strerror(errno));
            exit(1);
        }
        fprintf (stderr,"server get connection from %s\n", inet_ntoa(client_addr.sin_addr));
        
        int *client_sock = malloc(sizeof(int));
        *client_sock = new_fd;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client_sock);
        pthread_detach(tid);

    }

    exit(0);
}