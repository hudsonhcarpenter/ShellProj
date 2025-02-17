#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

void myError()
{
  char error_message[30] = "An error has occurred\n";
  write(STDOUT_FILENO, error_message, strlen(error_message));
}

//parse arguments from a command into comm_argv and return comm_argc
int parse_command(char *input, char **comm_argv) {
    char *token;
    int comm_argc = 0;
    char *spaces = " \t\n";

    //tokenize the input string using space as a delimiter
    char *input_copy = strdup(input);
    token = strtok(input_copy, spaces);
    while (token != NULL) {
        comm_argv[comm_argc++] = token;
        token = strtok(NULL, spaces);
    }

    // Null-terminate the argv array
    comm_argv[comm_argc] = NULL;

    return comm_argc;
}

//prints the remaining input of a command that is too long
void print_excess_input(FILE *input){
  int ch;
  char buffer[2];
  buffer[1] = '\0';

  while((ch = fgetc(input)) != '\n'){
    buffer[0] = (char)ch;
    myPrint(buffer);
  }
  myPrint("\n");
}

//load the arguments and file for a redirect, return 0 if there is an error
int parse_redirect(char *command, char **comm_argv, int *comm_argc, char **rdr_file){
  char *left = strtok(command, ">");
  char *right = strtok(NULL, "\n");
  if(!left || !right)
    return 0;

  //get args for program
  *comm_argc = parse_command(left, comm_argv);

  //get redirect output file
  *rdr_file = strdup(right);
  return 1;
}

void execute_command(char *command, char **comm_argv, int comm_argc){
  if(comm_argc < 1){
    return;
  }

  //setup redirection arguments and check syntax
  int rdr = 0;
  char *rdr_filename;
  int advrdr_fd;
  char *existing_contents = NULL;
  off_t file_size;
  if(strstr(command, ">")){
    rdr = 1;
    //check if only whitespace
    char *cpy = strdup(command);
    while(1){
      if(!strchr(" \t\n>", *cpy))
        break;
      else if(*cpy == '\0'){
        myError();
        return;
      }
      cpy++;
    }
    //load redirect arguments
    if(parse_redirect(command, comm_argv, &comm_argc, &rdr_filename) == 0){
      myError();
      return;
    }
    //check if advanced redirect
    if(rdr_filename[0] == '+'){
      rdr_filename++;
      rdr = 2;
    }
    //parse through filename syntax
    char *copy = strdup(rdr_filename);
    char *token = strtok(copy, " \t\n");
    int count = 0;

    while (token != NULL) {
      count++;
      if (count == 1) {
        rdr_filename = token;
      //check not more than one filename
      } else {
        myError();
        return;
      }
      token = strtok(NULL, " \t\n");
    }
    //check there is a filename
    if (count == 0) {
      myError();
      return;
    }
    //check too many ">"
    if(strstr(rdr_filename, ">")){
      myError();
      return;
    }
    //set up advanced redirect
    if(rdr == 2){
      advrdr_fd = open(rdr_filename, O_RDONLY, 0644);
      //file exists = advanced redirect
      if (advrdr_fd >= 0){
        //read existing contents
        file_size = lseek(advrdr_fd, 0, SEEK_END);
        if (file_size > 0) {
          lseek(advrdr_fd, 0, SEEK_SET);
          existing_contents = malloc(file_size + 1);
          read(advrdr_fd, existing_contents, file_size);
          //null-terminate
          existing_contents[file_size] = '\0';
        }
        //clear file contents
        close(advrdr_fd);
        advrdr_fd = open(rdr_filename, O_WRONLY);
        if (ftruncate(advrdr_fd, 0) == -1) {
          close(advrdr_fd);
          free(existing_contents);
          myError();
          exit(0);
        }
      //file doesn't exist - normal redirect
      } else {
        rdr = 1;
      }
      close(advrdr_fd);
    }
  }
  
  char *executable = comm_argv[0];
  char *built_in = "exit pwd cd";

  //built in commands
  if(strstr(built_in, executable) && rdr){
    myError();
    return;
  } else if(strstr(built_in, executable)){
    if(!strcmp(executable, "exit") && comm_argc == 1)
      exit(0);
    else if(!strcmp(executable, "pwd") && comm_argc == 1){
      char cwd[1024];
      getcwd(cwd, sizeof(cwd));
      myPrint(strcat(cwd,"\n"));
    } else if(!strcmp(executable, "cd") && comm_argc < 3){
      if(comm_argc == 2 && chdir(comm_argv[1]) == -1)
        myError();
      else if(comm_argc == 1 && chdir(getenv("HOME")) == -1)
        myError();
    } else {
      myError();
      return;
    }

  //non built-in commands
  } else {
    int forkret = fork();
    if(forkret == 0){
      //ADVANCED REDIRECT
      if(rdr == 2){
        printf("advanced redirecting\n");
        advrdr_fd = open(rdr_filename, O_CREAT | O_WRONLY, 0644);
        //redirect the command output to file
        if (dup2(advrdr_fd, STDOUT_FILENO) < 0) {
          myError();
          close(advrdr_fd);
          exit(0);
        }
        close(advrdr_fd);
      }
      //BASIC REDIRECT
      if(rdr == 1){
        //check file doesn't exist
        int rdr_fd = open(rdr_filename, O_RDONLY);
        if(rdr_fd >= 0){
          close(rdr_fd);
          myError();
          exit(0);
        }
        //open new file in write mode
        rdr_fd = open(rdr_filename, O_CREAT | O_WRONLY, 0644);
        if(rdr_fd < 0) {
          myError();
          exit(0);
        }
        //redirect the command output to file
        if (dup2(rdr_fd, STDOUT_FILENO) < 0) {
          myError();
          close(rdr_fd);
          exit(0);
        }
        close(rdr_fd);
      } 
      //run program
      if(execvp(comm_argv[0], comm_argv) == -1){
        myError();
        exit(0);
      }
    } else {
      wait(NULL);
      //if adcanced redirection, append preexisting contents
      if(rdr == 2){
        advrdr_fd = open(rdr_filename, O_WRONLY | O_APPEND);
        write(advrdr_fd, existing_contents, file_size);
        close(advrdr_fd);
        free(existing_contents);
      }
    }
  }
}

