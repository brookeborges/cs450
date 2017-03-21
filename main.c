/*
Stephanie Lam
Brooke Borges
Hanani Ikeh
 
CS 450 - Homework 2

This is a simple shell-like program in C that reads user commands and executes them using
a combination of "fork", "exec", and "pipe"
 
 */
#include <stdio.h>
#include "constants.h"
#include "parsetools.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

// for commands that use a pipe
int pipeCount = 0;
int pipePos = 0;
int pipeArray[1000];
void syserror(const char *s);
void pipeFunc(int pipePos, char *line_words[], int num_words);

// for commands that use stdin and/or stdout
typedef enum { false, true, nil } bool;
bool inFlag;
bool outFlag;
int inPos = 0;
int outPos = 0;
void stdFunc(char *line_words[], int inPos, int outPos, char *inputFile, char *outputFile);

int main() {
    // Buffer for reading one line of input
    char line[MAX_LINE_CHARS];
    char *line_words[MAX_LINE_WORDS + 1];
    // in case redirection is used
    char *inputFile;
    char *outputFile;
    
    // Loop until user hits Ctrl-D (end of input)
    // or some other input error occurs
    //fork();
    while( fgets(line, MAX_LINE_CHARS, stdin) ) {
        int num_words = split_cmd_line(line, line_words);
        int a = fork();
        // Just for demonstration purposes
        if (a == 0)
        {
            int x = 0;
            for (int i = 0; i < num_words; i++)
            {
                //printf("%s\n", line_words[i]);
           
                // count number of pipes and save pipe position(s)
                if(*line_words[i] == '|')
                {
                    pipeCount += 1;
                    pipeArray[x] = i;
                    x++;
                }
               
                // recognize stdin
                if (*line_words[i] == '<')
                {
                   inFlag = true;
                   inPos = i;
                   inputFile = line_words[i + 1];
                }
                // recognize stdout
                if (*line_words[i] == '>')
                {
                   outFlag = true;
                   outPos = i;
                   outputFile = line_words[i + 1];
                }
            }
            
            
            // if command contains a pipe
            if (pipeCount == 1)
            {
                pipePos = pipeArray[0];
                pipeFunc(pipePos, line_words, num_words);
            }
            // if command contains < or >
            else if (inFlag == true || outFlag == true)
            {
                if (pipeCount == 0)
                    stdFunc(line_words, inPos, outPos, inputFile, outputFile);
            }
            // single command (no pipes or redirection)
            else
                execvp(line_words[0], line_words);
        }
    }

    printf("here - end of main()\n");
    exit(1);
    return 0;
}

void pipeFunc(int pipePos, char* line_words[], int num_words)
{
    // stores command before pipe
    char* lwBefore[pipePos];
    lwBefore[pipePos] = (char*)NULL;
    
    for (int i = 0; i < pipePos; i++)
        lwBefore[i] = line_words[i];
    
    //stores command after pipe
    int afterSize = (num_words - 1) - pipePos; //printf("afterSize: %i \n", afterSize);
    char* lwAfter[afterSize];
    lwAfter[afterSize] = (char*)NULL;
    
    int j = pipePos + 1;
    for (int i = 0; j < num_words; i++)
    {
        lwAfter[i] = line_words[j];
        j++;
    }
    
    // altered code from pipe_demo.c
    int pfd[2];
    pid_t pid;
    
    if ( pipe (pfd) == -1 )
    syserror( "Could not create a pipe" );
    switch ( pid = fork() ) {
        case -1:
        syserror( "First fork failed" );
        
        case  0:
        if ( close( 0 ) == -1 )
            syserror( "Could not close stdin" );
        dup(pfd[0]);
        if ( close (pfd[0]) == -1 || close (pfd[1]) == -1 )
            syserror( "Could not close pfds from first child" );
        execvp(lwAfter[0], lwAfter);
        syserror( "Could not exec wc");
    }
    //fprintf(stderr, "The first child's pid is: %d\n", pid);
    switch ( pid = fork() ) {
        case -1:
        syserror( "Second fork failed" );
        
        case  0:
        if ( close( 1 ) == -1 )
            syserror( "Could not close stdout" );
        dup(pfd[1]);
        if ( close (pfd[0]) == -1 || close (pfd[1]) == -1 )
            syserror( "Could not close pfds from second child" );
        execvp(lwBefore[0], lwBefore);
        syserror( "Could not exec who" );
    }
    //fprintf(stderr, "The second child's pid is: %d\n", pid);
    if (close(pfd[0]) == -1)
        syserror( "Parent could not close stdin" );
    if (close(pfd[1]) == -1)
        syserror( "Parent could not close stdout" );
    while ( wait(NULL) != -1) ;
    
}


void stdFunc(char *line_words[], int inPos, int outPos, char *inputFile, char *outputFile)
{
    // get command before stdin / stdout
    char *IN[MAX_LINE_CHARS];
    char *OUT[MAX_LINE_CHARS];
    
    // if input redirection is used
    if (inFlag == true)
    {
        IN[inPos] = (char*)NULL;
    
        for(int i = 0; i < inPos; i++)
            IN[i] = line_words[i];
    
        int log = open(inputFile, O_RDONLY, 0777);
        dup2(log, 0);
        if (close(log) == -1)
            printf("Could not close log");
    }
    
    // if output redirection is used
    if (outFlag == true)
    {
        OUT[outPos] = (char*)NULL;
        
        for(int i = 0; i < outPos; i++)
            OUT[i] = line_words[i];
        
        int log2 = open(outputFile, O_WRONLY | O_TRUNC, 0777);
        dup2(log2, 1);
        if (close(log2) == -1)
            printf("Could not close log2");
    }
    
    /*
    // if only stdin is used
    if (inFlag == true && outFlag == false)
    {
        execvp(IN[0], IN);
        printf("Exec error");
    } */
    
    // if only stdout is used
    if (outFlag == true && inFlag == false)
    {
        outFlag = nil;
        inFlag = nil;
        execvp(OUT[0], OUT);
        printf("Exec error");
    }
    // both stdin and stdout are used
    else
    {
        outFlag = nil;
        inFlag = nil;
        execvp(IN[0], IN);
        printf("Exec error");
    }
        
}


void syserror(const char *s)
{
    extern int errno;
    
    fprintf( stderr, "%s\n", s );
    fprintf( stderr, " (%s)\n", strerror(errno) );
    exit( 1 );
}

