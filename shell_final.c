#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/resource.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#define COMMAND_SIZE 50

int plofand = 0;
int size = 0;
int p_size = 0;
int shell = 1;
int buf = 1024;
char *paths[50];
pid_t pid;
int a = 0;
int status;
char cwd[PATH_MAX];
int isCused = 0;
int isMused = 0;
int isBused = 0;
int isCMused = 0;
int isCBused = 0;
int isMBused = 0;
int resLim = 0;
int cpuLim = 2;
int virLim = 16000000;
int stkLim = 1000000;
int pos = 0;

int commandCd(char *args[]) {

    char *cmd = args[0];

    if (strcmp(cmd, "cd") == 0) {

        if (size < 2) {
            args[1] = "/home";
        }
        if (chdir(args[1]) != 0) {
            perror("ERROR");
        }
        return 0;
    }
    return 1;
}

int breakIntoTokens(char *line, char *args[]) {
    char *str;
    size = 0;
    str = strtok(line, " \t");
    if (strcmp(str, "\n") == 0)
        return 0;
    while (str != NULL) {
        if (str[strlen(str) - 1] == '\n') {
            str[strlen(str) - 1] = '\0';
        }

        if (strcmp(str, "\n") != 0 && strcmp(str, "") != 0 && strcmp(str, "\t") != 0 && strcmp(str, " ") != 0 &&
            str != NULL) {
            args[size++] = str;
        }

        str = strtok(NULL, " \t");
    }
    args[size] = NULL;
    return 1;
}


void setPath() {

    char *p = getenv("PATH");
    char *str;
    str = strtok(p, ":");
    while (str != NULL) {
        paths[p_size++] = str;
        str = strtok(NULL, ":");
    }
}

char *fullPath(char *first) {

    int i;
    char *p;
    for (i = 0; i < p_size; i++) {
        p = NULL;
        p = (char *) malloc(strlen(paths[i]) + strlen("/") + strlen(first) + 1);
        strcpy(p, paths[i]);
        strcat(p, "/");
        strcat(p, first);

        int acc = access(p, F_OK);

        if (acc != -1) {
            return p;
        }
    }

    return first;
}


int backgroundProcess(char *args[]) {

    char *ending = args[size - 1];
    int len = strlen(ending);
    if (len == 1) {
        if (ending[0] == '&') {
            args[size - 1] = NULL;
            size--;
            return 1;
        }
    }
    return 0;
}

void print_cpu_time(struct rusage* usage) {
    printf("CPU time: %ld.%09ld sec user, %ld.%09ld sec system\n",
           usage->ru_utime.tv_sec, usage->ru_utime.tv_usec,
           usage->ru_stime.tv_sec, usage->ru_stime.tv_usec);
}

void print_memory_usage(struct rusage* usage) {

        printf("Size of the operating residential memory used by the process: %ld\n", usage->ru_maxrss);

}


void print_block_usage(struct rusage *usage) {
        printf("Num of input blocks used by the process: %ld\n", usage->ru_inblock);
        printf("Num of input blocks used by the process: %ld\n", usage->ru_oublock);

}


void change_CPU_limit(struct rlimit rl){
    getrlimit (RLIMIT_CPU, &rl);
    //printf("\n Default CPU value is : %lld\n", (long long int)rl.rlim_cur);

    rl.rlim_cur = cpuLim;
    setrlimit (RLIMIT_CPU, &rl);
    getrlimit (RLIMIT_CPU, &rl);
    //printf("\n Updated CPU value now is : %lld\n", (long long int)rl.rlim_cur);
}

void change_Mem_limit(struct rlimit rl){

    // First get the limit on memory
    getrlimit (RLIMIT_AS, &rl);
    //printf("\n Default VM value is : %lld\n", (long long int)rl.rlim_cur);

    rl.rlim_cur = virLim;

    setrlimit (RLIMIT_AS, &rl);
    getrlimit (RLIMIT_AS, &rl);
    //printf("\n Updated Vm value now is : %lld\n", (long long int)rl.rlim_cur);

    char *ptr = NULL;
    ptr = (char*) malloc(1024);
    if(NULL == ptr)
    {
        perror("\n Memory allocation failed\n");
        EXIT_FAILURE;
    }
    free(ptr);
}

void change_Stk_limit(struct rlimit rl){
    getrlimit (RLIMIT_STACK, &rl);
    //printf("\n Default stask limit is : %lld\n", (long long int)rl.rlim_cur);

    rl.rlim_cur = stkLim;
    setrlimit (RLIMIT_STACK, &rl);
    getrlimit (RLIMIT_STACK, &rl);
    //printf("\n Updated stack value now is : %lld\n", (long long int)rl.rlim_cur);
}



int executeCommand(char *args[]) {
    struct rusage usage;
    struct rlimit rl;
    int flag = backgroundProcess(args);

    pid = fork();

    if (pid < 0) {
        printf("ERROR");
    } else if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        if (execv(fullPath(args[0]), args) == -1) {
            printf("%s: command not found\n", args[0]);
        }
        exit(EXIT_FAILURE);
    } else if (flag == 0) {
        //waitpid(pid, &status, 0);
        wait4(pid, &status, 0, &usage);
        change_CPU_limit(rl);
        change_Mem_limit(rl);
        change_Stk_limit(rl);
        if(isCused ==1 && isMused == 1 && isBused ==1){
            print_cpu_time(&usage);
            print_memory_usage(&usage);
            print_block_usage(&usage);
        } else if(isCMused ==1){
            print_cpu_time(&usage);
            print_memory_usage(&usage);
        } else if(isCBused ==1){
            print_cpu_time(&usage);
            print_block_usage(&usage);
        } else if(isMBused ==1){
            print_memory_usage(&usage);
            print_block_usage(&usage);
        } else if(isCused ==1){
            print_cpu_time(&usage);
        } else if(isBused ==1){
            print_block_usage(&usage);
        } else if(isMused ==1){
            print_memory_usage(&usage);
        }
        return 1;
    }

}

