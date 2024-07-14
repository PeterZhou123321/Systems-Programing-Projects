/**
 * Notes:
 * We call single command as command, single argument in two space called argument
 * For example: foo > bar | barz
 * Here we have 5 arguments in one single command
 * while loop is for reading commands
 * 
 * argument=token
*/
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<stdbool.h>
#include<signal.h>
#include<glob.h>//For wildcard
#include<time.h>

#define COMMAND_LENGTH 100
#define ARGUMENT_LENGTH 20
#define SINGLE_COMMAND_LENGTH 50

#define DEBUGMODE false

void stopProgram(int a)
{
    printf("Shell exit Success\n");
    exit(EXIT_SUCCESS);
}

int main(int args,char** argv)
{
    int batchModeFile=-2;//-2 for initial value, -1 for error
    //determine which mode currently run
    
    if(args==1)//Interactive mode
    {
        printf("Welcome to my shell!\n");   
    }
    else//Batch mode
    {
        batchModeFile=open(argv[1],O_RDONLY);
        dup2(batchModeFile,STDIN_FILENO);//Redirct input to the sh file
        close(batchModeFile);
    }

    bool lastCommandSuccess=true;

    //Handle both mode in a single loop
    while(true)
    {
        int pipeIndex=-1;//-1 indicate no pipe
        char* command=calloc(COMMAND_LENGTH,sizeof(char));
        char** arguments=calloc(ARGUMENT_LENGTH,sizeof(char*));
        arguments[0]=calloc(SINGLE_COMMAND_LENGTH,sizeof(char));
        //Get next command
        if(isatty(STDIN_FILENO))//When run in Interactive mode
        {
            printf("\nmysh> ");
            fflush(stdout);
            read(STDIN_FILENO,command,COMMAND_LENGTH);  
        }
        else//When run in Batch mode
        {
            char temp=' ';
            int index=0;
            while(temp!='\n')
            {
                int readByte=read(STDIN_FILENO,&temp,1);
                if(readByte==0&&index==0)
                {
                    free(command);
                    free(arguments[0]);
                    free(arguments);
                    return 0;
                }
                command[index]=temp;
                index++;
            }
        }
        //Divide commands into parts, store them in arguments array
        int arguIndex=0;//Index for n-th argument
        int arguCommandIndex=0;//Index for single argument
        for(int i=0;i<COMMAND_LENGTH;i++)
        {
            if(command[i]=='\n'||command[i]==0)
            {
                break;
            }
            else if(command[i]!=' '&&command[i]!='\n'&&command[i]!=0&&command[i]!='|')
            {
                arguments[arguIndex][arguCommandIndex]=command[i];
                arguCommandIndex++;
            }
            else if(command[i]==' ')
            {
                arguIndex++;
                arguCommandIndex=0;
                arguments[arguIndex]=calloc(SINGLE_COMMAND_LENGTH,sizeof(char));
            }
            else if(command[i]=='>'||command[i]=='<'||command[i]=='|')
            {
                arguIndex++;
                arguCommandIndex=0;
                arguments[arguIndex]=calloc(SINGLE_COMMAND_LENGTH,sizeof(char));
                
                arguIndex++;
                arguments[arguIndex]=calloc(SINGLE_COMMAND_LENGTH,sizeof(char));

                if(command[i]=='|')
                {
                    pipeIndex=arguIndex;
                }
            }

        }
        //Now all part of command stored into arguments array, free command array
        //Amount of arguments is just argument index plus 1
        int argumentAmount=arguIndex+1;
        int originalArgumentAmmount=argumentAmount;
        

        /**
         * Control command run or not:
         * test if there is else, then statement at the beginning
        */
        if(strcmp(arguments[0],"then")==0&&!lastCommandSuccess)
        {
            goto freePart;
        }
        else if(strcmp(arguments[0],"else")==0&&lastCommandSuccess)
        {
            goto freePart;
        }

        /**
         * Two big case: pipe need or not
         * If doesn't need, consider that command as simple
        */
        if(pipeIndex!=-1)//There is pipeline in the command, have to connect two diff program
        {
            int inputPipe[2];
            int firstArgumentIndex=0;
            if(pipe(inputPipe)==-1)
            {
                if(DEBUGMODE)  printf("error when creating pipe\n");
                lastCommandSuccess=false;
                goto freePart;
            }
            if(strcmp(arguments[0],"then")==0||strcmp(arguments[0],"else")==0)
            {
                firstArgumentIndex=1;
            }
            pid_t secondPartThread=fork();
            if(secondPartThread==-1)
            {
                if(DEBUGMODE) printf("Error when creat process\n");
                lastCommandSuccess=false;
                exit(EXIT_FAILURE);
            }
            else if(secondPartThread==0)//Child process, the second part of pipeline process
            {
                close(inputPipe[1]);
                firstArgumentIndex=pipeIndex+1;
                if(strcmp(arguments[firstArgumentIndex],"cd")==0)
                {
                    glob_t files;
                    int res=glob(arguments[firstArgumentIndex+1],GLOB_NOCHECK,NULL,&files);
                    if(res==GLOB_ABORTED)
                    {
                        if(DEBUGMODE) printf("glob resulting in error\n");//Only for debug use
                        lastCommandSuccess=false;
                        globfree(&files);
                        exit(EXIT_FAILURE);
                    }
                    if(chdir(files.gl_pathv[0])==-1)
                    {
                        printf("Change failed\n");
                        lastCommandSuccess=false;
                        globfree(&files);
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        printf("Success change to dirctory: %s\n",arguments[firstArgumentIndex+1]);
                        lastCommandSuccess=true;
                        globfree(&files);
                        exit(EXIT_FAILURE);
                    }
                }
                else if(strcmp(arguments[firstArgumentIndex],"pwd")==0)
                {
                    char dictoryName[100];
                    if(getcwd(dictoryName,100)!=NULL)
                    {
                        printf("%s\n",dictoryName);
                        lastCommandSuccess=true;
                        exit(EXIT_SUCCESS);
                    }
                    else
                    {
                        printf("Error when finding path\n");
                        lastCommandSuccess=false;
                        exit(EXIT_FAILURE);
                    }
                }
                else if(strcmp(arguments[firstArgumentIndex],"exit")==0)
                {
                    //There may be a bug here: Additional space compare to original command
                    for(int i=firstArgumentIndex+1;i<argumentAmount;i++)
                    {
                        printf("%s ",arguments[i]);
                    }
                    printf("\n");
                    struct timespec waitTime;
                    waitTime.tv_sec=0;
                    waitTime.tv_nsec=10000000;
                    nanosleep(&waitTime,NULL);
                    kill(getppid(),SIGUSR1);
                    free(command);
                    free(arguments[0]);
                    free(arguments);
                    exit(EXIT_SUCCESS);
                }
                /**
                 * Notes for which command:
                 * When not finding any match item, we assume command fail
                */
                else if(strcmp(arguments[firstArgumentIndex],"which")==0)
                {
                    if(argumentAmount<=firstArgumentIndex+1)
                    {
                        printf("Error format\n");
                        lastCommandSuccess=false;
                        exit(EXIT_FAILURE);
                    }
                    if(strcmp(arguments[firstArgumentIndex+1],"which")==0||strcmp(arguments[firstArgumentIndex+1],"cd")==0||strcmp(arguments[firstArgumentIndex+1],"pwd")==0||strcmp(arguments[firstArgumentIndex+1],"exit")==0)
                    {
                        lastCommandSuccess=false;
                        goto skipAll;
                    }
                    glob_t files;
                    char pathToFileOne[100]={"/usr/local/bin/"};        
                    strcat(pathToFileOne,arguments[firstArgumentIndex+1]);            
                    int res=glob(pathToFileOne,GLOB_NOSORT,NULL,&files);
                    if(res==GLOB_ABORTED)
                    {
                        if(DEBUGMODE) printf("glob resulting in error\n");//Only for debug use
                        lastCommandSuccess=false;
                        globfree(&files);
                        exit(EXIT_FAILURE);
                    }      
                    if(res==GLOB_NOMATCH)
                    {
                        goto secondPart1Which;
                    }
                    for(int i=0;i<files.gl_pathc;i++)
                    {
                        printf("%s\n",files.gl_pathv[i]);
                        if(i==files.gl_pathc-1)
                        {
                            
                            lastCommandSuccess=true;
                            globfree(&files);
                            exit(EXIT_SUCCESS);
                        }
                    }
                    secondPart1Which:
                    char pathToFileTwo[100]={"/usr/bin/"};        
                    strcat(pathToFileTwo,arguments[firstArgumentIndex+1]);            
                    res=glob(pathToFileTwo,GLOB_NOSORT,NULL,&files);
                    if(res==GLOB_ABORTED)
                    {
                        if(DEBUGMODE) printf("glob resulting in error\n");//Only for debug use
                        lastCommandSuccess=false;
                        globfree(&files);
                        exit(EXIT_FAILURE);
                    }      
                    if(res==GLOB_NOMATCH)
                    {
                        goto ThirdPart1Which;
                    }
                    for(int i=0;i<files.gl_pathc;i++)
                    {
                        printf("%s\n",files.gl_pathv[i]);
                        if(i==files.gl_pathc-1)
                        {
                            
                            lastCommandSuccess=true;
                            globfree(&files);
                            exit(EXIT_SUCCESS);
                        }
                    } 

                    ThirdPart1Which:
                    char pathToFileThree[100]={"/bin/"};        
                    strcat(pathToFileThree,arguments[firstArgumentIndex+1]);            
                    res=glob(pathToFileThree,GLOB_NOSORT,NULL,&files);
                    if(res==GLOB_ABORTED)
                    {
                        if(DEBUGMODE) printf("glob resulting in error\n");//Only for debug use
                        lastCommandSuccess=false;
                        globfree(&files);
                        exit(EXIT_FAILURE);
                    }      
                    if(res==GLOB_NOMATCH)
                    {
                        globfree(&files);
                        exit(EXIT_SUCCESS);
                    }
                    for(int i=0;i<files.gl_pathc;i++)
                    {
                        printf("%s\n",files.gl_pathv[i]);
                        if(i==files.gl_pathc-1)
                        {
                            lastCommandSuccess=true;
                            globfree(&files);
                            exit(EXIT_SUCCESS);
                        }
                    }   
                    skipAll:
                    globfree(&files);

                }                
                //buffer not required, data only read once, then abort
                //run program command
                else//indicates this command will run an excutable file
                {
                    char* exeFilePath;
                    glob_t exeFiles;
                    if(strchr(arguments[firstArgumentIndex],'/')!=NULL)
                    {
                        glob(arguments[firstArgumentIndex],GLOB_NOCHECK,NULL,&exeFiles);
                        exeFilePath=exeFiles.gl_pathv[0];
                    }
                    else
                    {
                        char pathToFileOne[100]={"/usr/local/bin/"};        
                        strcat(pathToFileOne,arguments[firstArgumentIndex]);  
                        int res=glob(pathToFileOne,GLOB_NOSORT,NULL,&exeFiles);
                        if(res!=GLOB_NOMATCH)
                        {
                            exeFilePath=exeFiles.gl_pathv[0];
                            goto afterFindExeFilePath1;
                        }
                        else
                        {
                            globfree(&exeFiles);
                            char pathToFileTwo[100]={"/usr/bin/"};        
                            strcat(pathToFileTwo,arguments[firstArgumentIndex]);  
                            int res2=glob(pathToFileTwo,GLOB_NOSORT,NULL,&exeFiles);
                            if(res2!=GLOB_NOMATCH)
                            {
                                exeFilePath=exeFiles.gl_pathv[0];
                                goto afterFindExeFilePath1;
                            }
                            else
                            {
                                globfree(&exeFiles);
                                char pathToFileThree[100]={"/bin/"};        
                                strcat(pathToFileThree,arguments[firstArgumentIndex]);  
                                int res3=glob(pathToFileThree,GLOB_NOSORT,NULL,&exeFiles);
                                if(res3!=GLOB_NOMATCH)
                                {
                                    exeFilePath=exeFiles.gl_pathv[0];
                                    goto afterFindExeFilePath1;
                                }
                                else
                                {
                                    printf("Command doesn't exist\n");
                                    lastCommandSuccess=false;
                                    goto freePart;
                                }
                            }
                        }
                    }

                    afterFindExeFilePath1:
                    char* stdinPath=NULL;
                    char* stdoutPath=NULL;
                    char** exeFileArguments=calloc(2,sizeof(char*));//Big enough
                    exeFileArguments[0]=arguments[firstArgumentIndex];
                    int exeFileArgumentIndex=0;
                    for(int i=firstArgumentIndex+1;i<argumentAmount;i++)
                    {
                        if(strcmp(arguments[i],"<")==0)//Std input
                        {
                            stdinPath=arguments[i+1];
                            i=i+1;
                        }
                        if(strcmp(arguments[i],">")==0)//Std output
                        {
                            stdoutPath=arguments[i+1];
                            i=i+1;
                        }
                        else
                        {
                            glob_t files;         
                            int res=glob(arguments[i],GLOB_NOCHECK,NULL,&files);
                            if(res==GLOB_ABORTED)
                            {
                                if(DEBUGMODE) printf("glob resulting in error\n");//Only for debug use
                                lastCommandSuccess=false;
                            }        
                            for(int i=0;i<files.gl_pathc;i++)
                            {
                                exeFileArgumentIndex++;
                                exeFileArguments=realloc(exeFileArguments,(exeFileArgumentIndex+2)*sizeof(char*));
                                exeFileArguments[exeFileArgumentIndex]=files.gl_pathv[i];
                            }

                        }
                    }
                    exeFileArguments[++exeFileArgumentIndex]=NULL;

                    if(stdinPath==NULL)
                    {
                        dup2(inputPipe[0],STDIN_FILENO);
                        close(inputPipe[0]);
                    }
                    if(stdinPath!=NULL)
                    {
                        int fdInput=open(stdinPath,O_RDONLY);
                        if(fdInput!=-1)
                        {
                            dup2(fdInput,STDIN_FILENO);
                            close(fdInput);
                        }
                        else
                        {
                            printf("Invalid input path\n");
                            lastCommandSuccess=false;
                            exit(EXIT_FAILURE);
                        }
                        
                    }
                    if(stdoutPath!=NULL)
                    {
                        int fdOutput=open(stdoutPath,O_TRUNC|O_CREAT|O_WRONLY,0640);
                        if(fdOutput!=-1)
                        {
                            dup2(fdOutput,STDOUT_FILENO);
                            close(fdOutput);
                        }
                        else
                        {
                            printf("Error when spcify output\n");
                            lastCommandSuccess=false;
                            exit(EXIT_FAILURE);
                        }
                    }

                    execv(exeFilePath,exeFileArguments);

                    //Upon success, the following code will not be execute
                    if(DEBUGMODE) printf("Error in running exe file\n");
                    lastCommandSuccess=false;
                    exit(EXIT_FAILURE);

                }         
            }
            else//Parent process, the first part of pipeline process
            {
                
                close(inputPipe[0]);//Close read port
                argumentAmount=pipeIndex;
                /**
                 * Merge two condition together: has then or else at beginning or not
                 * Using firstArgumentIndex to identify index
                 * Argument example: then cd <dir> 
                 * Index:            0    1  2 
                 * In this case, firstArgumentIndex=1
                 * 
                */
                if(strcmp(arguments[firstArgumentIndex],"cd")==0)
                {
                    glob_t files;
                    int res=glob(arguments[firstArgumentIndex+1],GLOB_NOCHECK,NULL,&files);
                    if(res==GLOB_ABORTED)
                    {
                        if(DEBUGMODE) printf("glob resulting in error\n");//Only for debug use
                        lastCommandSuccess=false;
                    }
                    if(chdir(files.gl_pathv[0])==-1)
                    {
                        printf("Change failed\n");
                        lastCommandSuccess=false;
                    }
                    else
                    {
                        printf("Success change to dirctory: %s\n",arguments[firstArgumentIndex+1]);
                        lastCommandSuccess=true;
                    }
                }
                else if(strcmp(arguments[firstArgumentIndex],"pwd")==0)
                {
                    char dictoryName[100];
                    if(getcwd(dictoryName,100)!=NULL)
                    {
                        write(inputPipe[1],dictoryName,strlen(dictoryName));
                        lastCommandSuccess=true;
                    }
                    else
                    {
                        printf("Error when finding path\n");
                        lastCommandSuccess=false;
                    }
                }
                else if(strcmp(arguments[firstArgumentIndex],"exit")==0)
                {
                    //There may be a bug here: Additional space compare to original command
                    for(int i=firstArgumentIndex+1;i<argumentAmount;i++)
                    {
                        write(inputPipe[1],arguments[i],strlen(arguments[i]));
                        write(inputPipe[1]," ",1);
                    }

                    //printf("The shell exit success\n");
                    //return 0;
                }
                /**
                 * Notes for which command:
                 * When not finding any match item, we assume command fail
                */
                else if(strcmp(arguments[firstArgumentIndex],"which")==0)
                {
                    if(argumentAmount<=firstArgumentIndex+1)
                    {
                        printf("Error format\n");
                        lastCommandSuccess=false;
                        goto freePart;
                    }
                    if(strcmp(arguments[firstArgumentIndex+1],"which")==0||strcmp(arguments[firstArgumentIndex+1],"cd")==0||strcmp(arguments[firstArgumentIndex+1],"pwd")==0||strcmp(arguments[firstArgumentIndex+1],"exit")==0)
                    {
                        lastCommandSuccess=false;
                        goto skipAll2;
                    }
                    
                    glob_t files;
                    char pathToFileOne[100]={"/usr/local/bin/"};        
                    strcat(pathToFileOne,arguments[firstArgumentIndex+1]);            
                    int res=glob(pathToFileOne,GLOB_NOSORT,NULL,&files);
                    if(res==GLOB_ABORTED)
                    {
                        if(DEBUGMODE) printf("glob resulting in error\n");//Only for debug use
                        lastCommandSuccess=false;
                    }      
                    if(res==GLOB_NOMATCH)
                    {
                        goto secondWhich;
                    }
                    for(int i=0;i<files.gl_pathc;i++)
                    {
                        write(inputPipe[1],files.gl_pathv[i],strlen(files.gl_pathv[i]));
                        write(inputPipe[1],"\n",1);
                        if(i==files.gl_pathc-1)
                        {
                            lastCommandSuccess=true;
                            continue;
                        }
                    }
                    secondWhich:
                    globfree(&files);
                    char pathToFileTwo[100]={"/usr/bin/"};        
                    strcat(pathToFileTwo,arguments[firstArgumentIndex+1]);            
                    res=glob(pathToFileTwo,GLOB_NOSORT,NULL,&files);
                    if(res==GLOB_ABORTED)
                    {
                        if(DEBUGMODE) printf("glob resulting in error\n");//Only for debug use
                        lastCommandSuccess=false;
                    }      
                    if(res==GLOB_NOMATCH)
                    {
                        goto ThirdWhich;
                    }                    
                    for(int i=0;i<files.gl_pathc;i++)
                    {
                        write(inputPipe[1],files.gl_pathv[i],strlen(files.gl_pathv[i]));
                        write(inputPipe[1],"\n",1);
                        if(i==files.gl_pathc-1)
                        {
                            lastCommandSuccess=true;
                        }
                    } 

                    ThirdWhich:
                    globfree(&files);
                    char pathToFileThree[100]={"/bin/"};        
                    strcat(pathToFileThree,arguments[firstArgumentIndex+1]);            
                    res=glob(pathToFileThree,GLOB_NOSORT,NULL,&files);
                    if(res==GLOB_ABORTED)
                    {
                        if(DEBUGMODE) printf("glob resulting in error\n");//Only for debug use
                        lastCommandSuccess=false;
                    }      
                    if(res==GLOB_NOMATCH)
                    {
                        goto skipThem;
                    }
                    for(int i=0;i<files.gl_pathc;i++)
                    {
                        write(inputPipe[1],files.gl_pathv[i],strlen(files.gl_pathv[i]));
                        write(inputPipe[1],"\n",1);
                        if(i==files.gl_pathc-1)
                        {

                            lastCommandSuccess=true;
                        }
                    }   
                    skipThem:
                    globfree(&files);
                    skipAll2:

                }
                else//indicates this command will run an excutable file
                {
                    glob_t files; 
                    char* exeFilePath;
                    glob_t exeFiles;
                    if(strchr(arguments[firstArgumentIndex],'/')!=NULL)
                    {
                        glob(arguments[firstArgumentIndex],GLOB_NOCHECK,NULL,&exeFiles);
                        exeFilePath=exeFiles.gl_pathv[0];
                    }
                    else
                    {
                        char pathToFileOne[100]={"/usr/local/bin/"};        
                        strcat(pathToFileOne,arguments[firstArgumentIndex]);  
                        int res=glob(pathToFileOne,GLOB_NOSORT,NULL,&exeFiles);
                        if(res!=GLOB_NOMATCH)
                        {
                            exeFilePath=exeFiles.gl_pathv[0];
                            goto afterFindExeFilePath2;
                        }
                        else
                        {
                            globfree(&exeFiles);
                            char pathToFileTwo[100]={"/usr/bin/"};        
                            strcat(pathToFileTwo,arguments[firstArgumentIndex]);  
                            int res2=glob(pathToFileTwo,GLOB_NOSORT,NULL,&exeFiles);
                            if(res2!=GLOB_NOMATCH)
                            {
                                exeFilePath=exeFiles.gl_pathv[0];
                                goto afterFindExeFilePath2;
                            }
                            else
                            {
                                globfree(&exeFiles);
                                char pathToFileThree[100]={"/bin/"};        
                                strcat(pathToFileThree,arguments[firstArgumentIndex]);  
                                int res3=glob(pathToFileThree,GLOB_NOSORT,NULL,&exeFiles);
                                if(res3!=GLOB_NOMATCH)
                                {
                                    exeFilePath=exeFiles.gl_pathv[0];
                                    goto afterFindExeFilePath2;
                                }
                                else
                                {
                                    printf("Command doesn't exist\n");
                                    lastCommandSuccess=false;
                                    goto freePart;
                                }
                            }
                        }
                    }

                    afterFindExeFilePath2:
                    char* stdinPath=NULL;
                    char* stdoutPath=NULL;
                    char** exeFileArguments=calloc(2,sizeof(char*));//Big enough
                    exeFileArguments[0]=arguments[firstArgumentIndex];
                    int exeFileArgumentIndex=0;
                    
                    for(int i=firstArgumentIndex+1;i<argumentAmount;i++)
                    {
                        if(strcmp(arguments[i],"<")==0)//Std input
                        {
                            stdinPath=arguments[i+1];
                            i=i+1;
                        }
                        if(strcmp(arguments[i],">")==0)//Std output
                        {
                            stdoutPath=arguments[i+1];
                            i=i+1;
                        }
                        else
                        {
                                    
                            glob(arguments[i],GLOB_NOCHECK,NULL,&files);
                            for(int i=0;i<files.gl_pathc;i++)
                            {
                                exeFileArgumentIndex++;
                                exeFileArguments=realloc(exeFileArguments,(exeFileArgumentIndex+2)*sizeof(char*));
                                exeFileArguments[exeFileArgumentIndex]=calloc(strlen(files.gl_pathv[i]),sizeof(char));
                                memcpy(exeFileArguments[exeFileArgumentIndex],files.gl_pathv[i],strlen(files.gl_pathv[i]));

                            }
                            globfree(&files);
                        }
                    }
                    exeFileArguments[++exeFileArgumentIndex]=NULL;
                    

                    int sendDataPipe[2];
                    if(pipe(sendDataPipe)==-1)
                    {
                        if(DEBUGMODE) printf("error when creating pipe\n");
                        lastCommandSuccess=false;
                        goto freePart;
                    }
                    //Create child process
                    pid_t pid_sub=fork();
                    if(pid_sub==-1)
                    {
                        if(DEBUGMODE) printf("Error\n");
                        lastCommandSuccess=false;
                        exit(EXIT_FAILURE);
                    }
                    /**
                     * Now in child process, in child process:
                     * 1. run the program
                     * 2. using pipe send data to parent
                    */
                    else if(pid_sub==0)
                    {
                        if(stdinPath!=NULL)
                        {
                            int fdInput=open(stdinPath,O_RDONLY);
                            if(fdInput!=-1)
                            {
                                dup2(fdInput,STDIN_FILENO);
                                close(fdInput);
                            }
                            else
                            {
                                printf("Invalid input path\n");
                                lastCommandSuccess=false;
                                exit(EXIT_FAILURE);
                            }
                        
                        }
                        close(sendDataPipe[0]);
                        if(DEBUGMODE) printf("exe file path is %s\n",exeFilePath);
                        if(DEBUGMODE) printf("arguments is %s\n",exeFileArguments[0]);
                        dup2(sendDataPipe[1],STDOUT_FILENO);
                        execv(exeFilePath,exeFileArguments);

                        //Upon success, the following code will not be execute
                        if(DEBUGMODE) printf("Error in running exe file\n");
                        lastCommandSuccess=false;
                        exit(EXIT_FAILURE);
                    }
                    /**
                     * Now in parent process, this process will do:
                     * 1. Receive data from child process and store them in buffer array
                     * 2. Put data to file if specfied
                     * 3. Send data to inputPipe write port
                    */
                    else
                    {
                        globfree(&exeFiles);
                        for(int i=1;i<exeFileArgumentIndex;i++)
                        {
                            free(exeFileArguments[i]);
                        }
                        free(exeFileArguments);

                        close(sendDataPipe[1]);
                        int exitStatus;
                        waitpid(pid_sub,&exitStatus,0);
                        if(exitStatus==0)
                        {
                            lastCommandSuccess=true;
                            char buffer[10000];//Size 10000 should be enough
                            int actualReadData=read(sendDataPipe[0],buffer,10000);
                            /**
                             * 这里使用signal函数处理了sigpipe信号原因：
                             * 无法确保子进程先执行，如果父进程先执行则程序无法得知是否有打开的端口
                             * 当尝试向一个没有读取地方的管道写入数据时
                             * 会弹sigpipe信号导致程序异常退出
                             * 使用sig_ign可以忽略该信号，让程序继续执行
                             * 因为有子程序读取，所以不怕管道不协调的问题
                            */
                            signal(SIGPIPE,SIG_IGN);
                            //Write data to pipe
                            write(inputPipe[1],buffer,actualReadData);
                            //Write data to file if specfied
                            if(stdoutPath!=NULL)
                            {
                                int fdOutput=open(stdoutPath,O_TRUNC|O_CREAT|O_WRONLY,0640);
                                if(fdOutput!=-1)
                                {
                                    write(fdOutput,buffer,actualReadData);
                                    close(fdOutput);
                                }
                                else
                                {
                                    if(DEBUGMODE) printf("Error when spcify output\n");
                                    lastCommandSuccess=false;
                                    exit(EXIT_FAILURE);
                                }
                            }
                            if(DEBUGMODE) printf("The program execute and exit success.\n");
                        }
                        else
                        {
                            lastCommandSuccess=false;
                            if(DEBUGMODE) printf("The program itself has an error.\n");
                        }
                        
                    }

                    // for(int i=0;i<exeFileArgumentIndex-2;i++)
                    // {
                    //     free(exeFileArguments[i]);
                    // }

                }
                int exitStatusTwo;

                signal(SIGUSR1,stopProgram);
                waitpid(secondPartThread,&exitStatusTwo,0);
                //free(arguments);


                


            }
            




        }
        else//No need to connect two different program
        {
            int firstArgumentIndex=0;
            /**
             * Merge two condition together: has then or else at beginning or not
             * Using firstArgumentIndex to identify index
             * Argument example: then cd <dir> 
             * Index:            0    1  2 
             * In this case, firstArgumentIndex=1
             * 
            */
            if(strcmp(arguments[0],"then")==0||strcmp(arguments[0],"else")==0)
            {
                firstArgumentIndex=1;
            }
            //Built-in command: cd, pwd, exit, which
            else if(strcmp(arguments[firstArgumentIndex],"cd")==0)
            {
                glob_t files;
                int res=glob(arguments[firstArgumentIndex+1],GLOB_NOCHECK,NULL,&files);
                if(res==GLOB_ABORTED)
                {
                    if(DEBUGMODE) printf("glob resulting in error\n");//Only for debug use
                    lastCommandSuccess=false;
                }
                if(chdir(files.gl_pathv[0])==-1)
                {
                    if(DEBUGMODE) printf("Change failed\n");
                    lastCommandSuccess=false;
                }
                else
                {
                    if(DEBUGMODE) printf("Success change to dirctory: %s\n",arguments[firstArgumentIndex+1]);
                    lastCommandSuccess=true;
                }
                globfree(&files);
            }
            else if(strcmp(arguments[firstArgumentIndex],"pwd")==0)
            {
                char dictoryName[100];
                if(getcwd(dictoryName,100)!=NULL)
                {
                    printf("%s\n",dictoryName);
                    lastCommandSuccess=true;
                }
                else
                {
                    printf("Error when finding path\n");
                    lastCommandSuccess=false;
                }
            }
            else if(strcmp(arguments[firstArgumentIndex],"exit")==0)
            {
                //There may be a bug here: Additional space compare to original command
                for(int i=firstArgumentIndex+1;i<argumentAmount;i++)
                {
                    printf("%s ",arguments[i]);
                }
                if(argumentAmount>firstArgumentIndex+1)
                {
                    printf("\n");
                }
                printf("The shell exit success\n");
                free(command);
                free(arguments[0]);
                for(int i=1;i<originalArgumentAmmount;i++)
                {
                    free(arguments[i]);
                }
                free(arguments);
                return 0;
            }
            /**
             * Notes for which command:
             * When not finding any match item, we assume command fail
            */
            else if(strcmp(arguments[firstArgumentIndex],"which")==0)
            {
                if(argumentAmount<=firstArgumentIndex+1)
                {
                    printf("Error format\n");
                    lastCommandSuccess=false;
                    goto freePart;
                }
                if(strcmp(arguments[firstArgumentIndex+1],"which")==0||strcmp(arguments[firstArgumentIndex+1],"cd")==0||strcmp(arguments[firstArgumentIndex+1],"pwd")==0||strcmp(arguments[firstArgumentIndex+1],"exit")==0)
                {
                    lastCommandSuccess=false;
                    goto skipAll3;
                }
                glob_t files;
                char pathToFileOne[100]={"/usr/local/bin/"};        
                strcat(pathToFileOne,arguments[firstArgumentIndex+1]);  
                if(DEBUGMODE) printf("Path is %s\n",pathToFileOne);          
                int res=glob(pathToFileOne,GLOB_NOCHECK,NULL,&files);
                if(res==GLOB_ABORTED)
                {
                    if(DEBUGMODE) printf("glob resulting in error\n");//Only for debug use
                    lastCommandSuccess=false;
                }      
                if(res==GLOB_NOMATCH)
                {
                    goto secondPartWhich;
                }
                for(int i=0;i<files.gl_pathc;i++)
                {
                    printf("%s\n",files.gl_pathv[i]);
                    if(i==files.gl_pathc-1)
                    {
                        lastCommandSuccess=true;
                        continue;
                    }
                }
                secondPartWhich:
                globfree(&files);
                char pathToFileTwo[100]={"/usr/bin/"};        
                strcat(pathToFileTwo,arguments[firstArgumentIndex+1]);            
                res=glob(pathToFileTwo,GLOB_NOCHECK,NULL,&files);
                if(res==GLOB_ABORTED)
                {
                    if(DEBUGMODE) printf("glob resulting in error\n");//Only for debug use
                    lastCommandSuccess=false;
                }      
                if(res==GLOB_NOMATCH)
                {
                    goto ThirdPartWhich;
                }                    
                for(int i=0;i<files.gl_pathc;i++)
                {
                    printf("%s\n",files.gl_pathv[i]);
                    if(i==files.gl_pathc-1)
                    {
                        lastCommandSuccess=true;
                    }
                } 

                ThirdPartWhich:
                globfree(&files);
                char pathToFileThree[100]={"/bin/"};        
                strcat(pathToFileThree,arguments[firstArgumentIndex+1]);            
                res=glob(pathToFileThree,GLOB_NOCHECK,NULL,&files);
                if(res==GLOB_ABORTED)
                {
                    if(DEBUGMODE) printf("glob resulting in error\n");//Only for debug use
                    lastCommandSuccess=false;
                }      
                if(res==GLOB_NOMATCH)
                {
                    goto freePart;
                }
                for(int i=0;i<files.gl_pathc;i++)
                {
                    printf("%s\n",files.gl_pathv[i]);
                    if(i==files.gl_pathc-1)
                    {
                        lastCommandSuccess=true;
                    }
                } 
                globfree(&files);
                skipAll3:
                
                  

            }

            //run program command
            //if(strchr(arguments[firstArgumentIndex],'/')!=NULL)
            else//indicates this command will run an excutable file
            {
                char* exeFilePath;
                glob_t exeFiles;
                if(strchr(arguments[firstArgumentIndex],'/')!=NULL)
                {
                    glob(arguments[firstArgumentIndex],GLOB_NOCHECK,NULL,&exeFiles);
                    exeFilePath=exeFiles.gl_pathv[0];
                }
                else
                {
                    char pathToFileOne[100]={"/usr/local/bin/"};        
                    strcat(pathToFileOne,arguments[firstArgumentIndex]);  
                    int res=glob(pathToFileOne,GLOB_NOSORT,NULL,&exeFiles);
                    if(res!=GLOB_NOMATCH)
                    {
                        exeFilePath=exeFiles.gl_pathv[0];
                        goto afterFindExeFilePath;
                    }
                    else
                    {
                        globfree(&exeFiles);
                        char pathToFileTwo[100]={"/usr/bin/"};        
                        strcat(pathToFileTwo,arguments[firstArgumentIndex]);  
                        int res2=glob(pathToFileTwo,GLOB_NOSORT,NULL,&exeFiles);
                        if(res2!=GLOB_NOMATCH)
                        {
                            exeFilePath=exeFiles.gl_pathv[0];
                            goto afterFindExeFilePath;
                        }
                        else
                        {
                            globfree(&exeFiles);
                            char pathToFileThree[100]={"/bin/"};        
                            strcat(pathToFileThree,arguments[firstArgumentIndex]);  
                            int res3=glob(pathToFileThree,GLOB_NOSORT,NULL,&exeFiles);
                            if(res3!=GLOB_NOMATCH)
                            {
                                exeFilePath=exeFiles.gl_pathv[0];
                                goto afterFindExeFilePath;
                            }
                            else
                            {
                                printf("Command doesn't exist\n");
                                lastCommandSuccess=false;
                                goto freePart;
                            }
                        }
                    }
                }

                afterFindExeFilePath:
                char* stdinPath=NULL;
                char* stdoutPath=NULL;
                char** exeFileArguments=calloc(2,sizeof(char*));//Big enough
                exeFileArguments[0]=arguments[firstArgumentIndex];
                int exeFileArgumentIndex=0;
                for(int i=firstArgumentIndex+1;i<argumentAmount;i++)
                {
                    if(strcmp(arguments[i],"<")==0)//Std input
                    {
                        stdinPath=arguments[i+1];
                        i=i+1;
                    }
                    if(strcmp(arguments[i],">")==0)//Std output
                    {
                        stdoutPath=arguments[i+1];
                        i=i+1;
                    }
                    else
                    {
                        glob_t files;         
                        int res=glob(arguments[i],GLOB_NOCHECK,NULL,&files);
                        if(res==GLOB_ABORTED)
                        {
                            if(DEBUGMODE) printf("glob resulting in error\n");//Only for debug use
                            lastCommandSuccess=false;
                        }        
                        for(int i=0;i<files.gl_pathc;i++)
                        {
                            exeFileArgumentIndex++;
                            exeFileArguments=realloc(exeFileArguments,(exeFileArgumentIndex+2)*sizeof(char*));
                            exeFileArguments[exeFileArgumentIndex]=calloc(strlen(files.gl_pathv[i]),sizeof(char));
                            memcpy(exeFileArguments[exeFileArgumentIndex],files.gl_pathv[i],strlen(files.gl_pathv[i]));
                        }
                        globfree(&files);

                    }
                }
                exeFileArguments[++exeFileArgumentIndex]=NULL;
                

                //Create child process
                pid_t pid=fork();
                if(pid==-1)
                {
                    if(DEBUGMODE) printf("Error\n");
                    lastCommandSuccess=false;
                    exit(EXIT_FAILURE);
                }
                //Now in child process, since there is no pipeline, no need to create a pipe and communicate with parent process
                else if(pid==0)
                {
                    if(stdinPath!=NULL)
                    {
                        int fdInput=open(stdinPath,O_RDONLY);
                        if(fdInput!=-1)
                        {
                            dup2(fdInput,STDIN_FILENO);
                            close(fdInput);
                        }
                        else
                        {
                            printf("Invalid input path\n");
                            lastCommandSuccess=false;
                            exit(EXIT_FAILURE);
                        }
                        
                    }
                    if(stdoutPath!=NULL)
                    {
                        int fdOutput=open(stdoutPath,O_TRUNC|O_CREAT|O_WRONLY,0640);
                        if(fdOutput!=-1)
                        {
                            dup2(fdOutput,STDOUT_FILENO);
                            close(fdOutput);
                        }
                        else
                        {
                            printf("Error when spcify output\n");
                            lastCommandSuccess=false;
                            exit(EXIT_FAILURE);
                        }
                    }
                    execv(exeFilePath,exeFileArguments);

                    //Upon success, the following code will not be execute
                    if(DEBUGMODE) printf("Error in running exe file\n");
                    lastCommandSuccess=false;
                    exit(EXIT_FAILURE);
                }
                else//Now in parent process
                {
                    for(int i=1;i<exeFileArgumentIndex;i++)
                    {
                        free(exeFileArguments[i]);
                    }
                    int exitStatus;
                    wait(&exitStatus);
                    lastCommandSuccess=true;

                    

                }
                globfree(&exeFiles);
                free(exeFileArguments[exeFileArgumentIndex]);
                free(exeFileArguments);

            }
        }
        // for(int i=0;i<originAmount;i++)
        // {
        //     free(arguments[i]);
        // }
        freePart:
        free(command);
        free(arguments[0]);
        for(int i=1;i<originalArgumentAmmount;i++)
        {
            free(arguments[i]);
        }
        free(arguments);
    }
    return 0;
}