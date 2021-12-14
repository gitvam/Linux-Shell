/* Vamvakousis Giorgos, csd4112 */
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#define COMMAND_DELIM " \t\r\n\a"
#define PIPE_DELIM ">"
#define BUFSIZE 128
#define MAX_CHARS_USERNAME 64
#define MAX_CHARS_PROMPT 512

void  INThandler(int sig){ /* ctrl c shortcut*/
    signal(sig, SIG_IGN);
    exit(0);
}

char **pipe2tokens(char * str){

  char **pipe_table = (char **) malloc(BUFSIZE * sizeof(char *));
  int i = 0;
  char *token;
  token = strtok(str,PIPE_DELIM);

  while(token!=NULL){
    pipe_table[i] = token;
    i++;
    token = strtok(NULL,PIPE_DELIM);
    if(i>BUFSIZE)break;
  }

  pipe_table[i] = NULL;
  return pipe_table;
}

char **line2tokens(char * str){

  char **token_table = (char **) malloc(BUFSIZE * sizeof(char *));
  int i = 0;
  char *token;
  char *newStr = (char *)malloc(BUFSIZE);
  newStr = strcpy(newStr,str);
  token = strtok(newStr,COMMAND_DELIM);

  while(token!=NULL){
    token_table[i] = token;
    i++;
    token = strtok(NULL,COMMAND_DELIM);
    if(i>BUFSIZE)break;
  }

  token_table[i] = NULL;
  return token_table;
}

void printPrompt()
{
  char *cwd;
  char *loginUser;
  char *promptMsg;
  promptMsg = (char *)malloc(MAX_CHARS_PROMPT * sizeof(char));
  loginUser = (char *)malloc(MAX_CHARS_USERNAME * sizeof(char));
  loginUser = getlogin();
  if(loginUser == NULL) {
    printf("Couldn't get login user\n");
    exit(EXIT_FAILURE);
  }
  cwd = getcwd(NULL,0);
  strcat(promptMsg, loginUser);
  strcat(promptMsg, "@cs345sh");
  strcat(promptMsg,cwd);
  strcat(promptMsg, "/$ ");
  printf("\n%s", promptMsg);
}


char *readCommand(void){
  size_t bufsize = 0; /* getline decides the bufsize */
  char *line = NULL;
  getline(&line, &bufsize, stdin);
  return line;
}

char* replaceWord(const char* s, const char* oldW,const char* newW){
    char* result;
    int i, cnt = 0;
    int newWlen = strlen(newW);
    int oldWlen = strlen(oldW);

    for (i = 0; s[i] != '\0'; i++) {
        if (strstr(&s[i], oldW) == &s[i]) {
            cnt++;
            i += oldWlen - 1;
        }
    }

    result = (char*)malloc(i + cnt * (newWlen - oldWlen) + 1);

    i = 0;
    while (*s) {
        if (strstr(s, oldW) == s){
            strcpy(&result[i], newW);
            i += newWlen;
            s += oldWlen;
        }
        else
            result[i++] = *s++;
    }

    result[i] = '\0';
    return result;
}


void generateChild (int in, int out, char *pipe){
  char **token = line2tokens(pipe);
  pid_t pid;
  pid = fork();
  if(pid < 0){  /*the creation of a child process was unsuccessful*/
    perror("fork");
    exit(EXIT_FAILURE);
  }
  if (pid == 0){ /* the newly created child */
    if (in != 0) { /* in is not stdin */
      dup2 (in, 0);
      close (in);
    }

    if (out != 1){ /* if out is not stdout */
      dup2 (out, 1);
      close (out);
    }

    execvp(token[0],token);
    printf("Error: %s is not supported by vamprompt", token[0]);
    exit(EXIT_FAILURE);
  }else{ /* the process ID of the child process passes to the parent */
    wait(NULL);
  }
}

char * get_input_file_offset(char *pipe){
  char **token = line2tokens(pipe);
  int i = 0;
  while(token[i] != NULL){
    if(strcmp(token[i],"|") == 0){
      return token[i+1];
    }
    i++;
  }
  return NULL;
}

char * get_ouput_file_offset(char *pipe){
  char **token = line2tokens(pipe);
  int i = 0;
  while(token[i] != NULL){
    if(strcmp(token[i],"||") == 0 || strcmp(token[i],"|||") == 0){
      return token[i+1];
    }
    i++;
  }
  return NULL;
}

char * exclude_read_redirection(char *pipe){
  char **token = line2tokens(pipe);
  char *new_command = (char *)malloc(40);
  int i = 0;
  while(token[i] != NULL){
    if(strcmp(token[i],"|") == 0){
      if(token[i+2] != NULL){
        i+=2;
      }else{
      return new_command;
      }
    }
    strcat(new_command," ");
    strcat(new_command,token[i]);
    i++;
  }
  return new_command;
}