void combineCheck(int s, char *args[]) {

    plofand = s;

    int flag = commandCd(args);

    if (flag == 1) {
        execv(fullPath(args[0]), args);
    }
}

void executeAnd(int i, char *args[]) {

    char *arr1[100];
    char *arr2[100];

    int m;

    for (m = 0; m < i; m++) {
        arr1[m] = args[m];

    }

    arr1[m] = NULL;
    int j, n = 0;

    for (j = i + 1; j < size - a; j++) {
        arr2[n] = args[j];
        n++;
    }

    arr2[n] = NULL;
    if ((pid = fork()) == -1) {
        printf("ERROR");
    } else if (pid == 0) {

        combineCheck(i, arr1);

    } else if (pid > 0)
        waitpid(pid, &status, 0);

    int check = 0;
    int k = 0;
    for (k = 0; k < n; k++) {
        if (strcmp(arr2[k], "&&") == 0) {
            a += i + 1;
            check = 1;
            break;
        }
    }

    if (check == 1) {
        executeAnd(k, arr2);
    } else {
        if ((pid = fork()) == -1) {
            printf("ERROR");
        } else if (pid == 0) {
            combineCheck(k, arr2);
        } else if (pid > 0)
            waitpid(pid, &status, 0);
    }
}

void signalHandler(int signo) {

    if (signo == SIGINT) {
        printf("\n");
    } else if (signo == SIGQUIT) {
        printf("\n");
    }

}

int main(int argc, char *argv[]) {
    struct rlimit rl;
    if(argc>1) {
        if (strstr(argv[1], "c") != 0 && strstr(argv[1], "m") != 0 && strstr(argv[1], "-") != 0) {
//            printf("This is mc\n");
            isCMused = 1;
        } else if (strstr(argv[1], "c") != 0 && strstr(argv[1], "b") != 0 && strstr(argv[1], "-") != 0) {
//            printf("This is cb\n");
            isCBused = 1;
        } else if (strstr(argv[1], "m") != 0 && strstr(argv[1], "b") != 0 && strstr(argv[1], "-") != 0) {
//            printf("This is mb\n");
            isMBused = 1;
        } else if (strstr(argv[1], "-c") != 0) {
//            printf("This is c\n");
            isCused = 1;
        } else if (strstr(argv[1], "-m") != 0) {
//            printf("This is m\n");
            isMused = 1;
        } else if (strstr(argv[1], "-b") != 0) {
//            printf("This is b\n");
            isBused = 1;
        } else if (strstr(argv[1], "-a") != 0) {
            isCused = 1;
            isMused = 1;
            isBused = 1;
//            printf("This is all\n");
        }
        if(strstr(argv[1], "-r")!=0){
            resLim = 1;
            pos = 1;
        }
    }
    if(argc>2) {
        if (strstr(argv[2], "-r") != 0) {
            resLim = 1;
            pos = 2;
        }
    }

    if(resLim==1){
        int i = 0;
//        cpuLim = argv[pos+1];
//        virLim = argv[pos+2];
//        stkLim = argv[pos+3];
        sscanf(argv[pos+1], "%d", &cpuLim);
        sscanf(argv[pos+2], "%d", &virLim);
        virLim*=1000000;
        sscanf(argv[pos+3], "%d", &stkLim);
        stkLim*=1000000;
//        printf("cpu limit %d\n", cpuLim);
//        printf("vm lim %d\n", virLim);
//        printf("stk lim %d\n", stkLim);
        //printf("bu cpu limdir %s\n", argv[pos +1]);
    }

    //printf("Res lim var %d\n", resLim);

    char *args[COMMAND_SIZE / 2 + 1];

    setPath();

    char *com = malloc(COMMAND_SIZE);
    char *com2 = malloc(80 * COMMAND_SIZE);

    signal(SIGINT, signalHandler);
    signal(SIGQUIT, signalHandler);

    while (shell) {
        getcwd(cwd, sizeof(cwd));
        if (strcmp(com2, "") == 0 && strcmp(args[size - 1], "\\") != 0) printf("%s$ ", cwd);
        char *line = malloc(COMMAND_SIZE + 1);
        fgets(line, buf, stdin);

        if (strcmp(line, "\n") == 0 || strcmp(line, "\r\n") == 0) {
            continue;
        }


        strcat(com2, " ");
        strcat(com2, line);
        strcat(com, com2);

        breakIntoTokens(com, args);

        com = "";
        com = malloc(10 * COMMAND_SIZE);
        if ((strcmp(args[0], "exit") == 0) || strcmp(args[0], "quit") == 0) {
            kill(0, SIGTERM);
        }
        if (strcmp(args[size - 1], "\\") == 0) {
            com2 = "";
            com2 = malloc(10 * COMMAND_SIZE);

            for (int i = 0; i < size - 1; i++) {
                strcat(com2, " ");
                strcat(com2, args[i]);
            }
            printf(">");
            continue;
        }

        int check = 0;
        int andand = 0;

        for (int i = 0; i < size; i++) {

            if (strcmp(args[i], "&&") == 0) {
                a = 0;
                andand = 1;
                check = 1;
                executeAnd(i, args);
                break;
            }
        }
        int flag = commandCd(args);

        if (flag == 1 && check == 0) {
            if (executeCommand(args) == -1) {
                printf("%s : command not found!!\n", com);
                continue;
            }
        }
        com = malloc(10 * COMMAND_SIZE);
        com2 = malloc(10 * COMMAND_SIZE);
    }
    return 0;
}

#pragma clang diagnostic pop
