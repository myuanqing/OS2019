#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<regex.h>
#include<fcntl.h>
#define BUFSZ 1024

int main(int argc, char *argv[])
{
        int fildes[2];
        char buf[BUFSZ];
        buf[BUFSZ-1] = '\0';
        pid_t pid;
        if( pipe(fildes)<0 )
        {
                perror("failed to pipe");
                exit(1);
        }
        //int flags = fcntl(fildes[0], F_GETFL);
        //fcntl(fildes[0], F_SETFL, flags | O_NONBLOCK);
        pid = fork();
        if(pid ==  0)
        {
                close(fildes[0]);
                dup2(fildes[1],STDERR_FILENO);
                //close(STDOUT_FILENO);
                //printf("%s\n",argv[1]);
                freopen("/dev/null", "w", stdout);
                char *argv[]={"strace","-T","./p",NULL};
                char *envp[]={0,NULL}; 
                
                execve("/usr/bin/strace", argv, envp);
                exit(0);
        }
        else if(pid > 0)
        {       
                //char *file = "<a href=\"http://www.awaysoft2.com\">Awaysoft2.com</a><a href=\"http://www.awaysoft3.com\">Awaysoft.com3</a>", *st;
                regex_t reg;
                char* pattern = "(\\w*)([(].*[)]\\s*=.*<)(.*)(>)";
                regmatch_t match[5];
                if(regcomp(&reg, pattern, REG_EXTENDED|REG_NEWLINE) != 0){
                        printf("Cannot compile!\n");
                        return -1;
                }
                //st = file;
                while(1){
                /*        if (regexec(&reg, st, 4, match, REG_NOTEOL) != REG_NOMATCH) {
                                printf("%d\n", match[2].rm_so);
                                st = &st[match[3].rm_eo];
                        }
                */        
                        
                        close(fildes[1]);
                        memset(buf,'\0',1024);
                        dup2(fildes[0], 0);
                        if (!fgets(buf, 1023, stdin)) break;
                        puts(buf);
                        //int c1 = read(fildes[0], buf, 1023);
                        //read(fildes[0], buf+c1, 1023);
                        if(regexec(&reg, buf, 5, match, REG_NOTEOL) != REG_NOMATCH){
                                char p1[256];
                                char p2[256];
                                memset(p1,'\0',256);
                                memset(p2,'\0',256);
                                //printf("%d\n",match[1].rm_eo);
                                strncpy(p1,buf+ match[1].rm_so, match[1].rm_eo-match[1].rm_so);
                                strncpy(p2,buf+ match[3].rm_so, match[3].rm_eo-match[3].rm_so);
                                printf("%s: %s\n", p1, p2);
                        }
                        //printf("%s",buf); 
                }
                //readline(fildes[0], buf, 50);
                               
                exit(0);
        }else{
          
        }
        return 0;
}