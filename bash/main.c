#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include<sys/wait.h> 
#include "header.h"


#define READ 0
#define WRITE 1


//env -i myshell to clear out all the environment variables

char array_export[MAX_BUFSIZE][MAX_BUFSIZE] = {"PATH="};
char array_history[MAX_BUFSIZE][MAX_BUFSIZE];



char *new[100]; //new export arggument
char *stored[100]; //old export arguments

int PWD_bool = 0; //checks if we run the program for the first time


//char *read_line(void);
char **parse(char *line);
int execute(char **args, int size_args);
void loop(void);
void add_to_history(char *line);
static void parseline(char *cmdline, char **argv);
int forking(char **args);
char **parse_pipes(char *line);
int execute_pipe(char **args, char *line);
int redirect_output(char **args, int size_args);
int redirect_input(char **args, int size_args);
void pipeline(char ***cmd, int number_elemnts);
int cd(char **args, int size_args);
int pwd(char **args, int size_args);
int export(char **args, int size_args);
int history(char **args, int size_args);
int get_command_number(char **args, int size_args);
int exit_program(char **args, int size_args);
int external_command(char **args, int size_args, int redirect_out_flag, int saved_stdout, int saved_stderr);


//list of builtin commands
char *builtin_str[] = {
  "cd",
  "pwd",
  "history",
  "export",
  "get_command_number",
  "exit_program"
};

int (*builtin_func[]) (char **) = {
  &cd,
  &pwd,
  &history,
  &export,
  &get_command_number,
  &exit_program
};

//number of commands
int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}



int main(int argc, char **argv)
{ 
    //Refferd to a tutorial
    //source: https://brennan.io/2015/01/16/write-a-shell-in-c/
    char *line; //line to be inputed by the user
    char **args; //an array that stores the parsed line inputed by the user
    int status = 1; //returns 0 if the shell is exsited otherwise 1
    char **args_pipe; //an array of pipes if any pipes are identified 
    int status_pipes =1; //returns 1 on success for execution of pipes



    //insert PWD with current directory into the array_export
    char pwd_name[] = "PWD=";
    
    char cwd[1024]; //current working directory

        if(getcwd(cwd, sizeof(cwd)) == NULL){

            perror("error");
        }else{
        
        printf("%s\n", cwd); 
        }


    char src[50], dest[50];

    strcpy(src,  cwd);
    strcpy(dest, pwd_name);

    strcat(dest, src);



    char *argument_pwd[50] = {"export", dest, NULL};
    export(argument_pwd, 50); //export the PWD current directory at the begining of the programme 
 



    do {
    printf(">");

    char copy_line[100];

    line = read_line(); //read the line inputed by the user

    strcpy(copy_line,line);
    add_to_history(line); //input the line inputed by the user in history
    args_pipe = parse_pipes(line);

    int count = 0;
    while(args_pipe[count] != NULL){
        count++;
    }
    args = parse(copy_line); // parse the input 

    
    int number_pipes = 0; 

    while(args_pipe[number_pipes]!=NULL){
        number_pipes++;
    }

    int number_el = 0;

    while(args[number_el] != NULL){
        number_el++;

    }

    
    if(number_el == 0 || number_pipes < 2){

        status = execute(args, number_el);

    }
    else{
    
    status_pipes = execute_pipe(args_pipe, line);
    }


    //free the allocated space
    free(line);
    free(args);
    free(args_pipe);

  } while (status_pipes && status);

  
  return EXIT_SUCCESS;
}

