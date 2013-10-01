#include<stdio.h>
#include<fcntl.h>
#include<string.h>
#include<stdlib.h>
#include<signal.h>
#include<sys/types.h>
#include<unistd.h>
#include<regex.h>
#include<errno.h>
#include<time.h>

char *pdr = {0};
int i;
char hist_array[1024][1024] = {0};
char **array = NULL;
int pid_all[1024];
char running[1024] = {0};
int j = 0 , l = 0;
char in[100],out[100];
char **new_arr = NULL;
char **exec_array = NULL;
int r_flag , p_flag;
regex_t reg , reg1;
char command[1024];
char argument[1000];

void command_line(char *pdr) //prints the prompt
{
    char pwd[1024] , boot_dir[1024] ;
    getcwd(pwd,1024);
    strcpy(boot_dir,pdr);
    if(strstr(pwd , boot_dir) == pwd)
    {
        strcat(boot_dir,"/%[^\n]");
        char dir[1024] = {0} ;
        sscanf(pwd , boot_dir , dir);
        if(strlen(dir) == 0)
        {
            printf("<%s@%s:~>",getenv("USER"),getenv("HOSTNAME"));
        }
        else
        {
            printf("<%s@%s:~/%s>",getenv("USER"),getenv("HOSTNAME"),dir);
        }
    }
    else
    {
        printf("<%s@%s:%s>",getenv("USER"),getenv("HOSTNAME"),getenv("PWD"));
    }
}

void SIG_HANDLE() //handling ctrl+c signal
{
    printf("\n");
    command_line(pdr);
    fflush(stdout);
}

void SIG_CHLD() //handling SIGCHLD signal
{
    int status;
    pid_t cpid = waitpid(-1,&status,WNOHANG);
    int i;
    for(i=0;i<j;i++)
    {
        if(pid_all[i]==cpid && running[i]==1)
        {
            if(WIFEXITED(status))
            {
                running[i]=0;
                printf("\n%s %d exited normally\n",hist_array[i],pid_all[i]);
                command_line(pdr);
            }
            else if(WIFSIGNALED(status))
            {
                running[i]=0;
                printf("\n%s %d exited (signalled)\n",hist_array[i],pid_all[i]);
                command_line(pdr);
            }
#ifdef WCOREDUMP
            else if(WCOREDUMP(status))
            {
                running[i]=0;
                printf("\n%s %d exited (core dumped).\n",hist_array[i],pid_all[i]);
                command_line(pdr);
            }
#endif // WCOREDUMP
        }
    }
}

void check_reg1()
{
    while(regexec(&reg1,command,0,NULL,0) == 0)
    {
        strcpy(command,hist_array[atoi(command+5)-1]);
    }
}

void command_exec(char **exec_array)
{
    errno=0;
    if( strcmp( *exec_array , "pid" ) == 0)
    {
        if(exec_array[1] != NULL && strcmp(exec_array[1],"all") == 0)
        {
            int k;
            printf("List of currently executing processes spawned from this shell:\n");
            for( k = 0 ; k < j-1 ; k++ )
            {
                printf("command name: %s process id: %d\n",hist_array[k],pid_all[k]);
            }
        }
        else if(exec_array[1] != NULL && strcmp(exec_array[1],"current") == 0)
        {
            int k;
            printf("List of currently executing processes spawned from this shell:\n");
            for( k = 0 ; k < j-1 ; k++ )
            {
                if(running[k] == 1)
                {
                    printf("command name: %s process id: %d\n",hist_array[k],pid_all[k]);
                }
            }
        }
        else
        {
            printf("command name: %s process id: %d\n",argument,pid_all[l-1]);
        }
    }
    else if(regexec(&reg,*exec_array,0,NULL,0) == 0)
    {
        int k;
        if( strlen(*exec_array) > 4 )
        {  
            if(*exec_array[0] == '!')
            {
                printf("%s\n",*exec_array+5);
                printf("%s\n",hist_array[atoi(*exec_array+5) - 1]);
            }
            else if( atoi(*exec_array+4)  > j - 1 )
            {
                for( k = 0 ; k < j - 1 ; k++)
                {
                    printf("%d. %s\n",k+1,hist_array[k]);
                }

            }
            else
            {
                int l = 1 , d = j - atoi( *exec_array+4 ) - 1;
                for(k = d ; k < j-1 ; k++)
                {
                    printf("%d. %s\n",l,hist_array[k]);
                    l++;
                }
            }

        }
        else
        {
            for( k = 0 ; k < j - 1 ; k++)
            {
                printf("%d. %s\n",k+1,hist_array[k]);
            }

        }
    }
    else
    {
        int retval = execvp(exec_array[0],exec_array);
        perror(exec_array[0]);
        exit(retval);
    }
    exit(errno);
}
char temp[5] = "temp";

/*void cpy_file(char *temp, char *out)
{
    char ch;
    FILE *fdr = fopen(out,"r");
    FILE *fdw = fopen(temp,"w");
    while(fscanf(fdr,"%c",ch) != EOF)
        fprintf(fdw,"%c",ch);
    fclose(fdr);
    fclose(fdw);
}*/

void my_pipe(char *in , char *out , int r)
{
    int last_valid = i;
    while(last_valid > 0 && new_arr[last_valid - 1] == NULL)
        --last_valid;
    if( strcmp( *new_arr , "cd" ) == 0)
    {
        if(r==0)
        {
            pid_all[l] = getpid();
            l++;
        }
        char a[1024];
        int s = 0;
        getcwd(a,1024);
        if(new_arr[1] == NULL)
        {
            s = chdir(pdr);
        }
        else if(strcmp(new_arr[1] , "~") == 0)
        {
            s = chdir(pdr);
        }
        else if(strcmp(new_arr[1] , "-") == 0)
        {
            char* old = getenv("OLD_PWD");
            if(old)
            {
                s = chdir(old);
            }
            else
            {
                s = -2;
                fprintf(stderr,"cd: OLDPWD not set\n");
            }
        }
        else
        {
            s = chdir(new_arr[1]);
        }
        if(s == 0)
        {
            setenv("OLD_PWD",a,1);
        }
        else if(s == -1)
        {
            perror("cd");
        }
        return;
    }
    int isbg = 0;
    if(strcmp(new_arr[last_valid-1],"&") == 0)
    {
        isbg = 1;
        new_arr[last_valid-1] = NULL;
    }
    pid_t pid;
    pid = fork();
    if(pid == 0)
    {
        if(in != "")
        {
            int fdr = open(in,O_RDONLY,0666);
            dup2(fdr , STDIN_FILENO);
        }
        if(p_flag == 1 )
        {
            int fdw = open(out,O_CREAT|O_WRONLY|O_TRUNC,0666);
            dup2(fdw , STDOUT_FILENO);
        }
        else if(out != "")
        {
            int fdw = open(out,O_CREAT|O_WRONLY|O_TRUNC,0666);
            dup2(fdw , STDOUT_FILENO);
        }
        check_reg1();
        command_exec(new_arr);
    }
    else if(pid > 0)
    {
        pid_all[l] = pid;
        l++;
        if(!isbg)
        {
            waitpid(pid, NULL, 0);
        }
        else
        {
            running[j-1]=1;
            fprintf(stderr,"command %s pid %d\n",exec_array[0],pid);
        }
        strcpy(in,out);
//        cpy_file(temp,out);
//        FILE *fdr = fopen("temp","r");
//        char ch;
//        while(ch = fgetc(fdr) != EOF)
//            putc(ch,stdout);
 
//        strcpy(in,temp);
        strcpy(out , "");
    }
    else
    {
        perror(argument);
    }
}

int main(int argc, char* argv[])
{
    signal(SIGTSTP,SIG_IGN);
    signal(SIGINT,SIG_HANDLE);
    signal(SIGCHLD, SIG_CHLD);
    pdr = getenv("PWD");
    command_line(pdr);
    command[0] = '\0';
    int pipe_count;
    while(scanf("%[^\n]",command) != EOF && strcmp(command,"quit") != 0)
    {
        p_flag = 0 , r_flag = 0;
        getchar();
        if(command[0] == '\0' )
        {
            command_line(pdr);
            continue;
        }
        pipe_count = 0;
        strcpy(hist_array[j] , command);
        j++;
        regcomp(&reg1,"^!hist[0-9]*$",REG_EXTENDED);
        regcomp(&reg,"^hist[0-9]*$",REG_EXTENDED);
        check_reg1();
        i = 0;
        array = calloc(1024,sizeof(char *));
        char *word = strtok(command," \t");
        while(word != NULL)
        {
            array[i] = word;
            if(strcmp(array[i] , "|") == 0)
                p_flag = 1;
            if(strcmp(array[i] , "<") == 0 || strcmp(array[i] , ">") == 0)
                r_flag = 1;
            word = strtok(NULL , " \t");
            i++;
        }
        array[i] = NULL;
        strcpy(in,""); 
        strcpy(out,"");
        int count = 0 , a_count = 0;
        strcpy(argument,argv[0]);
        new_arr = calloc(1024,sizeof(char *));
        while(count != i)
        {
            if(strcmp(array[count] , "<") == 0)
            {
                strcpy(in , array[++count] );
            }
            else if(strcmp(array[count] , ">") == 0)
            {
                strcpy(out , array[++count]);
            }
            else if(strcmp(array[count] , "|") == 0)
            {
                p_flag=1;
                new_arr[a_count] = NULL;
                fflush(stdout);
                strcpy(out,"/tmp/dummy");
                my_pipe(in,out,pipe_count);
                pipe_count++;
                fflush(stdout);
                a_count = 0;
                p_flag=0;
            }
            else
            {
                new_arr[a_count] = array[count];
                a_count++;
            }
            count++;
        }
        new_arr[a_count] = NULL;
        fflush(stdout);
        my_pipe(in,out,0);
        command_line(pdr);
        command[0] = '\0';
    }
    if(strcmp(command,"quit"))
    {
        printf("\n");
    }
    return 0;
}