char * exclude_write_redirection(char *pipe){
  char **token = line2tokens(pipe);
  char *new_command = (char *)malloc(40);
  int i = 0;
  while(token[i] != NULL){
    if(strcmp(token[i],"||") == 0){
      return new_command;
    }
    strcat(new_command," ");
    strcat(new_command,token[i]);
    i++;
  }
  return new_command;

}

char * exclude_append_redirection(char *pipe){
  char **token = line2tokens(pipe);
  char *new_command = (char *)malloc(128);
  int i = 0;
  while(token[i] != NULL){
    if(strcmp(token[i],"|||") == 0){
      return new_command;
    }
    strcat(new_command," ");
    strcat(new_command,token[i]);
    i++;
  }
  return new_command;
}

int has_read_rdr(char * pipe){  /* gia ta read */
  char **token = line2tokens(pipe);
  int i = 0;
  while(token[i] != NULL){
    if(strcmp(token[i],"|") == 0){
      return 1;
    }
    i++;
  }
  return 0;
}

int has_write_rdr(char * pipe){ /* gia ta write */
  char **token = line2tokens(pipe);
  int i = 0;
  while(token[i] != NULL){
    if(strcmp(token[i],"||") == 0){
      return 1;
    }
    i++;
  }
  return 0;
}

int has_append_rdr(char * pipe){  /* gia ta append */
  char **token = line2tokens(pipe);
  int i = 0;
  while(token[i] != NULL){
    if(strcmp(token[i],"|||") == 0){
      return 1;
    }
    i++;
  }
  return 0;
}

void print_env_var(char * env_var){
  if(getenv(env_var) == NULL){
    printf("%s=\n",env_var);
  }else{
    printf("%s=%s\n",env_var,getenv(env_var));
  }
}

int exec_commands(int size,char **pipes){
  char **tokens = line2tokens(pipes[0]);
  if(strcmp(tokens[0],"cd") == 0){
    chdir(tokens[1]);
    return 1;
  }else if(strcmp(tokens[0],"exit") == 0) {
    return 0;
  }else if(strcmp(tokens[0],"setenv") == 0){
    setenv(tokens[1],tokens[2],1);
    return 1;
  }else if(strcmp(tokens[0],"unsetenv") == 0){
    unsetenv(tokens[1]);
    return 1;
  }else if(strcmp(tokens[0],"env") == 0){
    print_env_var("HOME");
    print_env_var("PATH");
    return 1;
  }
  else{
    int i;
    int in, fd[2],out;
    char * s_command;
    char * input_file;
    char * output_file;

    in = 0; /* read from STDIN at first */

    for(i = 0; i < size; i++){

      pipe(fd);
      out = fd[1];

      s_command = pipes[i];

      /* if it has has redirections */
      if(strstr(pipes[i],"|") != NULL || strstr(pipes[i],"||") != NULL || strstr(pipes[i],"|||") != NULL){

        if(has_read_rdr(pipes[i])){
          input_file =  get_input_file_offset(pipes[i]);
          s_command = exclude_read_redirection(pipes[i]);
          in = open(input_file,O_RDONLY);
        }

        /* ATTENTION the value of s_command below may have been altered from exclude_read_redirection */
        if(has_write_rdr(pipes[i])){
          output_file =  get_ouput_file_offset(s_command);
          s_command = exclude_write_redirection(s_command);
          out = open(output_file, O_CREAT | O_WRONLY | O_TRUNC,0666);
        }
        else if(has_append_rdr(pipes[i])){
          output_file =  get_ouput_file_offset(s_command);
          s_command = exclude_append_redirection(s_command);
          out = open(output_file, O_CREAT | O_APPEND | O_WRONLY,0666);
        }
      }

      if(i == size-1){
        if(!has_write_rdr(pipes[i]) && !has_append_rdr(pipes[i])){
          out = 1;
        }
        generateChild(in, out, s_command);
        close(fd[0]);
        close(fd[1]);

      }else{
        generateChild(in, out, s_command);
        close (fd [1]);
        in = fd [0]; /* save read-end current pipe */
      }

    }
    return 1;
  }
}

int noCommand(char *command){
  int i;
  for(i = 0; i < strlen(command); i++) if(command[i] != ' ' && command[i] !='\n') return 0;
  return 1;
}

int main(int argc, char **argv){
  int status = 1;
  char *command;

  do{
    printPrompt();
    command = readCommand();
    command = replaceWord(command, "\"", "");
    if(!noCommand(command)) {
      char **pipes = pipe2tokens(command);
      int i = 0;
      while(pipes[i])i++;
      status = exec_commands(i,pipes);
    }
    free(command);
  }while(status);

  return 0;
}
