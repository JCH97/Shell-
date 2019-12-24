//describe a simple commad and arguments
typedef struct
{
	char *_file;
	int _type;
} output_file;

typedef struct
{ 		
	char ** _tokens; //array of arguments
	int _no_tokens; //number of arguments
	output_file *_outfiles;
	int _no_infiles;
	char **_infiles;
	int _no_outfiles;
} simple_command;


typedef struct
{
	simple_command *_simple_commands; //list of simple commands
	int _no_simple_commands; //number of simple commands
	int _background; 	//the process would be in background

} command;