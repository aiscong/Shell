#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//when > fails, didnt not kill the child process ---- solved
//another bug, args after echo including > would be written in to the file
int isBuildin(const char *cmd){
  char *exit = "exit";
  char *cd = "cd";
  char *pwd = "pwd";
  if(strcmp(cmd, exit) == 0){
    return 0;
  }else if (strcmp(cmd, cd) == 0){
    return 1;
  }else if (strcmp(cmd, pwd) == 0){
    return 2;
  }
  //if its not a build-in cmd
  return -1;
}

char * pwd();
void runfirst(int fd[], char* cmd[]);
void runsec(int fd[], char* cmd[]);
void rundirect(char *cmd[]);
void runappend(char *cmd[]);
int cd(const char * path);

int main(){
  while(1){
    printf("%s", "mysh> ");
    char in[1024];
    char *cmd[1025];
    fgets(in, 1024, stdin);  
    //To do: need to check the return value of fgets
    //To do: need to parse the input string in to an array of strings <- done
    char * temp;
    temp = strtok(in, "  \n");
    cmd[0] = temp;
    int i = 1; // index to put splited strings into array
    while (temp != NULL){
      //      printf ("%s\n",temp);
      temp = strtok (NULL, "  \n");
      cmd[i] = temp;
      i++;
    }
    if(cmd[0] != NULL){
      //build-in exit is called
      if(isBuildin(cmd[0]) == 0){
	if(cmd[1] == NULL){
	  return 0;
	  //command does not follow proper syntax
	}else{
	  fprintf(stderr, "Error!\n");
	}
	//build-in cd command is called  
      }else if(isBuildin(cmd[0]) == 1){
	//TODO: implement the cd command
	if(cmd[1] == NULL){
	  // printf("Home directory is: %s\n", getenv("HOME"));
	  if(chdir(getenv("HOME")) == -1){
	    fprintf(stderr, "Error!\n");
	  }
	}else if(cmd[1] != NULL && cmd[2] == NULL){
	  cd(cmd[1]);
	  //command does not follow proper syntax
	}else{
	  fprintf(stderr, "Error!\n");
	}
	//TODO: absolute path and relative path strcat; need to check if cmd[2]==NULL or not proper syntax
	//build-in pwd command is called
      }else if(isBuildin(cmd[0]) == 2){
	if(cmd[1] == NULL){
	  char * cdir = pwd();
	  if(cdir != NULL){
	    printf("%s\n", cdir);
	    free(cdir);
	  }
	  //command does not follow proper syntax
	}else{
	  fprintf(stderr, "Error!\n");
	} 
      }
      //Not build-in commands
      else{
	int error = 0;
	if(findc(cmd) == 0){
	  rundirect(cmd);
	}// ">" 
	
	else if (findc(cmd) == 1){
	  runappend(cmd);
	}// >>

	else if(findc(cmd) == 2){
	  int fd[2];
	  int e = 0;
	  int i = findi(cmd);
	  cmd[i] = NULL;
	  if(pipe(fd) == 0){
	    runfirst(fd, cmd);
	    runsec(fd, &cmd[i+1]);
       	    close(fd[0]);
	    close(fd[1]);
	    wait();
	  }
	  else{
	    fprintf(stderr, "Error!\n");
	  }
	} // pipe 

	else{
	  int rc = fork();
	  if(rc == 0){
	    // if no error occured, execute the command in the child process
	    execvp(cmd[0], cmd);
	    // fail to run the cmd or program                                                                                                                
	    fprintf(stderr, "Error!\n");                                                                                                    
	    //if failure occurs, kill the child process 
	    kill(getpid(), SIGKILL);
	  }
	  else if (rc > 0){
	    //parent waiting for child process to finish
	    wait();
	  }
	  //fork failed 
	  else{
	    fprintf(stderr, "Error!\n");
	  }
	}
      } // end of non-buildin command
    }
  } // end of first arg is not NULL
  return 0;
}

