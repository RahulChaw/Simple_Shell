#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define ARGU_MAX 17
#define CMDLINE_MAX 512
#define DIRNAMES_MAX 20
#define PATHH_MAX 10

int change_dir(char*);
void error_message(int);
void exec_run(char*);
int list_dir_content_size(void);
char* output_redirec(char*);
int piping(char**);
int print_work_dir(void);
int system_copy(char*);

int out_red = 0;

enum
{
        ERR_TOO_MANY_ARGU,
        ERR_MISSING_CMD,
        ERR_NO_OUTPUT_FILE,
        ERR_FAIL_OPEN_OUTPUT_FILE,
        ERR_NOT_CD_DIR,
        ERR_CMD_NOTFOUND,
        ERR_MISLOCATED_OUT_REDIR
};

int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1) {
                char *nl;
                int retval = 0;
                char *command_Line = NULL;

                /* Print prompt */
                printf("sshell@ucd$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl) 
                        *nl = '\0';

                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        fprintf(stderr, "+ completed 'exit' [0]\n");
                        break;
                }

                /* Duplicating commandline */
                command_Line = strdup(cmd);

                /* Regular command */
                retval = system_copy(cmd);
                if (retval == 1)
                {
                        continue;
                }
                else if (retval != 0)
                {
                        retval = 1;
                }
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        command_Line, retval);
        }
        return EXIT_SUCCESS;
}

int system_copy(char* cmd)
{
        char* parse[ARGU_MAX] = {NULL};
        char* found = NULL;
        char* command = NULL;
        char* file_name = NULL;
        char* piping_parse[4] = {NULL};

        pid_t pid;

        int ret_status = 0;
        int counter = 0;
        int status = 0;

        /* Error management */
        if (cmd[strlen(cmd) - 1] == '>')
        {
                error_message(ERR_NO_OUTPUT_FILE);
                return 1;
        }
        if (cmd[strlen(cmd) - 1] == '|')
        {
                error_message(ERR_MISSING_CMD);
                return 1;
        }
        if (cmd[0] == '|' || cmd[0] == '>')
        {
                error_message(ERR_MISSING_CMD);
                return 1;
        }

        if(strchr(cmd, '|'))
        {
                int cmd_num = 0;
                found = strtok(cmd, "|");
                if (strchr(found, '>'))
                {
                        error_message(ERR_MISLOCATED_OUT_REDIR);
                        return 1;
                }
                while(found && cmd_num < 4)
                {
                        piping_parse[cmd_num] = found;
                        found = strtok(NULL, "|");
                        cmd_num++;
                }
                piping(piping_parse);
                return 0;
        }

        /* Checking for output redirection */
        if(strchr(cmd, '>'))
        {
                if (strchr(strchr(cmd, '>'), '/'))
                {
                        error_message(ERR_FAIL_OPEN_OUTPUT_FILE);
                        return 1;
                }
                file_name = output_redirec(cmd);
                found = strtok(cmd, ">");
                cmd = found;
        }
        
        found = strtok(cmd, " ");
        while(found)
        {
                parse[counter] = found;
                found = strtok(NULL, " ");
                counter++;
                if (counter >= ARGU_MAX)
                {
                        error_message(ERR_TOO_MANY_ARGU);
                        return 1;
                }
        }

        command = parse[0];
        pid = fork();
        if(pid == 0) {
                /* Child */
                /* Checking for output redirection flags and performing redirection accordingly */
                if(out_red == 1)
                {
                        int fd;
                        fd = open(file_name, O_WRONLY | O_TRUNC | O_CREAT, 0644);
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                }
                else if(out_red == 2)
                {
                        int fd;
                        fd = open(file_name, O_WRONLY | O_TRUNC | O_CREAT, 0644);
                        dup2(fd, STDOUT_FILENO);
                        dup2(fd, STDERR_FILENO);
                        close(fd);
                }

                /* Checking for cd, pwd or general command*/
                if(!strcmp("cd", command))
                {
                        char* argu = parse[1];
                        ret_status = change_dir(argu);
                        if(ret_status != 0)
                        {
                                error_message(ERR_NOT_CD_DIR);
                        }
                        exit(ret_status);
                }
                else if (!strcmp("pwd", command))
                {
                        ret_status = print_work_dir();
                        exit(ret_status);
                }
                else if (!strcmp("sls", command))
                {
                        ret_status = list_dir_content_size();
                        exit(ret_status);
                }
                else
                {
                        execvp(command, parse);
                        error_message(ERR_CMD_NOTFOUND);
                        exit(1);
                }
        }
        else if (pid > 0) {
                /* Parent */
                waitpid(pid, &status, 0);

                if(!strcmp("cd", command))
                {
                        char* argu = parse[1];
                        change_dir(argu);
                }
                return status;
        }
        else {
                perror("fork");
                exit(1);
        }
        return status; 
}

