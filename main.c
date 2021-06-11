/*	Mustafa Saglam
    150140129
    BLG322E-HW3
    22.05.19 Tue
*/
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>

typedef struct Boxes{
	int count;
	char* colorlist;
    char colorque[6];
}Box;
 
Box readInput(const char* filepath);
void writeOut(const char* filepath, const pid_t pid, char color);

int main(int argc, char* argv[]){
    char inputfile[25], outputfile[25];
    if(argc == 3){
        strcpy(inputfile, argv[1]);
        strcpy(outputfile, argv[2]);//if argvs exist
    }
    else{
        strcpy(inputfile, "input.txt");
        strcpy(outputfile, "output.txt");//if argvs do not exist
    }

    int i, j;
    pid_t pidF;
    int shmid, *shm;
    Box *boxdata, boxes = readInput(inputfile);//read input.txt

    key_t keyB = ftok("dev/null", argc);//creating a rondow key_t value for shared memory

    if ((shmid = shmget(keyB, 1024, IPC_CREAT | 0666)) < 0) {
        perror("shmget error"); //defining and creating the shared memory
        exit(1);
    }

    boxdata = shmat(shmid, NULL, 0);//shared mem attach
    if (boxdata == (Box *)(-1)) {
        perror("shmat error");
        exit(1);
    }
    *boxdata = boxes;

    writeOut(outputfile, 6, ' ');//<- is not this always 6. there are 6 colors in total

    sem_t *sem = sem_open("pSem", O_CREAT | O_EXCL, 0644, boxes.count);//semaphore create

    printf("SIMULATION BEGINS\n");

    for(i=0, j=0; i<boxes.count; ++i){
        pidF = fork();//fork enough child processes
        if(pidF < 0){
            sem_unlink("pSem");//clean semaphore
            sem_close(sem);
            printf("fork error");
            exit(-1);
        } 
        else if(pidF == 0)
            break;
    }

    pid_t pidC = 0;//current pid of current child
    if (pidF != 0){//parent process
        while (pidF = waitpid(-1, NULL, 0)){
            if (errno == ECHILD)
                break;
        }

        printf ("\nParent: All children have exited.\n");

        shmdt(boxdata); //shared memory detach 
        shmctl(shmid, IPC_RMID, 0);
        
        sem_unlink("pSem");//clean and cluse semaphore
        sem_close(sem);
        
        exit(0);
    }

    else{ //child process
        sem_wait(sem);//wait semaphore
        char color = boxdata->colorlist[i];
    
        while(color != boxdata->colorque[j]){
            waitpid(pidC, NULL, 0);//wait if its not your turn to paint
            if(i == boxdata->count-1){
                j++;
            } 
        } 
            
        printf(" %c ", color);
        pidC = getpid();
        writeOut(outputfile, pidC, color);//write result as child
        sleep(1);
        sem_post(sem);//send semaphore to other child

        exit(0);
    }

	return 0;
}

Box readInput(const char* filepath){
    FILE* fin = fopen(filepath, "r");//read file
    if(!fin) exit(-1);
    
    Box toRet;
    fscanf(fin, "%d\n", &(toRet.count));//read count
    
    toRet.colorlist = (char*)malloc(toRet.count);
    if(!toRet.colorlist) exit(-2);
    
    int i, j, m = 0;
    for(i=0; i<toRet.count; ++i)//read colors
        fscanf(fin, "%c\n", &(toRet.colorlist[i]));
    
    toRet.colorque[0] = toRet.colorlist[0];//create que for first come first serve
    for(i=0; i<6; i++){
        for(j=0; j<m; j++)
            if(toRet.colorlist[i]==toRet.colorque[j])
                break;
        if(j==m){
            toRet.colorque[m]=toRet.colorlist[i];
            m++;
        }
    }

    fclose(fin);
    return toRet;
}

void writeOut(const char* filepath, const pid_t pid, char color){
    FILE* fout;//file
    if(color == ' ')//write for main process
        fout = fopen(filepath, "w");
    else fout = fopen(filepath, "a");//append for child processes
    
    if(!fout) exit(-1);

    fprintf(fout, "%i %c\n", pid, color);//->write
    fclose(fout);
}