//parses a line into multiple commands, returns the number of commands
int parse_line(char *input, char **comm_arr){
  int comm_cnt = 0;
  char *token;

  token = strtok(input, ";");
  while (token != NULL) {
    if (*token != '\n') 
      comm_arr[comm_cnt++] = token;
    token = strtok(NULL, ";");
  }

  return comm_cnt;
}

int main(int argc, char *argv[]) 
{
    //BATCH MODE - read file
    if(argc > 1){
      FILE *batch_file = fopen(argv[1], "r");
      if(!batch_file){
        myError();
        exit(0);
      }

      char cmdline[514];
      char copy[514];
      while(fgets(cmdline, 514, batch_file)){
        //check if line is too long
        if(strlen(cmdline) > 512 && cmdline[512] != '\n'){
          myPrint(cmdline);
          print_excess_input(batch_file);
          myError();
          continue;
        }

        //print command if not empty line
        strcpy(copy, cmdline);
        if(strtok(copy, " \n\t"))
          write(STDOUT_FILENO, cmdline, strlen(cmdline));

        //parse line (get commands)
        char *comm_arr[512];
        int comm_cnt = parse_line(cmdline, comm_arr);
        
        //parse command arguments and execute each command
        for(int i=0; i<comm_cnt; i++){
          char *comm_argv[512];
          int comm_argc = parse_command(comm_arr[i], comm_argv);

          //execute command
          execute_command(comm_arr[i], comm_argv, comm_argc);
        }
      }

      fclose(batch_file);
      return 0;
    }
    
    //INTERACTIVE MODE - get input
    char cmd_buff[514];
    char *pinput;

    while (1) {
        myPrint("myshell> ");
        pinput = fgets(cmd_buff, 514, stdin);
        if (!pinput) {
            myError();
            exit(0);
        }
  
        //check if input is too long
        if(strlen(pinput) > 512 && pinput[512] != '\n'){
          myPrint(pinput);
          print_excess_input(stdin);
          myError();
          continue;
        }

        //parse line (get commands)
        char *comm_arr[512];
        int comm_cnt = parse_line(pinput, comm_arr);
        
        //parse command arguments and execute each command
        char *command;
        for(int i=0; i<comm_cnt; i++){
          command = comm_arr[i];
          char *comm_argv[512];
          int comm_argc = parse_command(command, comm_argv);

          //execute command
          execute_command(command, comm_argv, comm_argc);
        }
    }
}