int change_dir(char* path_name)
{
        int ret_value = 0;
        char* first_char;
        int index;
        char* found;
        char* argu_names[DIRNAMES_MAX] = {NULL};

        first_char = strchr(path_name, '/');
        index = (int)(first_char - path_name);
        if (first_char == NULL)
        {
                /* single path name */
                ret_value = chdir(path_name);
        }
        else if (index == 0)
        {
                /* full path name (starts with /) */
                char* curr_path = NULL;
                char* curr_args[DIRNAMES_MAX] = {NULL}; 
                int loc_counter = 0;
                int curr_counter = 0;
                int counter = 0;
                int i=0;

                /* splitting different directory names in the given path */
                found = strtok(path_name, "/");
                while(found)
                {
                        argu_names[loc_counter] = found;
                        found = strtok(NULL, "/");
                        loc_counter++;
                }
                found = NULL;
                
                /* splitting different directory names in the current location path */
                curr_path = getcwd(curr_path, PATHH_MAX);
                found = strtok(curr_path, "/");
                while(found)
                {
                        curr_args[curr_counter] = found;
                        found = strtok(NULL, "/");
                        curr_counter++;
                }
        
                /* setting counter to the path with lower number of directories */
                if(curr_counter > loc_counter)
                {
                        counter = loc_counter;
                }
                else
                {
                        counter = curr_counter;
                }

                /* finding how many directories in the path are same to the current location */
                while(!strcmp(argu_names[i],curr_args[i]) && i <= counter)
                {
                        i++;
                }

                /* cd back to the directory common in both */
                while(curr_counter - i > 0)
                {
                        chdir("..");
                        curr_counter--;
                }

                /* cd forward to the directory */
                while(loc_counter - i >= 0)
                {
                        ret_value = chdir(argu_names[i]);
                        i++;
                }

                return ret_value;
        }
        else
        {
                /* parent to child dir (not starting with /) */
                found = strtok(path_name, "/");
                int counter = 0;
                while(found)
                {
                        ret_value = chdir(found);
                        found = strtok(NULL, " ");
                        counter++;
                }
        }
        return ret_value;
}

int print_work_dir()
{
        char* name_work_dir = NULL;
        fprintf(stdout, "%s\n", getcwd(name_work_dir, PATHH_MAX));
        return 0;
}

int list_dir_content_size()
{
        char* cwd = NULL;
        char* name_cwd;
        DIR* dirp;
        struct dirent* dp;
        struct stat sb;

        name_cwd = getcwd(cwd, PATHH_MAX); 
        dirp = opendir(name_cwd);
        while ((dp = readdir(dirp)) != NULL)
        {
                /* If found current dir, parent dir, and hidden file */
                if ((!strcmp(dp->d_name, ".")) || (!strcmp(dp->d_name, "..")) || (dp->d_name[0] == '.'))
                {
                        continue;
                }
                stat(dp->d_name, &sb);
                fprintf(stdout, "%s     (%lld bytes)\n", dp->d_name, (long long) sb.st_size); 
        }
        closedir(dirp);
        return 0;
}