void rundirect(char* cmd[]){
  int rc = fork();
  int error = 0;
  if(rc == 0){
    int error = 0;
    int i = findi(cmd, ">");
    if(cmd[i+2] != NULL){
      fprintf(stderr, "Error!\n");
      error = 1;
    }else{
      cmd[i] = NULL;
      int file = open(cmd[i+1], O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
      //fail to open the file
      if(file < 0){
	error = 1;
	fprintf(stderr, "Error!\n");
      }else{
	//Now we redirect standard output to the file using dup2
	if(dup2(file,1) < 0){
	  error = 1;
	  //fail to redirect
	  fprintf(stderr, "Error!\n");
	} // end of redirecting checking
      }// end of open the file checking
    } // proper syntax for >

    if(error == 0){
      execvp(cmd[0], cmd);
      // fail to run the cmd or program                                                                                                                
      fprintf(stderr, "Error!\n");                                                                                                    
    }
    //if failure occurs, kill the child process 
    kill(getpid(), SIGKILL);
  }
  else if (rc > 0){
    //parent waiting for child process to finish
    wait();
  }//fork failed 
  else{
    fprintf(stderr, "Error!\n");
  }
}

void runappend(char* cmd[]){
  int rc = fork();
  int error = 0;
  if(rc == 0){
    int i = findi(cmd, ">>");
    if(cmd[i+2] != NULL){
      fprintf(stderr, "Error!\n");
      error = 1;
    }else{
      cmd[i] = NULL;
      int file = open(cmd[i+1], O_RDWR | O_APPEND);
      if(file < 0){
	error = 1;
	fprintf(stderr, "Error!\n");
      }else{
	if(dup2(file, 1) < 0){
	  error = 1;
	  fprintf(stderr, "Error!\n");
	} //end of redirecting checking                                                                                                                                                                
      }// end of open the file checking                                                                                                                                                                 
    } // end of proper syntax
    if(error == 0){
      execvp(cmd[0], cmd);
      // fail to run the cmd or program                                                                                                                                                                       
      fprintf(stderr, "Error!\n");
    }
    //if failure occurs, kill the child process                                                                                                                                                               
    kill(getpid(), SIGKILL);
  } // end of child process 
  else if (rc > 0){
    //parent waiting for child process to finish                                                                                                                                                              
    wait();
  }//fork failed                                                                                                                                                                                             
  else{
    fprintf(stderr, "Error!\n");
  }
}
// first process, this end becomes stdout
void runfirst(int fd[], char* cmd[]){
  int rc = fork();
  if(rc == 0){
    close(fd[0]);
    int i = 0;
    if(dup2(fd[1], 1) != -1){
      execvp(cmd[0], cmd);
      fprintf(stderr, "Error!\n");
      kill(getpid(), SIGKILL);
    }else{
      fprintf(stderr, "Error!\n");
      kill(getpid(), SIGKILL);
    }
  } // end of child proc
  else if(rc > 0){
    wait();
  }//fork failed
  else{
    fprintf(stderr, "Error!\n");
  }       
}

// second proc, this end becomes stdout
void runsec(int fd[], char* cmd[]){
  int rc = fork();
  if(rc == 0){
    //Child process                                                                                                                                          
    //close this end                                                                                                                                         
    close(fd[1]);
    if(dup2(fd[0], 0) != -1){
      execvp(cmd[0], cmd);
      fprintf(stderr, "Error!\n");
      kill(getpid(), SIGKILL);
    }else{
      fprintf(stderr, "Error!\n");
      kill(getpid(), SIGKILL);
    }
  } // end of child proc                                                                                                                                     
  else if(rc > 0){
    // parent close pipe in main and parent wait 
  }
  else{
    fprintf(stderr, "Error!\n");
  }
}

int findi(const char* cmd[], char *key){
  int i;
  // printf("we are checking string %s\n", key);
  while (cmd[i] != NULL){
    //  printf("the current string in the command is %s\n", cmd[i]);
    if(strcmp(cmd[i], key) == 0){
      // printf("the current index is %d\n", i);
      return i;
    }
    i++;
  }
  return -1;
}

int findc(const char* cmd[]){
  int i = 0;
  while(cmd[i] != NULL){
    if(strcmp(cmd[i], ">") == 0){
      return 0;
    }
    if(strcmp(cmd[i], ">>") == 0){
      return 1;
    }
    if(strcmp(cmd[i], "|") == 0){
      return 2;
    }
    i++;
  }
  return -1;
}

int cd(const char* path){
  if(path[0] == '/'){
    if(chdir(path) == -1){
      fprintf(stderr, "Error!\n");
      return -1;
    }
  }else{
    //TODO: relative address
    char *cdir = pwd();
    //correctly get the current address
    //if not, get NULL pointer from pwd
    //the error msg has been printed in pwd function
    if(cdir != NULL){
      // char ddir[1024];
      // strcpy(ddir, path);
      strcat(cdir, "/");
      //printf("the combined addr is %s\n", cdir);
      //  strcat(
      strcat(cdir, path);
      if(chdir(cdir) == -1){
	fprintf(stderr, "Error!\n");
	free(cdir);
	return -1;
      }
      free(cdir);
    }else{
      return -1;
    }
    //  printf("current path is %s\n", path);
  }
  return 0; // indicate success change dir
}


char * pwd(){
  long size;
  char *path;
  size = pathconf(".", _PC_PATH_MAX);
  // printf("size is %d\n", size);
  if((path = (char *) malloc((size_t)size)) != NULL){
    if(getcwd(path, (size_t)size) != NULL){
      // printf("here\n");                                                                                         
      return path;
      // free(path);
    }else{
      // fprintf(stderr, "Error!\n");
      free(path);
    }
  }
  return NULL;
}

