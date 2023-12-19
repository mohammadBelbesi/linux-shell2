program name: shell2
student name: mohammad belbesi

==Description:==
In this exercise "shell2.c" we implemented a simple C language shell under the linux operating system.
The shell will show the user a prompt that will read the commands and send them to the operating system for execution.
After running our shell program (which will be called ex2) you will see the following prompt for the user: #cmd:<commandnums>|args:<argsNum>@<diractory>> directory: will be replaced with the current directory name
The user will be able to type commands including arguments, and the program will execute the command entered by the user.
Requirements from the casing will be as follows:
1. Input of a command from the user
2. Run the command
3. Execution of the cd command
4. The envelope will exit when the user press "enter" 3 times.
5. we can enter many commands by using ';' between them and our shell will run all the commands one after another
6. we can save enviroment variable and use them in the commands by using $
7. our shell also can run commands that including pipes "|" , redirections ">" , and "&" and also support the ctrl-z and bg


==functions==
three main functions:
1-stringParser function
2-generalCommand function = Command
3-main function


other functions:
int sentenceType(char *s);
void stringParser(char* s, int* wordNum, int* charNum);
void command(char *s,char **commandL,int *argsNum);
void removeSpaces(char* str);
int countAndRemoveQuotes(char* str);
char* get_env_var(char* name);
void set_env_var(char* name, char* value);
void parseString(char* input, char** name, char** value);
void deleteFirstDollar(char* str);
int echoQuotationChecker(char *s);
void addSpacesToSemiColon(char* str);
int semiColonChecker(char *s);
int count_words_without_semicolon(const char* str);

==programe files:==
one file: shell2.c

==How to compile?==
-the first one shell2.c
compile: gcc shell2.c -o shell2
run: ./shell2

==Input:==
command or 3 times enter or cd

==output:==
*Execution of the linux commands that we enter
*when we press enter 3 times the program will exit: