#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/type.h"


#define t_input 1
#define t_output_write 2
#define t_output_append 3
#define t_command 4

char *remainLine;

//count how many times a chararacter c divide a string str
int counter(char *line,char c) {
 int ans = 1;

    //cout the the number of characters equals to c
    int i = 0;
	for (i = 0; i < strlen(line) ; ++i)
	{
		if (line[i] == c)
			++ans;

		//not count consecutive character equals to c
		while(line[i] == c)
			++i;

		//not count lasts character equals to c
		if(line[i] == 0)
		{
			--ans;
			--i;
		}
	}
	return ans;
}


char* getToken(char *str,int *type) {
	
	char *token;
	*type = t_command;
	
	if(str != NULL)
		remainLine = str;
	
	while(*remainLine == ' ')
		++remainLine;

	if(*remainLine == 0)
	{
		remainLine = NULL;
		*type = 0;
		return NULL;
	}
	
	switch (*remainLine)
	{
		case '<':
		{
			*type = t_input;
			++remainLine;
			break;
		}
		case '>':
		{ 
			*type=t_output_write;
			if(*(remainLine + 1) == '>')
			{
				*type = t_output_append;
				++remainLine;
			}
			++remainLine;
			break;
		}
	}	
	
	while(*remainLine == ' ')
		++remainLine;

	char *current = remainLine;
	
    while(*current != ' ' && *current != '<' && *current != '>' && *current != 0 )
		++current;

	int token_size = current - remainLine;
	token=(char*)malloc((token_size)*(sizeof(char)));
	strncpy(token,remainLine,token_size);
	token[token_size]=0;
	remainLine = current;
	return token;
}

int parse_simplecommand(char *str, simple_command *cmd) {
	int i, k;
	char *str_tmp = str, *token;

	cmd->_no_tokens = 0;
	cmd->_no_outfiles = 0;
	cmd->_no_infiles = 0;
	cmd->_outfiles = 0; 
	cmd->_infiles = 0;
	cmd->_tokens = 0;

	remainLine = NULL;
	token = getToken(str_tmp,&k);
	while(token!=NULL)
	{
		switch (k)
		{
			case t_input:
			{
				++cmd->_no_infiles;
				break;
			}
			case t_output_append:
			case t_output_write:
			{
				++cmd->_no_outfiles;
				break;
			}
			case t_command:
			{
				++cmd->_no_tokens;
				break;
			}
		}		
		free(token);
		token = getToken(NULL,&k);
	}

	cmd->_outfiles = (output_file*)malloc(cmd->_no_outfiles*sizeof(output_file));
	cmd->_infiles = (char**)malloc(cmd->_no_infiles*sizeof(char*));
	cmd->_tokens = (char**)malloc(cmd->_no_tokens*sizeof(char*));
	
	for (i = 0; i < cmd->_no_tokens; ++i) cmd->_tokens[i] = NULL;
	for (i = 0; i < cmd->_no_infiles; ++i) cmd->_infiles[i] = NULL;
	for (i = 0; i < cmd->_no_outfiles; ++i)
	{
		cmd->_outfiles[i]._file=NULL;
		cmd->_outfiles[i]._type=0;
	}
	
	remainLine = NULL;
	token=getToken(str,&k);
	int it_out = 0, it_inf = 0, it_tokens = 0;
	while(token != NULL)
	{
		switch (k)
		{
			case t_input:
			{	
				cmd->_infiles[it_inf] = token;
				++it_inf;
				break;		
			}
			case t_output_append:
			{	
			    cmd->_outfiles[it_out]._file = token;
				cmd->_outfiles[it_out]._type = t_output_append;
				++it_out;
				break;
			}
			case t_output_write:
			{
				cmd->_outfiles[it_out]._file = token;
				cmd->_outfiles[it_out]._type = t_output_write;
				++it_out;
				break;
			}
			case t_command:
			{	
				cmd->_tokens[it_tokens] = token;
				++it_tokens;
				break;
			}
		}
			
		token = getToken(NULL, &k);

	}
	cmd->_infiles[cmd->_no_infiles] = NULL;
	cmd->_outfiles[cmd->_no_outfiles]._file = NULL;
	cmd->_outfiles[cmd->_no_outfiles]._type = 0;
	cmd->_tokens[cmd->_no_tokens] = NULL;

	return 0;
}


int parse_command(char *str, int size, command *cmd) {
	char *saveptr, *token;
	int i;

	cmd->_background = 0;
	
	size -= 2;
		
	while(str[size] == ' ')	
		--size;

	if(str[size] == '&')
		cmd->_background = 1;
	else
		++size;	

	str[size] = 0;

	cmd->_no_simple_commands = counter(str, '|');
	
	if(cmd->_no_simple_commands > 0)
	{
		cmd->_simple_commands = (simple_command*)malloc( cmd->_no_simple_commands * sizeof(simple_command));

		token = strtok_r(str, "|", &saveptr);
		
		for (i = 0; token!= NULL; ++i)
		{   
			parse_simplecommand(token,&cmd->_simple_commands[i]);
			token = strtok_r(NULL, "|", &saveptr);
		}
	}

	return 0;
}