//parses the command line into seperate words
//Refferd to source: https://brennan.io/2015/01/16/write-a-shell-in-c/
char **parse(char *line){

    //printf("LINE IN PARSE HOWEVER IS %s\n", line);
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token, **tokens_backup;
    
    if (!tokens) {
        fprintf(stderr, "allocation error\n");
        exit(EXIT_FAILURE);
    }
    
    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;
        
        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            tokens_backup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                free(tokens_backup);
                fprintf(stderr, "allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        
        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
    
  
  return tokens;

	
}


//execution of the shell
int execute(char **args, int size_args){

    int redirect_out_flag = 0;
    int saved_stdout = 0;
    int saved_stderr = 0;
    int redirect_in_flag = 0;
    int saved_stdin = 0;


    //check for the extension

    for(int loop = 0; loop < size_args; loop++){
        //search for a $ sign
        if(args[loop][0] == '$'){

            int length_path = strlen(args[loop]);
            
            //creating a variable that stores everythhing after the $ Sign
            char rest[length_path-1];

            int pos = 0;

            while(pos < length_path){

                rest[pos] = args[loop][pos+1];
                pos++;
            }


            int position = 0;
            
            //loops through the already exported arguments
            while(array_export[position][0] != 0){

                //parses the already exported arguments stored in an array
                parseline(array_export[position], stored);
                //compares the old with the new
                if(strcmp(rest, stored[0]) == 0){
                    //if there is a match exchanges them and returns
                    
                    char *ret;
                    ret = strchr(array_export[position], '=');
                    //gets everything after the = sign from the environment
                    int length_left = strlen(ret);
                    char after[length_left-1];

                    int posit = 0;

                    while(posit < length_left){
                        after[posit] = ret[posit+1];
                        posit++;
                    }
                    //exchanges $variable with the path
                    strcpy(args[loop], after);
                    
                    position++;
                }

                else{

                    position++; 

                }

            }


            
    
        }

    }


    //checking for the > symbol
    for(int i = 0; i < size_args; i++){

        if(strcmp(args[i], ">") == 0){

            //printf("Found it! \n");
            return redirect_output(args, size_args);
        }
    }

    //checking for the < symbol
    for(int i=0; i < size_args; i++){

        if(strcmp(args[i], "<") == 0){

            //printf("FOUND < symbol \n");
            return redirect_input(args, size_args);
        }
    }
    
    
    int i;

    //if no arguments entered return 
    if(args[0] == NULL){
        
        return 1;
    }



    //change of directory, cd
    if(strcmp(args[0], "cd") == 0){

      return cd(args, size_args);
    }

     //check current directory
    //source: https://stackoverflow.com/questions/16285623/how-to-get-the-to-get-path-to-the-current-file-pwd-in-linux-from-c
    if(strcmp(args[0], "pwd") == 0){

        return pwd(args, size_args);
    }

    //implementation of export
    if(strcmp(args[0], "export") == 0){

        return export(args, size_args);

    }

    //implementation of history

    if(strcmp(args[0], "history") == 0){

        return history(args, size_args);

    }

    //implementation of !
    if(strncmp(args[0], "!", 1) == 0){

        return get_command_number(args, size_args);


    }

    //exit the shell
    if(strcmp(args[0], "exit") == 0){

        return exit_program(args, size_args);
    }

    return external_command(args, size_args, redirect_out_flag, saved_stdout, saved_stderr);
    
}

//addts to history file
void add_to_history(char *line){

    FILE *fp = fopen("history.txt", "a+");

    if (fp == NULL) 
        { 
            puts("Couldn't open file or file already exists"); 
            exit(0); 
        } 
        else
        { 
            fputs(line, fp); 
            //puts("Done"); 
            fclose(fp); 
        }  


}

//source: https://github.com/ptg180/CSO--Shell-Lab/blob/master/tsh.c
static void parseline(char *cmdline, char **argv) 
{
    char array[MAX_BUFSIZE]; // reads line
    char *buf = array;       //traverses through line
    char *delim;             //points to first = delimiter
    int argc;                //number of agrs

    
    strcpy(buf, cmdline);
    
    //build argv list
    argc = 0;
  
    delim = strchr(buf, '=');
   
    
    while (delim) {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) 
            buf++;
        
       
 
    
        delim = strchr(buf, '=');
    
        
    }

    argv[argc] = NULL;
     
}

int forking(char **args){
    
    pid_t child_pid;
    int stat_loc;
    
    child_pid = fork();
    
    if(child_pid < 0){
        
        perror("Fork failed");
        exit(1);
    }
    
    else if(child_pid == 0){
        //Child Process
        //printf("INSIDE FORK ARGS[0] %s\n", args[0]);
        if(execvp(args[0], args) < 0){
            perror(args[0]);
            exit(1);
            }
        //exit(EXIT_FAILURE);
    } else {
        //Parent Process
        
        do{
        waitpid(child_pid, &stat_loc, WUNTRACED);
        } while(!WIFEXITED(stat_loc) && !WIFSIGNALED(stat_loc));
    }
    
    return 1;
    
}

