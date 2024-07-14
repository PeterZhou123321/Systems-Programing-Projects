#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<fcntl.h>
#include<unistd.h>
#include<dirent.h>
#include<string.h>

typedef struct trieNode
{
    struct trieNode* nextAlph;
    unsigned char thisNode;
    bool canBeEnd;
    unsigned char nodeLength;
}trieNode;
void recursiveRead(DIR* d,trieNode* root,char* nowFolder,int* canExitSuccess,bool isSingleFile);
void freeTrie(trieNode* node);
int punctuationWord(char* word);


/**
 * This function insert new node in tireTree
 * return 1 for create new node
 * return 2 for existing node, onlu mark canBeEnd
 * return 3 for fauilre insert
*/
int insert(trieNode* root,char* words,int count) 
{
    char* word;
    word=strndup(words,100);
    
    trieNode* iter=root;
    int length=0;
    for(int i=0;i<strlen(word);i++)
    {
        if(word[i]!=' ')//10 indicates new line
        {
            length++;
        }
    }
    
    for(int i=0;i<length;i++)// control level of traverse
    {
        bool found=false;
        for(int j=0;j<iter->nodeLength;j++) 
        {
            if ((iter->nextAlph)[j].thisNode==word[i])// case when find
            { 
                if (i==length-1) 
                {
                    (iter->nextAlph)[j].canBeEnd=true;// Mark end of word
                    free(word);
                    return 2;
                }
                iter=&((iter->nextAlph)[j]);
                found=true;
                break;
            }
        }
        if(!found)
        {
            iter->nodeLength++;
            trieNode* temp=iter->nextAlph;

            temp=(trieNode*)realloc(iter->nextAlph,sizeof(trieNode)*(iter->nodeLength));
            if(temp!=NULL)
            {
                iter->nextAlph=temp;
            }
            
            //May need to free here, not sure yet
            //iter->nextAlph=newNextAlph;
            iter->nextAlph[iter->nodeLength-1].thisNode=word[i];
            iter->nextAlph[iter->nodeLength-1].canBeEnd=(i==length-1);
            iter->nextAlph[iter->nodeLength-1].nextAlph=NULL;
            iter->nextAlph[iter->nodeLength-1].nodeLength=0;
            if(i==length-1) 
            {
                free(word);
                return 1;
            }
            iter=&iter->nextAlph[iter->nodeLength-1];
        }
    }
    free(word);
    return 3;
}
/**
 * This function will try to find a word matches any existing record in tireTree
 * return -1 if found
 * return any other value to represent incorrect char
*/
int find(trieNode* root,char* word)
{
    
    trieNode* iter=root;
    int length=0;
    for(int i=0;i<1000;i++)
    {
        if(word[i]!=' ')//10 indicates new line
        {
            length++;
        }
        if(word[i]==' ')
        {
            break;
        }
    }
    
    for(int i=0;i<length;i++)// control level of traverse
    {
        for(int j=0;j<iter->nodeLength;j++) 
        {
            if((iter->nextAlph)[j].thisNode==word[i]&&i==length-1&&iter->nextAlph[j].canBeEnd==true)// case when find, and that is the last char of word
            { 
                return -1;
            }
            else if((iter->nextAlph)[j].thisNode==word[i]&&i!=length-1)//case when find but that is not last of char
            {
                iter=&iter->nextAlph[j];
                goto loopOneEnd;
            }
            //other case is, find that char but that char cannot be end when i==length-1
            //this case is also belongs to false, there is no need to write a new condition
        }

        return i;
        loopOneEnd:
    }
    return -1;
}

    
int main(int args,char** argv)
{
    // struct timeval start;
	// struct timeval end;
	//gettimeofday(&start, NULL);
    int counter=0;
    //insert part
    //char* dictOry=argv[1];


    //int fd=open("../../../../../usr/share/dict/words",O_RDONLY);
    int fd=open(argv[1],O_RDONLY);
    char stringBuf=' ';//string buffer that read single char
    char* word=malloc(100*sizeof(char));//array stores words
    for(int i=0;i<100;i++)
    {
        word[i]=' ';
    }
    word[99]='\0';
    trieNode* root=calloc(1,sizeof(trieNode));
    //int nowWordLength=0;
    int debugCounter=0;
    while(true)
    {
        debugCounter++;
        int signal=read(fd,&stringBuf,1);
        if(signal==0)
        {
            break;
        }
        
        if(true)//(stringBuf>=65&&stringBuf<=90)||(stringBuf>=97&&stringBuf<=122)||stringBuf==45||stringBuf=='\n'||stringBuf=='\''
        {
            if(stringBuf=='\n'||stringBuf==' ')
            {
                
                insert(root,word,counter);
                counter++;
                if(word[0]<65||word[0]>90)//Means first letter is not capital
                {
                    char* word_temp=malloc(100);
                    memcpy(word_temp,word,100);
                    word_temp[0]=word_temp[0]-32;
                    insert(root,word_temp,counter);
                    free(word_temp);
                    counter++;
                }
                /**
                 * Only if this is the last condition!!!!
                 * If add more condition, this must use strdup to avoid address change
                */
                for(int i=0;i<100;i++)
                {
                    if(word[i]>=97&&word[i]<=122)
                    {
                        word[i]=word[i]-32;
                        
                    }
                    else if(word[i]==' ')
                    {
                        break;
                    }
                }
                insert(root,word,counter);
                counter++;

                for(int i=0;i<100;i++)
                {
                    word[i]=' ';
                }
                 
            }
            else
            {
                int i=0;
                while(word[i]!=' ')
                {
                    i++;
                }
                word[i]=stringBuf;
            }
        }

    }
    free(word);
    int canExitSuccess=1;

    for(int i=2;i<args;i++)
    {
        DIR* d=opendir(argv[i]);
        bool isSingleFile=false;
        if(d==NULL)//When the given argument is a single file
        {
            isSingleFile=true;
            if(strlen(argv[i])>=4)
            {
                char* fileType=&(argv[i][strlen(argv[i])-4]);
                if(strcmp(fileType,".txt")==0)//indicates it's txt file
                {
                    recursiveRead(d,root,argv[i],&canExitSuccess,isSingleFile);
                }
            }
            else
            {
                printf("Failed to open this file: %s\n",argv[i]);
            }
        }
        else
        {
            recursiveRead(d,root,argv[i],&canExitSuccess,isSingleFile);
        }

        closedir(d);
    }
    
    freeTrie(root);
    free(root);
    if(canExitSuccess==1)
    {
        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
    

    
}
void freeTrie(trieNode* node) 
{
    if(node==NULL) return;
    if(node->nextAlph!=NULL) 
    { 
        for(int i=0;i<node->nodeLength;i++) 
        {
            freeTrie(&(node->nextAlph[i]));
        }
        free(node->nextAlph);
    }
}
void recursiveRead(DIR* d,trieNode* root,char* nowFolder,int* canExitSuccess,bool isSingleFile)
{
    if(isSingleFile)
    {
        goto txtFileProcess;
    }
    struct dirent *dir;
    
    dir=readdir(d);
    
    while(dir!=NULL)
    {
        if(dir->d_type==4)
        {
            goto skipFileLength;
        }
        int fileNameLength=strlen(dir->d_name);
        if(fileNameLength-4<0)
        {
            goto endOfWhile;
        }
        char* fileType=&dir->d_name[fileNameLength-4];
        
        //DT_REG=8, DT_DIR=4
        //Case when we find a dir, ignore all folder name start with .
        skipFileLength:
        if(dir->d_type==4&&dir->d_name[0]!='.')
        {

            char* nextFolderPath;
            nextFolderPath=strdup(nowFolder);

            nextFolderPath=realloc(nextFolderPath,strlen(nextFolderPath)+2);
            int tempLength=strlen(nextFolderPath);
            nextFolderPath[tempLength]='/';
            nextFolderPath[tempLength+1]=0;

            nextFolderPath=realloc(nextFolderPath,strlen(nextFolderPath)+strlen(dir->d_name)+1);
            strcat(nextFolderPath,dir->d_name);
            
            DIR* d=opendir(nextFolderPath);
            
            recursiveRead(d,root,nextFolderPath,canExitSuccess,isSingleFile);
            closedir(d);
            free(nextFolderPath);
        }
        else if(dir->d_type==8&&strcmp(".txt",fileType)==0)//case when txt file find
        {

            char* txtFileName;
            
            txtFileName=strdup(nowFolder);
            
            txtFileName=realloc(txtFileName,strlen(txtFileName)+2);
            int tempLength=strlen(txtFileName);
            txtFileName[tempLength]='/';
            txtFileName[tempLength+1]=0;

            txtFileName=realloc(txtFileName,strlen(txtFileName)+strlen(dir->d_name)+1);//txtFileName,strlen(txtFileName)+strlen(dir->d_name)
            strcat(txtFileName,dir->d_name);
            
            txtFileProcess:
            if(isSingleFile)
            {
                txtFileName=nowFolder;
            }
            int fd=open(txtFileName,O_RDONLY);
            char stringBuf=' ';//string buffer that read single char
            char word[100];//array stores words
            for(int i=0;i<100;i++)
            {
                word[i]=' ';
            }
            int lineNumber=1;
            int readByte=read(fd,&stringBuf,1);
            int hyphenNum=0;
            int columnNumber=0;
            int extraEnter=0;
            while(readByte!=0)
            {
                startExtra:
                columnNumber++;
                int endIgnore;
                
                if(true)
                {
                    if(stringBuf=='-')
                    {
                        hyphenNum++;
                    }
                    if(stringBuf=='\n'||stringBuf==' '||readByte==0)
                    {
                        int charErrorIndex;
                       

                        if(hyphenNum!=0)
                        {
                            int previousLength=0;//store length that previous words take
                            char separateWord[100];
                            for(int i=0;i<100;i++)
                            {
                                separateWord[i]=' ';
                            }
                            int separateWordIndex=0;
                            
                            for(int i=0;i<100;i++)
                            {
                                previousLength++;
                                if(word[i]=='-'||word[i]==' ')
                                {
                                    
                                    endIgnore=punctuationWord(separateWord);
                                   
                                    charErrorIndex=find(root,separateWord);
                                    if(charErrorIndex!=-1)
                                    {
                                        int separateWordLength=0;
                                        for(int j=0;j<100;j++)
                                        {
                                            if(separateWord[j]!=' ')
                                            {
                                                separateWordLength++;
                                            }
                                            else
                                            {
                                                break;
                                            }
                                        }
                                        charErrorIndex=previousLength-separateWordIndex+charErrorIndex;
                                        goto reportError;
                                    }
                                    else
                                    {
                                        for(int k=0;k<100;k++)
                                        {
                                            separateWord[k]=' ';
                                        }
                                    }
                                    
                                    for(int j=0;j<100;j++)
                                    {
                                        separateWord[i]=0;
                                    }
                                    separateWordIndex=0;
                                }
                                else
                                {
                                    separateWord[separateWordIndex]=word[i];
                                    separateWordIndex++;
                                }
                            }
                        }
                        else
                        {
                            
                            endIgnore=punctuationWord(word);
                            charErrorIndex=find(root,word);
                        }
                        

                        

                        
                        reportError:
                        if(charErrorIndex!=-1)//When they are not exactly same
                        {
                            char* errorWord;
                            int wordLength=0;
                            for(int i=0;i<100;i++)
                            {
                                
                                if(word[i]==' ')
                                {
                                    break;
                                }
                                wordLength++;   
                         
                            }
                            errorWord=calloc(wordLength+1,sizeof(char));
                            for(int i=0;i<wordLength;i++)
                            { 
                                errorWord[i]=word[i];
                            }
                            
                            if(isSingleFile)
                            {
                                printf("%s (%d,%d): %s\n",nowFolder,lineNumber,(columnNumber-wordLength-endIgnore)>=1?(columnNumber-wordLength-endIgnore):1,errorWord);
                            }
                            else
                            {
                                printf("%s (%d,%d): %s\n",dir->d_name,lineNumber,(columnNumber-wordLength-endIgnore)>=1?(columnNumber-wordLength-endIgnore):1,errorWord);
                            }
                            
                            free(errorWord);
                        }
                        for(int i=0;i<100;i++)
                        {
                            word[i]=' ';
                        }
                        hyphenNum=0;
                        
                    }
                    else
                    {
                        int i=0;
                        while(word[i]!=' ')
                        {
                            i++;
                        }
                        word[i]=stringBuf;
                    }
                }
                if(stringBuf=='\n')
                {
                    lineNumber++;
                    columnNumber=0;
                }
                readByte=read(fd,&stringBuf,1);
                if(readByte==0&&extraEnter==0)
                {
                    extraEnter++;
                    goto startExtra;
                }

            }
            if(isSingleFile)
            {
                return;
            }
            free(txtFileName);
            
        }
        else//For any other file, report an error, this is not standard file
        {
            if(dir->d_name[0]!='.')
            {
                printf("Failed to open this file: %s\n",dir->d_name);
                *canExitSuccess=0;
            }
        }
        endOfWhile:
        dir=readdir(d);

    }
    return;
}

int punctuationWord(char* word)
{
    int counter=0;
    bool findFirstValid=false;
    int lastCharIndex=0;
    int firstCharIndex=0;
    int afterIgnore=-1;
    while(true)
    {
        if(word[counter]!='{'&&word[counter]!='['&&word[counter]!='('&&word[counter]!='\''&&word[counter]!='\"'&&findFirstValid==false)
        {
            findFirstValid=true;
            firstCharIndex=counter;
        }
        if(findFirstValid==false&&(word[counter]=='{'||word[counter]=='['||word[counter]=='('||word[counter]=='\''||word[counter]=='\"'))
        {
            word[counter]=' ';
            afterIgnore++;
        }
        if((word[counter]>=65&&word[counter]<=90)||(word[counter]>=97&&word[counter]<=122))
        {
            lastCharIndex=counter;
        }
        counter++;
        if(word[counter]==' ')
        {
            break;
        }
    }
    int findLastIgnore=lastCharIndex;
    while(true)
    {
        if(word[findLastIgnore]!=' ')
        {
            afterIgnore++;
        }
        else
        {
            break;
        }
        findLastIgnore++;
    }
    for(int i=lastCharIndex+1;i<100;i++)
    {
        word[i]=' ';
    }
    for(int i=firstCharIndex,j=0;i<=lastCharIndex;i++,j++)
    {
        word[j]=word[i];
        if(i!=j)
        {
            word[i]=' ';
        }
    } 
    if(!((word[0]>=65&&word[0]<=90)||(word[0]>=97&&word[0]<=122))&&word[1]==' ')
    {
        word[0]=' ';
        afterIgnore++;
    }
    return afterIgnore;
}