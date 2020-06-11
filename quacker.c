/* 
*  CIS 415 - Jack Sanders
*  Notes: Discussed logic with Stephanie Schofield.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define RANDOMDIVISOR	100000000
#define TRUE 1
#define URLSIZE 64
#define CAPSIZE 64
#define	ENTRYLENGTH	80
#define MAXENTRIES 100
#define	NUMENTRIES 64
#define	BUFFERSIZE 1024
#define NUMBUFFERS 64
#define MAXPUBS 64
#define MAXSUBS	64
#define NUMPROXYS 64
#define BUFFERSIZE 1024
int cleanup_flag = 0;
int delta = 0;

typedef struct topicEntry {
	int entryNum;
	struct timeval timeStamp;
	int pubID;
	char photoURL[URLSIZE];
	char photoCaption[CAPSIZE];
} topicEntry; 

typedef struct TEQ {
  int TEQ_ID; 
  char name[BUFFERSIZE];
  topicEntry *buffers;
  int entry;
  int head;
  int tail;
  int length;
  pthread_mutex_t lock;
} TEQ; 

typedef struct threadargs {
  int id;
  int lastEntries[MAXENTRIES];
  pthread_t threadID;
} threadargs;

TEQ topics[MAXENTRIES];
threadargs publisher_pool[NUMPROXYS];
threadargs subscriber_pool[NUMPROXYS];

pthread_t pubs[MAXPUBS];    // thread ID for publishers
pthread_t subs[MAXSUBS];    // thread ID for subscribers
pthread_attr_t  attr;     // thread attributes

pthread_mutex_t mutex[NUMBUFFERS];  // mutex locks for buffers

void *publisher(void *threadargs);
void *subscriber(void *threadargs);

void initialize(char *TEQname, int len, int pos, int id) { 

  topics[pos].TEQ_ID = id;
  strcpy(topics[pos].name, TEQname);
  topics[pos].buffers = (topicEntry *)malloc(sizeof(topicEntry)*(len+1));
  topics[pos].head = 0;
  topics[pos].tail = 0;
  topics[pos].length = len;
  topics[pos].entry = 1;
  pthread_mutex_init(&(topics[pos].lock), NULL);

} 

void freeTEQ(int pos, char *TEQ_ID) { 
    free(topics[pos].buffers);
}

int enqueue(int id, topicEntry *entry) {
  int ret = 0;
  int i, flag = 0;

  for(i = 0; i < MAXENTRIES; i++) {
    if(id == topics[i].TEQ_ID) { 
      flag = 1; 
      break; 
    }
  }

  if(flag) {
    int tail = topics[i].tail;
    if(topics[i].buffers[tail].entryNum != -1) {
      topics[i].buffers[tail].pubID = entry->pubID;
      strcpy(topics[i].buffers[tail].photoURL, entry->photoURL);
      strcpy(topics[i].buffers[tail].photoCaption, entry->photoCaption);
      gettimeofday(&topics[i].buffers[tail].timeStamp, NULL);
      topics[i].buffers[tail].entryNum = topics[i].entry;
      topics[i].entry++;
      if (tail == topics[i].length) {
        topics[i].tail = 0; 
      } 
      else { 
        topics[i].tail++; 
      }
      ret = 1;
    }
  }
  return ret;
} 

int dequeue(int buffid, double delta) {
  if (topics[buffid].length == 0) {
    return -1;
  }

  topics[buffid].length--;
  topics[buffid].head = (topics[buffid].head +1) % BUFFERSIZE;
  return 0;
}

int getEntry(int lastEntry, topicEntry *recent, int id) {

  int ret = 0;
  int i, flag = 0;

  for (i = 0; i < MAXENTRIES; i++) {
    if (id == topics[i].TEQ_ID) {
      flag = 1;
      break;
    }
  }

  if(flag) {
    int head = topics[i].head;
    int tail = topics[i].tail;

    if(head != tail) {
      for (int j = head; j == tail; j++) {
        if(j == topics[i].length) { 
          j = 0;
        }
        if (topics[i].buffers[j].entryNum == (lastEntry + 1)) {
          recent->pubID = topics[i].buffers[tail].pubID;
          strcpy(recent->photoURL, topics[i].buffers[tail].photoURL);
          strcpy(recent->photoCaption, topics[i].buffers[tail].photoCaption);
          recent->timeStamp = topics[i].buffers[tail].timeStamp;
          topics[i].entry = topics[i].buffers[tail].entryNum;
          return 1;

        } else if (topics[i].buffers[j].entryNum > (lastEntry + 1)) {
          recent->pubID = topics[i].buffers[tail].pubID;
          strcpy(recent->photoURL, topics[i].buffers[tail].photoURL);
          strcpy(recent->photoCaption, topics[i].buffers[tail].photoCaption);
          recent->timeStamp = topics[i].buffers[tail].timeStamp;
          topics[i].entry = topics[i].buffers[tail].entryNum;
          return topics[i].buffers[tail].entryNum;
        }
      }
    }
  }
  return ret;
}

void *publisher(void *args) {
    int bid, tid; // buffer id and thread id
    int randomnum;  // random number

    tid = ((struct threadargs *) args)->id;
    fprintf(stdout, "publisher thread id = %d, %ld\n", tid, pthread_self());

    while(TRUE) {
      for (bid = 0; bid < NUMBUFFERS; bid++) {
        pthread_mutex_lock(&(mutex[bid]));
        if (enqueue(bid, topicEntry) == -1) {
          fprintf(stdout, "publisher %d: buffer %d is full, inserted = %d\n", tid, bid, buffers[bid].inserted);
          pthread_mutex_unlock(&(mutex[bid]));
          sleep(1);
        }
        else {
          pthread_mutex_unlock(&(mutex[bid]));
        }
      }
    }

  return 0;

}

void *subscriber(void *args) {

  int bid, tid; // buffer id and thread id
    int randomnum;  // random number
  
    tid = ((struct threadargs *) args)->id;
    fprintf(stdout, "subscriber thread id = %d, %ld\n", tid, pthread_self());
  
    while(TRUE) {
    
    for (bid=0; bid<NUMBUFFERS; bid++) {
      pthread_mutex_lock(&(mutex[bid]));

      if (dequeue(bid, delta) == -1) {
        fprintf(stdout, "subscriber %d: buffer %d is empty, inserted = %d\n", tid, bid, buffers[bid].inserted);
        pthread_mutex_unlock(&(mutex[bid]));
        sleep(1);
      }
      else {
        pthread_mutex_unlock(&(mutex[bid]));
      }
    }
  }

}

// -------------------------------------------------------- //
// parts 3 and 4

int readPub(char *argv) {
  const char *character = " ";
    const char *filename = argv;
    FILE *input_file = fopen( filename, "r" );
    char buffer[ BUFFERSIZE ];
    char *token;

    if (input_file == NULL ){
        fprintf(stderr, "Unable to open file %s\n", filename);
    }
    else{
        while( fgets(buffer, BUFFERSIZE, input_file) != NULL ){
            token = strtok( buffer, character );
            char *line[BUFFERSIZE];
            int i = 0;
            while( token != NULL ){
                if (token[strlen(token) - 1] == '\n')
                  token[strlen(token) - 1] = '\0';
                line[i] = token;
                token = strtok( NULL, character );
                i++;
            }

            if (strncmp(line[0], "put", 3) == 0) {
              printf("put topic entry: <%s> <%s> <%s>\n", line[1], line[2], line[3]);
            }
            if (strncmp(line[0], "sleep", 5) == 0){
              int sleep_length = atoi(line[1]);
              sleep(sleep_length/1000);
              printf("The publisher will sleep for <%d> milliseconds\n", sleep_length);
            }
            if (strncmp(line[0], "stop", 4) == 0) {
              printf("The publisher threadargs stops reading commands and the threadargs is returned to the respective pool.\n");
            }
        }

        if( ferror(input_file)) {
            perror( "The following error occurred" );
        }
        fclose( input_file );
    }
    return 0;
}

int readSub(char *argv) {
  const char *character = " ";
    const char *filename = argv;
    FILE *input_file = fopen( filename, "r" );
    char buffer[BUFFERSIZE];
    char *token;

    if( input_file == NULL ){
        fprintf(stderr, "Unable to open file %s\n", filename);
    }

    else {
        while (fgets(buffer, BUFFERSIZE, input_file) != NULL ){
            token = strtok( buffer, character );
            char *line[BUFFERSIZE];
            int i = 0;
            while( token != NULL ){
                if (token[strlen(token) - 1] == '\n')
                  token[strlen(token) - 1] = '\0';
                line[i] = token;
                token = strtok( NULL, character );
                i++;
            }
            if (strncmp(line[0], "get", 3) == 0) {
              printf("get topic entry: <%s>\n", line[1]);
            }
            if (strncmp(line[0], "sleep", 5) == 0){
              int sleep_length = atoi(line[1]);
              sleep(sleep_length/1000);
              printf("The subscriber will sleep for <%d> milliseconds\n", sleep_length);
            }
            if (strncmp(line[0], "stop", 4) == 0) {
              break;
            }
        }
        fclose( input_file );
    }
    return 0;
}

int main(int argc, char *argv[]) {

    const char *character = " ";
    const char *filename = argv[1];
    FILE *input_file = fopen(filename, "r" );
    char buffer[BUFFERSIZE];
    char *token;
    char pub[40];
    char *pubFile;
    char sub[40];
    char *subFile;

    if (input_file == NULL ){
        fprintf(stderr, "Unable to open file %s\n", filename);
    }

    else{
        while( fgets(buffer, BUFFERSIZE, input_file) != NULL ){
            token = strtok(buffer, character);
            char *line[BUFFERSIZE];
            int i = 0;

            if (token[strlen(pub) - 1] == '\n')
                token[strlen(pub) - 1] = '\0';

            while (token != NULL ){
                if (token[strlen(token) - 1] == '\n')
                  token[strlen(token) - 1] = '\0';
                line[i] = token;
                token = strtok(NULL, character);
                i++;
            }

            if ((strncmp(line[0], "create", 6) == 0) && (strncmp(line[1], "topic", 5) == 0)) {
              printf("create topic: <%s> <%s> <%s>\n", line[2], line[3], line[4]);
            }
            if ((strncmp(line[0], "query", 5) == 0) && (strncmp(line[1], "topics", 6) == 0)) {
              printf("Print out all topic IDs and their lengths\n");
            }
            if ((strncmp(line[0], "add", 3) == 0) && (strncmp(line[1], "publisher", 10) == 0)) {
              strcpy(pub, line[2]);
              if (pub[strlen(pub) - 1] == '"')
                  pub[strlen(pub) - 1] = '\0';
                pubFile = pub + 1;
              printf("Create publisher <%s>\n", pubFile);
            }
            if ((strncmp(line[0], "query", 5) == 0) && (strncmp(line[1], "publishers", 11) == 0)) {
              printf("Print out current publishers and their command file names\n");
            }
            if ((strncmp(line[0], "add", 3) == 0) && (strncmp(line[1], "subscriber", 10) == 0)) {
              strcpy(sub, line[2]);
              if (sub[strlen(sub) - 1] == '"')
                  sub[strlen(sub) - 1] = '\0';
                subFile = sub + 1;
              printf("Create subscriber <%s>\n", subFile);
            }
            if ((strncmp(line[0], "query", 5) == 0) && (strncmp(line[1], "subscribers", 11) == 0)) {
              printf("Print out subscribers and their command file names\n");
            }
            if (strncmp(line[0], "delta", 5) == 0) {
              printf("Set DELTA to <%s>\n", line[1]);
            }
            if (strncmp(line[0], "start", 5) == 0) {
              printf("Start all of the publishers and subscribers\n");
              readPub(pubFile);
            readSub(subFile);
            }
        }
        fclose( input_file );
    }
    return 0;
}