char **parse_pipes(char *line){

    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token, **tokens_backup;
    
    if (!tokens) {
        fprintf(stderr, "allocation error\n");
        exit(EXIT_FAILURE);
    }
    
    token = strtok(line, "|");
    while (token != NULL) {
        
        tokens[position] = token;
        
        position++;
        
        /*if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            tokens_backup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                free(tokens_backup);
                fprintf(stderr, "allocation error\n");
                exit(EXIT_FAILURE);
            }
        } */
        
        token = strtok(NULL, "|");
    }
    tokens[position] = NULL;
    //return tokens;

    /*int count = 0;
    while(tokens[count] != NULL){
        printf("TOKEN IS %s\n",tokens[count]);
        count++;
    } */

  
  return tokens;


}

int execute_pipe(char **args, char *line){

    //number of pipes + 1
    int number_pipes = 0; //number of pipe commands

    while(args[number_pipes]!=NULL){
        //printf("PIPE ELEMENTS %s\n", args[number_pipes]);
        number_pipes++;
    }

    //char **cmd[number_pipes];
    char *cat_cmd[]={"/usr/bin/grep", "orange", NULL};

    int cat_size = 0;
    while(cat_cmd[cat_size] != NULL){
        cat_size++;
    }

    char **cmd[number_pipes+1];
    
    for(int i = 0; i < number_pipes; i++){

        cmd[i] = parse(args[i]);
    }

    cmd[number_pipes] = NULL;

    int counter = 0;
    while(cmd[counter] != NULL){
        counter++;
    }

    
    //creating the pipe
    int elements = number_pipes+1;
    pipeline(cmd, elements);

      
    return 1;
}