int piping(char* piping_parse[])
{
        int ret_status = 0;
        pid_t pid;
        int check_amp[3] = {0,0,0};
        int i=0;

        while(piping_parse[i])
        {
                if(strchr(piping_parse[i], '&'))
                {
                        check_amp[i-1] = 1;
                        piping_parse[i] = strtok(piping_parse[i], "&");
                }
                i++;
        }

        pid = fork();
        if(pid == 0)
        {
                /* Child */
                int fd[2];
                pipe(fd);
                if(fork() != 0)
                {
                        /* Parent */
                        close(fd[0]);
                        dup2(fd[1], STDOUT_FILENO);
                        if(check_amp[0] == 1)
                        {
                                dup2(fd[1], STDERR_FILENO);
                        }
                        close(fd[1]);
                        exec_run(piping_parse[0]);
                        exit(0);
                }
                else
                {
                        /* Child */
                        if(piping_parse[2] != NULL)
                        {
                                /* Pipe 2 */
                                int fd1[2];
                                pipe(fd1);
                                close(fd[1]);
                                dup2(fd[0], STDIN_FILENO);
                                close(fd[0]);
                                if(fork() != 0)
                                {
                                        /* Parent */
                                        close(fd[1]);
                                        dup2(fd[0], STDIN_FILENO);
                                        close(fd[0]);
                                        close(fd1[0]);
                                        dup2(fd1[1], STDOUT_FILENO);
                                        if(check_amp[1] == 1)
                                        {
                                                dup2(fd1[1], STDERR_FILENO);
                                        }
                                        close(fd1[1]);
                                        exec_run(piping_parse[1]);
                                        exit(0);
                                }
                                else
                                {
                                        /* Child */
                                        if(piping_parse[3] != NULL)
                                        {
                                                int fd2[2];
                                                pipe(fd2);
                                                close(fd1[1]);
                                                dup2(fd1[0], STDIN_FILENO);
                                                close(fd1[0]);
                                                if(fork() != 0)
                                                {
                                                        /* Parent */
                                                        close(fd1[1]);
                                                        dup2(fd1[0], STDIN_FILENO);
                                                        close(fd1[0]);
                                                        close(fd2[0]);
                                                        dup2(fd2[1], STDOUT_FILENO);
                                                        if(check_amp[2] == 1)
                                                        {
                                                                dup2(fd2[1], STDERR_FILENO);
                                                        }
                                                        close(fd2[1]);
                                                        exec_run(piping_parse[2]);
                                                        exit(0);
                                                }
                                                else
                                                {
                                                        close(fd2[1]);
                                                        dup2(fd1[0], STDIN_FILENO);
                                                        close(fd1[0]);
                                                        exec_run(piping_parse[3]);
                                                        exit(0);
                                                }
                                        }
                                        else
                                        {
                                                close(fd1[1]);
                                                dup2(fd1[0], STDIN_FILENO);
                                                close(fd1[0]);
                                                exec_run(piping_parse[2]);
                                                exit(0);
                                        }
                                }
                                exit(0);
                        }
                        else
                        {
                                close(fd[1]);
                                dup2(fd[0], STDIN_FILENO);
                                close(fd[0]);
                                exec_run(piping_parse[1]);
                                exit(0);
                        }
                        exit(0);
                }
                exit(0);
        }
        else if(pid > 0)
        {
                /* Parent */
                int status;
                waitpid(pid, &status, 0);
                ret_status = status;
                return status;
        }
        return ret_status;
}

void exec_run(char* piping_parse_code)
{
        char* found = NULL;
        char* command = NULL;
        char* parse[ARGU_MAX] = {NULL};
        int cnt = 0;
        found = strtok(piping_parse_code, " ");
        while(found)
        {
                parse[cnt] = found;
                found = strtok(NULL, " ");
                cnt++;
        }
        command = parse[0];
        execvp(command, parse);
}

void error_message(int error_code)
{
        switch(error_code) {
                case ERR_TOO_MANY_ARGU:
                        fprintf(stderr, "Error: too many process arguments\n");
                        break;
                case ERR_MISSING_CMD:
                        fprintf(stderr, "Error: missing command\n");
                        break;
                case ERR_NO_OUTPUT_FILE:
                        fprintf(stderr, "Error: no output file\n");
                        break;
                case ERR_FAIL_OPEN_OUTPUT_FILE:
                        fprintf(stderr, "Error: cannot open output file\n");
                        break;
                case ERR_NOT_CD_DIR:
                        fprintf(stderr, "Error: cannot cd into directory\n");
                        break;
                case ERR_CMD_NOTFOUND:
                        fprintf(stderr, "Error: command not found\n");
                        break;
                case ERR_MISLOCATED_OUT_REDIR:
                        fprintf(stderr, "Error: mislocated output redirection\n");
                        break;
        } 
}

char* output_redirec(char* cmd)
{
        char* out_parse[ARGU_MAX] = {NULL};
        int out_red1[ARGU_MAX];
        int out_c = 0;
        char* found = NULL;
        char* file_name = NULL;

        found = strchr(cmd, '>');
        found = strtok(found, ">");
        while(found)
        {
                out_red1[out_c] = 1;
                if(strchr(found, '&'))
                {
                        out_red1[out_c] = 2;
                        found = strtok(found, "&");
                }
                out_parse[out_c] = found;
                found = strtok(NULL, ">");
                out_c++;
        }
        found = strtok(out_parse[out_c-1], " ");
        file_name = found;
        out_red = out_red1[out_c-1];
        cmd = strtok(cmd, ">");
        return file_name;
}