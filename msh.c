// The MIT License (MIT)
// 
// Copyright (c) 2023 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 128    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 11     // Mav shell updated to support 10 arguments, +1 for null

int main( int argc, char * argv[] )
{

  char * command_string = (char*) malloc( MAX_COMMAND_SIZE );
  int count = 0;
  char history[50][MAX_COMMAND_SIZE];

  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );

    //put input into an array for history function if not whitespace
    if(command_string[0] != '\0')
    {
      if(count < 50)
      {
        strcpy(history[count], command_string);
        count++;
      }
    }

    //if command starts with an exclamation mark convert the number after it, copy the command at that index-1 then use it instead
    if (command_string[0] == '!')
    {
      int n = atoi(command_string + 1);
      strcpy(command_string, history[n-1]);
    }

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      token[i] = NULL;
    }

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr = NULL;                                         
                                                           
    char *working_string  = strdup( command_string );                

    // we are going to move the working_string pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *head_ptr = working_string;

    // Tokenize the input strings with whitespace used as the delimiter
    while ( ( (argument_ptr = strsep(&working_string, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    //If the command is cd then take the next argument as the path and change directory in parent
    if(strcmp(token[0], "cd") == 0)
    {
      const char *path = token[1];
      chdir(path);
    }

    //If the command is history then print the index number + 1 and the string/command stored in history array at that index
    else if(strcmp(token[0], "history") == 0)
    {
      for(int i = 0; i < count; i++)
      {
        printf("[%d] %s\n", i + 1, history[i]);
      }
    }
    
    //Token[0] == NULL causes seg faults. Added this to prevent & ask reprompt quietly
    else if(token[0] != NULL)
    {
      // Cleanup allocated memory if tok0 is either quit or exit
      if (strcmp(token[0], "exit") == 0 || strcmp(token[0], "quit") == 0)
      {
        for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
        {
          if( token[i] != NULL )
          {
            free( token[i] );
          }
        }

        free( head_ptr );
        free( command_string );
        return 0;
        // e1234ca2-76f3-90d6-0703ac120004
      }

      //Start the fork process and call execvp starting with tok0 for cmd tok1 for arg1 etc 
      pid_t pid = fork( );

      if( pid == 0 )
      {
        // Notice you can add as many NULLs on the end as you want
        int ret = execvp( token[0], token );  

        if( ret == -1 )
        {
          //Exit 0 to prevent going another layer down causing a need to exit/quit twice to stop the program correctly.
          printf("%s: Command not found.\n", token[0]);
          exit(0);
        }
      }

      else 
      {
        int status;
        wait( & status );
      }
    }
  }
}
