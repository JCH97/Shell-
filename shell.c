#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "../include/parser.h"

#define register_size 51
#define buffer_size 1024

char commandSave[register_size][buffer_size];
int commandSaveSize[register_size], start_register = 0,  end_register = 0, indexLineToSave = -1;
const char *endl = "\n";
char *defaultPath, *dir;

void print_history(int fd) {
    int i, j = 1;
    char temp[buffer_size], *bet = " -- ";
	for (i = start_register; i < end_register; i = (i + 1) % register_size, ++j) { 
		sprintf(temp, "%d", j);
        write(fd, temp, strlen(temp));
        write(fd, bet, strlen(bet));
        write(fd, commandSave[i], commandSaveSize[i]);
    }
}

int write_prompt() {
	char path[1024], temp[1024] = ":~$ \0";
    getcwd(path, 1024);

	write(STDIN_FILENO, path, strlen(path));
    write(STDIN_FILENO, temp, strlen(temp));
}

void read_command(command *cmd) {
	char buffer[buffer_size];
	int size = 0;

	if(indexLineToSave == -1) 
		size = read(STDIN_FILENO, buffer, buffer_size);	
	else {
		size = commandSaveSize[indexLineToSave];
        strncpy(buffer, commandSave[indexLineToSave], size);
		indexLineToSave = -1;
	}

	if(buffer[0] != '!' && buffer[0] != ' ' && (end_register == 0 ||  strncmp(commandSave[end_register - 1], buffer, commandSaveSize[end_register - 1]))) {
		commandSaveSize[end_register] = size;
		strncpy(commandSave[end_register], buffer, size);
		end_register = (end_register + 1) % register_size;
	}

	if(end_register == start_register)
		start_register = (start_register + 1) % register_size;	
		
	parse_command(buffer, size, cmd);	
}

int getNumberLine(simple_command *cmd) {
    int i, ans = 0;
    for(i = 1; i < strlen(cmd->_tokens[0]) ; ++i)
        ans = ans * 10 + (cmd->_tokens[0][i] - '0');

    return ans - 1;
}

int get_outfile(simple_command *cmd) {
	int i, outfile = STDOUT_FILENO, size_outfile = cmd->_no_outfiles;

	if(size_outfile > 0) {
		for (i = 0; i < cmd->_no_outfiles; ++i) {
	        if(i > 0)
				close (outfile);
			if(cmd->_outfiles[i]._type == t_output_write)
				outfile=open(cmd->_outfiles[i]._file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
			else			
				outfile=open(cmd->_outfiles[i]._file, O_APPEND| O_CREAT|O_WRONLY, S_IRWXU);
		}
	}
	return outfile;
}

int get_infile(simple_command *cmd) {
	int infile = STDIN_FILENO;
	
	int size_infile = cmd->_no_infiles;
	
	if(size_infile > 0)
		infile = open(cmd->_infiles[size_infile-1],O_RDONLY, S_IRWXU);

	return infile;
}

int exec_command(simple_command* cmd,int infile,int outfile) {
	if (infile != STDIN_FILENO) {
		dup2 (infile, STDIN_FILENO);
		close (infile);
	}
	if (outfile != STDOUT_FILENO) {
		dup2 (outfile, STDOUT_FILENO);
		close (outfile);
	}


	execvp(cmd->_tokens[0], cmd->_tokens);
	perror ("error");
	exit (1);
}

int checkDefaultCommand(simple_command *cmd, int fd) {
	char *tmp = cmd->_tokens[0];
	if(strcmp("cd",tmp) == 0) {
		if(cmd->_tokens[1]==NULL || strcmp("~",cmd->_tokens[1]) == 0)		
			chdir(defaultPath);
		else if (chdir(cmd->_tokens[1]) != 0) 		
  			perror("error");
  		return 1;
	}
	if(strcmp("history", tmp) == 0) {
			print_history(fd);
			return 1;
	}

	if(tmp[0] == '!') {
    	if(tmp[1] == '!') {
        	indexLineToSave = end_register - 1;
            return 1;
        }
        indexLineToSave = getNumberLine(cmd);
        return 1;
    }

	if(strcmp("exit",tmp) == 0)
		return 2;
    return 0;
}

int executeProcess(command *cmd) {
	int i, fdpipe[2], infile = get_infile(&cmd->_simple_commands[0]), outfile;
	pid_t pid;
	
	for(i = 0;i < cmd->_no_simple_commands;i++) {
		if(infile < 0) {
			perror("error");
			return -1;
		}

		outfile=get_outfile(&cmd->_simple_commands[i]);
			
		if(i != cmd->_no_simple_commands - 1) {
			pipe(fdpipe);
				
			if(outfile==STDOUT_FILENO)
				outfile=fdpipe[1];
			else
				close(fdpipe[1]);
		}

		int ans = checkDefaultCommand(&cmd->_simple_commands[i], outfile);
		if(ans == 2)
			return 2;
		if(ans == 1)
			return 1;

		if(!ans) {
			pid=fork();
			int status;
			if(pid == 0)
				exec_command(&cmd->_simple_commands[i], infile, outfile);
		}
								
		if(infile!=get_infile(&cmd->_simple_commands[i]))
			close(infile);
		
		if(outfile!=get_outfile(&cmd->_simple_commands[i]))
			close(outfile);
			
		if(i < cmd->_no_simple_commands - 1)
			infile = get_infile(&cmd->_simple_commands[i + 1]);
			
		if(infile==STDIN_FILENO)
			infile=fdpipe[0];
		else 
			close(fdpipe[0]);
	}
	
	if(!cmd->_background) {
		int status;
		waitpid(pid, &status, 0);
	}
}

int main() {
	char *logname = getenv("LOGNAME");
	char home[] = "/home/";
	defaultPath = strcat(home, logname);
	chdir(defaultPath);
	
	while(1) {
		write_prompt();
		command cmd;
		read_command(&cmd);

		int exit = executeProcess(&cmd);

		if(exit == 2) {
			int status;
			waitpid(-1, &status, 0);
			return 0;
		}
	}

	return 0;
}