//impplementation of redirect output
int redirect_output(char **args, int size_args){

    int external_bool = 1;
    int pid;
    int redirect_out_flag = 1;
    int saved_stdout;
    int saved_stderr;



    //find the number of elements befor the > sign
    int before_elem = 0;
    while(strcmp(args[before_elem], ">") != 0){
        before_elem++;
    }


    char *command[before_elem];

    for(int i = 0; i < before_elem; i++){
        command[i] = (char *)malloc(50*sizeof(char));
    }

    for(int i = 0; i<before_elem; i++){

        for(int j =0; j < 50; j++){

            command[i][j] = args[i][j];
        }
    }

    command[before_elem] = NULL;

    //after element is at position before_elem+1


    int count = 0;
    while(command[count] != NULL){
        count++;
    }

    if ((pid = fork()) == -1) {
            perror("fork");
            exit(1);
    }

    else if (pid == 0)
    {
    // child
    int fd = open(args[before_elem+1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    saved_stdout = dup(1);
    saved_stderr = dup(2);
    dup2(fd, 1);   // make stdout go to file
    dup2(fd, 2);   // make stderr go to file 
                   

    close(fd);     // fd no longer needed - the dup'ed handles are sufficient

    //check if it is a built in command
    for (int broj = 0; broj < lsh_num_builtins(); broj++) {
    if (strcmp(command[0], builtin_str[broj]) == 0) {

        external_bool = 0;
        (*builtin_func[broj])(command);
    }
    }

    if(external_bool == 1){

        external_command(command,before_elem, redirect_out_flag, saved_stdout, saved_stderr);
        //execvp(command[0],command); 
    }
    exit(1);
    }
    else{
    wait(NULL); 

    free(command[before_elem]);
    }
    return 1;
}

int redirect_input(char **args, int size_args){

    int external_bool = 1; // 1 if external command
    int pid;
    int redirect_in_flag = 1;
    int saved_stdin;


    //printf("Input entered\n");

    //find the elements before the < symbol
    int before_elem = 0;
    while(strcmp(args[before_elem], "<") != 0){
        before_elem++;
    }

    //number of elements after the < sign
    int rest = size_args - (before_elem+1);


    char *command[before_elem+1];
    int count = 0;

    //elements after the < sign
    for(int i=0; i < before_elem; i++){

        command[count] = args[i];
        count++;
    }


    command[before_elem] = NULL;

    if ((pid = fork()) == -1) {
            perror("fork");
            exit(1);
    }
    else if (pid == 0)
    {

    int in;
    
    in = open(args[before_elem+1], O_RDONLY);
    //printf("FILE OPENED IS %s\n", args[0]);
    dup2(in, 0); //change standard input
    close(in);

    //check if it is a built in command
    for (int broj = 0; broj < lsh_num_builtins(); broj++) {
    if (strcmp(command[0], builtin_str[broj]) == 0) {

        external_bool = 0;
        (*builtin_func[broj])(command);
    }
    }

    if(external_bool == 1){

        
        execvp(command[0],command);
    }
    exit(1);
    }
    else{
    wait(NULL); 
    }
    return 1;
}

void pipeline(char ***cmd, int number_elemnts){

    int fd[2];
    pid_t pid;
    int fdd = 0;
    int bool_out = 0; // if there is redirected output bool_out is set to 1
    int bool_in = 0; //if there is redirection of input bool_in is set to 1
    int file_d; // file descriptor output
    int in; //file descriptor input

    int count = 0;

    //searching last command for redirection of output
    while(cmd[number_elemnts-2][count] != NULL){

        

        //find redirection of output
        if(strcmp(cmd[number_elemnts-2][count],">") == 0){

            bool_out = 1;

            //printf("Output redirect found! \n");

            int before_elem = 0;
            while(strcmp(cmd[number_elemnts-2][before_elem], ">") != 0){
                before_elem++;
            }

            //printf("Before elements number is %d\n", before_elem);

            //after element is at spot before_elem+1
            //printf("AFTER ELEMENT IS %s\n",cmd[number_elemnts-2][before_elem+1]);
            char copy_file[100];
            strcpy(copy_file,cmd[number_elemnts-2][before_elem+1]);
            file_d = open(copy_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            //printf("%d\n", file_d);
            

            char *command_1[before_elem+1];
            int counter = 0;

            for(int j=0; j < before_elem; j++){

                command_1[counter] = cmd[number_elemnts-2][j];
                counter++;
            }

            command_1[before_elem] = NULL;
            
            //reassigning value to cmd

            cmd[number_elemnts-2][before_elem] = NULL;


            break;

                    
        }

        count++;
    }

    //searching first element for input redirection

    if(strcmp(cmd[0][0], "<") == 0){

        //printf("Found input redirect!\n");
        bool_in = 1;

        // position 1 will be the file, make a coppy and open the file
        char copy_file_input[100];
        strcpy(copy_file_input,cmd[0][1]);

        //printf("FILE OPENED IS %s\n", copy_file_input);
        //open file for read only
        in = open(copy_file_input, O_RDONLY);

        int allelements = 0;
        //all the elemetns stored in the first process seperated by |
        while(cmd[0][allelements] != NULL){
            allelements++;
        }

        //the number of elements after the < file command and before the | sign
        int afterelements = allelements - 2;
        //printf("AFTER ELEMENTS %d\n", afterelements);

        //store all the elements after the file into a char array
        char *command_2[afterelements+1];
        int position = 0;
        for(int begin = 2; begin < allelements; begin++){

            command_2[position] = cmd[0][begin];
            //printf("CMD2 E %s\n", command_2[position]);
            position++; 
        }

        command_2[afterelements] = NULL;
        

        //modify cmd such that first element is everything after the file name and before the pipe
        //cmd[0] = command_2;

        for(int k = 0; k < afterelements + 1; k++){

            cmd[0][k] = command_2[k];

        }
        

    }


    int i=0;
    int external_bool = 1;

    //implementation of pipes
    for(i=0; i < number_elemnts; i++){

        pipe(fd);               
        if ((pid = fork()) == -1) {
            perror("fork");
            exit(1);
        }
        else if (pid == 0) {

            dup2(fdd, 0);

            //check if it is the last element and if there is a file to be opened
            if(i == number_elemnts-2 && bool_out == 1){
                    dup2(file_d, WRITE); //standard output goes to file  
                    dup2(file_d, 2); //standard error goes to file
                    close(file_d);     

            } 

            if (*(cmd + 1) != NULL) {

                dup2(fd[WRITE], WRITE);

                
            }

            //if first element and there is an input redirection modify standard input
            if(i == 0 && bool_in == 1){

                dup2(in, 0);
                close(in);
            }

            //printf("printf1\n");
            close(fd[READ]);

            //check if it is an external command
            for (int broj = 0; broj < lsh_num_builtins(); broj++) {
                //printf("BUILTIN%s\n", builtin_str[broj]);
                //printf("CMD IS %s\n", (*cmd)[0]);
                if (strcmp((*cmd)[0], builtin_str[broj]) == 0) {

                external_bool = 0;
                //printf("IT should enter here %s\n", (*cmd)[0]);
                (*builtin_func[broj])(*cmd);
                }
            }

            if(external_bool == 1){
            execvp((*cmd)[0], *cmd);
            }

            exit(1);
        }
        else {
            wait(NULL);     
            close(fd[WRITE]);
            fdd = fd[READ];
            cmd++;
        }
    }   
}

int cd(char **args, int size_args){

 

    if (args[1] == NULL) {
        fprintf(stderr, "expected argument to \"cd\"\n");

      } else {

        //chdri returns 0 if succesful
        if (chdir(args[1]) != 0) {
          perror("error");
        }

      }

    //update PWD every time you change a working directory
    char pwd_name[] = "PWD=";

    char cwd[1024];

        if(getcwd(cwd, sizeof(cwd)) == NULL){

            perror("error");
        }else{
        
        printf("%s\n", cwd); 
        }


    char src[50], dest[50];

    strcpy(src,  cwd);
    strcpy(dest, pwd_name);

    strcat(dest, src);

    char *argument_pwd[50] = {"export", dest, NULL};
    export(argument_pwd, 50); //export the PWD current directory at the begining of the programme

    int position = 0;
            
            //loops through the already exported arguments
            while(array_export[position][0] != 0){

                //update environment every time the directory has been changed
                putenv(array_export[position]);
                position++;

            }

    return 1;
}

int pwd(char **args, int size_args){

    //maximum size of a lince
        char cwd[1024];

        if(getcwd(cwd, sizeof(cwd)) == NULL){

            perror("error");
        }else{
        
        printf("%s\n", cwd); 
        }

        //printf("CWD IS %s\n", cwd);

    return 1;
}

int export(char **args, int size_args){

    //umber of elements in args, could be done with str len too
        int number_of_elements = 0;

        while(args[number_of_elements] != NULL){

            number_of_elements++;
        }

        //if no argument inputed, print whatever is exported
        if(args[1] == NULL){

        int count = 0;

        while(array_export[count][0] != 0){

            printf("%s\n", array_export[count]);

            count++;
            
        }

        }
        //if number of elements larger than 2, that would be too many arguments
        else if(number_of_elements > 2){

            printf("Too many arguments passed\n");

        //if number of arguments == 2    
        }else{

            //change environment variable when something is exported
            char *var = args[1];
            int ret;

            ret = putenv(var);
   
            //parses the second argument passed 
            //reffered to source: https://github.com/ptg180/CSO--Shell-Lab/blob/master/tsh.c
            char array[MAX_BUFSIZE]; //holds copy
            char *buf = array;       //ptr that traverses array
            char *delim;            //points to first = delimiter
            int argc;               //number of args 

            
            strcpy(buf, args[1]);
            
            argc = 0;
          
            delim = strchr(buf, '=');
           
            
            while (delim) {
                new[argc++] = buf;
                *delim = '\0';
                buf = delim + 1;
                while (*buf && (*buf == ' ')) 
                    buf++;
                
               
         
            
                delim = strchr(buf, '=');
            
                
            }

            new[argc] = NULL; //stores it in array new


            

            int position = 0;
            
            //loops through the already exported arguments
            while(array_export[position][0] != 0){

                //parses the already exported arguments stored in an array
                parseline(array_export[position], stored);
                //compares the old with the new
                if(strcmp(new[0], stored[0]) == 0){
                    //if there is a match exchanges them and returns
                    strcpy(array_export[position], args[1]);
                    return 1;
                }

                else{

                    position++; 

                }

            }

            //if no arguemnst of same name exist, add it at the end of the array
            strcpy(array_export[position], args[1]);
        }

    return 1;
}

int history(char **args, int size_args){
   
    int number_of_elements = 0;

        while(args[number_of_elements] != NULL){

            number_of_elements++;
        }

        //if more than history passed, too many elements 
        if(number_of_elements > 1){

            printf("Too many arguments\n");
            return 1;
        }

        //reading line by line from the history file
        //reffered to source: https://stackoverflow.com/questions/3501338/c-read-file-line-by-line
        FILE * fp; //file pointer
        char * line = NULL; //initialization of line
        size_t len = 0; 
        ssize_t read;


        fp = fopen("history.txt", "a+"); //open file
        rewind(fp);
        if (fp == NULL) 
        { 
            puts("Couldn't open file or file already exists"); 
            exit(0); 
        } 

        int i = 1;
        int broj = 1;
        
        //rread the file and store it in the history file
        while ((read = getline(&line, &len, fp)) != -1) {


            strcpy(array_history[i], line); //copy into history array

            printf("%d ", i); //print number

            printf("%s", line); //print the line

            i++;
        }
        rewind(fp);
        fclose(fp); //closing the file
        if (line)
            free(line);


        int j = 1;

        while(array_history[j][0] != 0){

            j++;
        }

    return 1;
}

int get_command_number(char **args, int size_args){

    int number_of_elements = 0;

        while(args[number_of_elements] != NULL){

            number_of_elements++;
        }

        if(number_of_elements > 1){

            printf("Too many arguments\n");
            return 1;
        }

        int len; 

        //printf("%c\n", args[0][1]);

        len = strlen(args[0]);
        char ostanato[len-1]; //stores everything after the ! character
        int i;

        for(i =0; i<len-1; i++){

            strcpy(&ostanato[i], &args[0][i+1]);
        }


        int j;

        for(j = 0; j < len-1 ; j++){
        }

        int result = atoi(ostanato); //convert the number into numeric
        
        //checks for out of range
        if(result < 0){

            printf("Line out of range\n");
            return 1;
        }

        int count = 1;

        while(array_history[count][0] != 0){

            count++;
            
        }

        //checks for out or range
        if(result > count - 1){
            printf("Line out of range here\n");
            return 1;
        }

        printf("The invoked command from history file is %s\n",array_history[result]);

    return 1;
}

int exit_program(char **args, int size_args){

    return 0;
}

int external_command(char **args, int size_args, int redirect_out_flag, int saved_stdout, int saved_stderr){

    char *ptr;
    int len;
    struct dirent *pDirent;
    DIR *pDir;

    char copy_path[100][100];


    strcpy(copy_path, array_export[0]);


    ptr = strchr(array_export[0], '=');
    ptr = &ptr[1];

    

    //tokenize
    char *token;

    token = strtok(ptr, ":");

    while( token != NULL ) {
      //printf( " %s\n", token );

      //check if directory exists
      if(access(token, F_OK) == 0){

      pDir = opendir (token);
        if (pDir == NULL) {
            printf ("Cannot open directory '%s'\n", token);
            
        }

        while ((pDirent = readdir(pDir)) != NULL) {

            //check if it starts ./ and try executing it if the folder of gcc has been exported
            if(args[0][0] == '.' && args[0][1] == '/'){

                if(strcmp(pDirent->d_name,"gcc") == 0){

                     forking(args);
                     return 1;
                }

               
            }

            if(strcmp(pDirent->d_name,args[0]) == 0){

            if(redirect_out_flag != 1){

            printf("Command found\n");
            printf("%s is an external command (%s/%s) \n", args[0], token, args[0]);
            printf("command arguments:\n");

            forking(args);
            }

            else{

                forking(args);
            }

            int command_arguments = 1;

            while(args[command_arguments] != NULL){

                //printf("%s\n",args[command_arguments]);
                command_arguments++;
            }

            strcpy(array_export[0],copy_path);

            return 1;

                
            }
            //printf ("[%s]\n", pDirent->d_name);
        }
        closedir (pDir);

    }


     
    
      token = strtok(NULL, ":");
   }

   if(redirect_out_flag == 1){

        dup2(saved_stdout, 1);
        dup2(saved_stderr, 2);
   }

   printf("command not found\n");
   strcpy(array_export[0],copy_path);
   
   

    return 1;
}



