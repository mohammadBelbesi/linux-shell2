//mohammad belbesi

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <wait.h>
#include <signal.h>

#define SENTENCE_LENGTH 511 //sentence length is 511 with \0 that's mean the sentence length is 510
#define MAX_ARGS 10
#define MAX_PROCESS 10
#define REGULAR 0
#define DOLLAR 1
#define EQUAL 2
#define EMPTY 3
#define CD 4
#define ECHO 5
#define QUOTATION_MARKS 6
#define SEMI_COLON 7
#define PIPE_APPEARS 8
#define REDIRECTION_APPEARS 9
#define AND_APPEARS 10
#define BG_APPEARS 11

int sentenceType(char * s);
void stringParser(char * s, int * wordNum, int * charNum);
void command(char * s, char ** commandL, int * argsNum);
void removeSpaces(char * str);
int countAndRemoveQuotes(char * str);
char * get_env_var(char * name);
void set_env_var(char * name, char * value);
void parseString(char * input, char ** name, char ** value);
void deleteFirstDollar(char * str);
int echoQuotationChecker(char * s);
void addSpacesToSemiColon(char * str);
int semiColonChecker(char * s);
int count_words_without_semicolon(const char * str);
int pipesChecker(char * s);
int redirectionChecker(char * s);
int andChecker(char * s);
int bgChecker(char * s);
void replaceDollar(char * sentence);
void parsePipes(char * sentence);
void pipeline(char ** * cmd);
int countCommandsBetweenPipes(char ** * cmd);

int quotationFlag = 0;
int andFlag = 0;
int num_env_vars = 0;
pid_t theRunning_pid = 0;
pid_t thePaused_pid[MAX_PROCESS];
int num_paused_pid = 0;
int commandsNum = 0, argsNum = 0, wordNum = 0, charNum = 0;
typedef struct {
    char name[SENTENCE_LENGTH];
    char value[SENTENCE_LENGTH];
}
        env_var_t;
env_var_t env_vars[SENTENCE_LENGTH];

void handle_sigint() {
    if (theRunning_pid != 0) {
        kill(theRunning_pid, SIGINT);
        theRunning_pid = 0;
    }
}

void handle_sigtstp() {
    if (theRunning_pid != 0) {
        thePaused_pid[num_paused_pid] = theRunning_pid;
        num_paused_pid++;
        kill(theRunning_pid, SIGSTOP);

        printf("\n[%d]+ Stopped\n", num_paused_pid);
        theRunning_pid = 0;
    }
}

void handle_bg() {
    if (num_paused_pid > 0) {
        pid_t pid = thePaused_pid[num_paused_pid - 1];
        num_paused_pid--;
        kill(pid, SIGCONT);
        printf("[%d]+ %d\n", num_paused_pid + 1, pid);
    } else {
        printf("No paused processes\n");
    }
}

int main() {
    signal(SIGINT, handle_sigint); // Set up signal handler for ctrl-c
    signal(SIGTSTP, handle_sigtstp); // Set up signal handler for ctrl-z

    char sentence[SENTENCE_LENGTH] = {}, cwd[SENTENCE_LENGTH] = {}, ** commandLine = {
            NULL
    }, ** cmd_args = {
            NULL
    }, * name = {
            NULL
    }, * value = {
            NULL
    };
    memset(sentence, 0, SENTENCE_LENGTH); //clear the char array and fill it with zero's
    memset(cwd, 0, SENTENCE_LENGTH); //clear the char array and fill it with zero's
    /*
    memset(name,0,SENTENCE_LENGTH);//clear the char array and fill it with zero's
    memset(value,0,SENTENCE_LENGTH);//clear the char array and fill it with zero's
    memset(commandLine,0,SENTENCE_LENGTH);//clear the char array and fill it with zero's
    memset(cmd_args,0,SENTENCE_LENGTH);//clear the char array and fill it with zero's
    */
    int enterCounter = 0, status = 0;
    getcwd(cwd, sizeof(cwd));
    while (1) { //the shell loop
        if (enterCounter == 3) {
            break;
        }
        printf("#cmd:%d|args:%d@%s>", commandsNum, argsNum, cwd); //the prompt
        if (fgets(sentence, SENTENCE_LENGTH, stdin) != NULL) { //to scan the command that we enter
            sentence[strlen(sentence) - 1] = '\0'; //solve the problem of \n and replace it with \0
            if (sentenceType(sentence) == EMPTY) {
                enterCounter++;
            } else if (sentenceType(sentence) == CD) {
                printf("cd not supported\n");
            } else if (sentenceType(sentence) == EQUAL) {
                parseString(sentence, & name, & value);
                set_env_var(name, value);
            } else if (bgChecker(sentence) == BG_APPEARS) {
                handle_bg();
                argsNum++;
                commandsNum++;
            } else { //if it's a command
                if (andChecker(sentence) == AND_APPEARS) {
                    andFlag = 1;
                    sentence[strlen(sentence) - 1] = '\0';
                }
                if (echoQuotationChecker(sentence) == QUOTATION_MARKS) { //check if the command contain echo and quotation and deal with it
                    countAndRemoveQuotes(sentence);
                    quotationFlag = 1;
                    wordNum += 2; //to deal with num world in the case that we have quotation
                }
                if (sentenceType(sentence) == DOLLAR) { //check if the command contain dollar and deal with it
                    replaceDollar(sentence);
                }
                if (quotationFlag == 0) {
                    stringParser(sentence, & wordNum, & charNum); //our main func for countCommandsBetweenPipes words and chars
                }
                if (semiColonChecker(sentence) == SEMI_COLON && sentenceType(sentence) == DOLLAR) {
                    argsNum += count_words_without_semicolon(sentence);
                }
                //wordNumInAllCommands+=wordNum;

                commandLine = (char ** ) malloc(sizeof(char * ) * (MAX_ARGS + 1)); //the array of the pointers on the words array
                if (commandLine == NULL) {
                    fprintf(stderr, "malloc failed!\n");
                    exit(EXIT_FAILURE);
                }
                commandLine[wordNum] = NULL;

                if (semiColonChecker(sentence) == SEMI_COLON) {
                    addSpacesToSemiColon(sentence);
                }
                int i = 0;
                char copySentenceForColumn[SENTENCE_LENGTH] = {};
                memset(copySentenceForColumn, '\0', SENTENCE_LENGTH);
                strcpy(copySentenceForColumn, sentence);
                commandLine[i] = strtok(copySentenceForColumn, ";"); //toke every command that occur between ; separately
                while (commandLine[i] != NULL) { //loop to split the command
                    i++;
                    commandLine[i] = strtok(NULL, ";");
                }
                commandLine[i] = NULL;

                //printf("ok %s\n",commandLine[1]);
                if (pipesChecker(sentence) == PIPE_APPEARS) {
                    int s = 0;
                    while (commandLine[s] != NULL) {
                        //printf("...%s...\n",commandLine[s]);
                        parsePipes(commandLine[s]);
                        s++;
                    }
                } else {
                    for (int j = 0; j < i; j++) { //main loop to run the commands
                        pid_t pid;
                        pid = fork();
                        if (pid < 0) { //fork failed
                            perror("fork failed!\n");
                            exit(EXIT_FAILURE);
                        }
                        if (pid == 0) { //child process
                            //command(sentence, commandLine);
                            cmd_args = (char ** ) malloc(MAX_ARGS * sizeof(char * ));
                            command(commandLine[j], cmd_args, & argsNum);
                        }

                        if (pid > 0) { //parent process
                            //wait(NULL);
                            //waitpid(pid, &status, 0);
                            theRunning_pid = pid;
                            if (!andFlag) {
                                waitpid(pid, NULL, WUNTRACED); // Wait for child to finish or be stopped
                                theRunning_pid = 0;
                            } else {
                                printf("[%d] %s &\n", pid, sentence); // Print background process information
                                andFlag = 0;
                            }
                            if (WIFEXITED(status)) { //to check if the command is legal then modify the command and the args number
                                commandsNum++;
                                argsNum += wordNum;
                            }
                            wordNum = 0, charNum = 0; //initialize the variable for the next iterations

                        }

                    }
                }
            }

        }

    }

    return 0;
}

int countCommandsBetweenPipes(char ** * cmd) {
    int count = 0;
    for (int i = 0; cmd[i] != NULL; i++) {
        count++;
    }
    commandsNum+=count;
    return count;

}

void pipeline(char ** * cmd) {
    int i, j = 0;
    pid_t pid;
    int cmd_len = countCommandsBetweenPipes(cmd);
    int fd[2 * cmd_len];

    i = 0;
    while (i < 2 * cmd_len) {
        if (pipe(fd + i) < 0) {
            perror("pipe error!\n");
            exit(EXIT_FAILURE);
        }
        i += 2;
    }

    for (;* cmd != NULL; cmd++, j += 2) {
        if ((pid = fork()) < 0) {
            perror("fork failed\n");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // if there is next
            if ( * (cmd + 1) != NULL) {
                close(fd[j]);
                if (dup2(fd[j + 1], STDOUT_FILENO) < 0) {
                    perror("dup2 error\n");
                    exit(EXIT_FAILURE);
                }
                close(fd[j + 1]);
            }

            if (j != 0) {
                close(fd[j + 1]);
                if (dup2(fd[j - 2], STDIN_FILENO) < 0) {
                    perror("dup2 error\n");
                    exit(EXIT_FAILURE);
                }
                close(fd[j - 2]);
            }
            // close all the other fds that I don't use
            i = 0;
            while (i < 2 * cmd_len) {
                if (i == j || i == j + 1) {
                    i++;
                    continue;
                }
                close(fd[i]);
                i++;
            }

            if (execvp(( * cmd)[0], * cmd) < 0) {
                perror(( * cmd)[0]);
                exit(EXIT_FAILURE);
            }
        } else if (pid < 0) {
            perror("fork error!\n");
            exit(EXIT_FAILURE);
        }
    }

    // close fds in parent process
    i = 0;
    while (i < 2 * cmd_len) {
        close(fd[i]);
        i++;
    }

    // wait for the children
    i = 0;
    while (i < cmd_len) {
        wait(NULL);
        i++;
    }
}

void parsePipes(char * sentence) {
    removeSpaces(sentence);
    //printf("ok %s\n",sentence);
    wordNum = 0;
    //int charNum = 0;
    char ** commandLine;
    char ** * cmd;
    commandLine = (char ** ) malloc(sizeof(char * ) * (MAX_ARGS + 1));
    if (commandLine == NULL) {
        fprintf(stderr, "malloc failed!\n");
        exit(EXIT_FAILURE);
    }
    commandLine[wordNum] = NULL;
    int commandNumber = 0;
    commandLine[commandNumber] = strtok(sentence, "|");
    while (commandLine[commandNumber] != NULL) {
        commandNumber++;
        commandLine[commandNumber] = strtok(NULL, "|");
    }
    cmd = (char ** * ) malloc(sizeof(char ** ) * commandNumber);
    int l;
    for (l = 0; l < commandNumber; l++) {
        cmd[l] = (char ** ) malloc(sizeof(char * ) * (MAX_ARGS + 1));
        char * word = strtok(commandLine[l], " ");
        int i = 0;
        while (word != NULL) {
            unsigned int wordLength = strlen(word) + 1;
            cmd[l][i] = (char * ) malloc(sizeof(char) * (wordLength + 1));
            if (cmd[l][i] == NULL) {
                fprintf(stderr, "malloc failed!\n");
                exit(EXIT_FAILURE);
            }
            strcpy(cmd[l][i], word);
            i++;
            word = strtok(NULL, " ");
        }
        cmd[l][i] = NULL;
    }
    cmd[l] = NULL;
    pipeline(cmd);
}

void deleteFirstDollar(char * str) {
    char * pos = strchr(str, '$'); // Find the first occurrence of '$' in the string
    if (pos != NULL) {
        memmove(pos, pos + 1, strlen(pos)); // Shift the rest of the string left to overwrite '$'
    }
}

char * get_env_var(char * name) {
    for (int i = 0; i < num_env_vars; i++) {
        if (strcmp(name, env_vars[i].name) == 0) {
            return env_vars[i].value;
        }
    }
    return NULL;
}

void set_env_var(char * name, char * value) {
    for (int i = 0; i < num_env_vars; i++) {
        if (strcmp(name, env_vars[i].name) == 0) {
            strcpy(env_vars[i].value, value);
            return;
        }
    }
    strcpy(env_vars[num_env_vars].name, name);
    strcpy(env_vars[num_env_vars].value, value);
    num_env_vars++;
}

void parseString(char * input, char ** name, char ** value) {
    // Find the position of the '=' character in the input string
    char * equalsPos = strchr(input, '=');
    if (!equalsPos) {
        // '=' not found in input string
        * name = NULL;
        * value = NULL;
        return;
    }

    // Allocate memory for the name and value strings
    * name = (char * ) malloc(sizeof(char) * (equalsPos - input + 1));
    size_t len = strlen(equalsPos + 1);
    * value = (char * ) malloc(sizeof(char) * (int)(len + 1));
    //*value = (char*) malloc(sizeof(char) * (strlen(equalsPos+1) + 1));
    //*value = (char*) malloc(sizeof(char) * (strlen(equalsPos) + 1 + 1));

    // Copy the name and value strings into their respective memory locations
    strncpy( * name, input, equalsPos - input);
    strncpy( * value, equalsPos + 1, strlen(equalsPos + 1));

    // Null-terminate the strings
    ( * name)[equalsPos - input] = '\0';
    ( * value)[strlen(equalsPos + 1)] = '\0';
}

int sentenceType(char * s) { //the func will take the sentence str and return if we had entered command, variable, cd or just press enter
    long unsigned int length = strlen(s) + 1;
    char copySentence[length]; //this string to copy the original string without spaces to solve the spaces or enter case
    memset(copySentence, 0, length);
    strcpy(copySentence, s); //copy func in c
    copySentence[length] = '\0';
    int i, j;
    for (i = 0, j = 0; j < length; j++) { //copy the sentence without spaces "its good for checking if we enter only spaces, or if we press enter in the command line"
        if (s[j] != ' ') {
            copySentence[i] = s[j];
            i++;
        }
    }
    copySentence[i] = '\0';
    char * equal = strchr(s, '=');
    char * dollar = strchr(s, '$');
    char * cd = strstr(s, "cd");

    //for the sentence type flags
    if (dollar) {
        return DOLLAR;
    } else if (equal) {
        return EQUAL;
    } else if (copySentence[0] == '\0') {
        return EMPTY;
    } else if (cd) {
        return CD;
    } else {
        return REGULAR;
    }

}

int echoQuotationChecker(char * s) {
    char * quotationMarks = strchr(s, '"');
    char * echo = strstr(s, "echo");
    if (echo) {
        if (quotationMarks) {
            return QUOTATION_MARKS;
        }
        return ECHO;
    }
    return 0;

}

int semiColonChecker(char * s) {
    char * semiColon = strchr(s, ';');
    if (semiColon) {
        return SEMI_COLON;
    }
    return 0;

}

void stringParser(char * s, int * wordNu, int * charNu) { // return how many words and chars in the sentence that we entered, and it takes the sentence str and two pointers that points on word and char number
    long unsigned int len = strlen(s) + 1; // the sentence length that we entered

    for (int i = 0; i < len;) { // loop to countCommandsBetweenPipes the words and the chars by o(n), only one time we check the char
        if (s[i] == ' ' || s[i] == ';') {
            i++;
        } else {
            ( * wordNu) ++;
            ( * charNu) ++;
            int j;
            for (j = i + 1; j < len; j++) {
                if (s[j] == ' ') {
                    i = j;
                    break;
                } else {
                    ( * charNu) ++;
                }
            }
            if (s[j] == '\0') {
                i = j;
            }
        }
    }

}

void command(char * s, char ** commandL, int * argsNu) {
    unsigned int length = strlen(s) + 1;
    char copyS[length];
    strcpy(copyS, s); // we copy the array because the strtok is not safe, and it makes changes on the original one
    copyS[length] = '\0';

    char * word = strtok(copyS, " ");
    int i = 0;
    while (word != NULL) {
        unsigned int wordLength = strlen(word) + 1;
        commandL[i] = (char * ) malloc(sizeof(char) * (wordLength + 1)); // take every word of the sentence and enter it in the 2d commandL string
        if (commandL[i] == NULL) {
            fprintf(stderr, "malloc failed!\n");
            exit(EXIT_FAILURE); // exit the code
        }
        strcpy(commandL[i], word);
        i++;
        word = strtok(NULL, " ");
    }
    ( * argsNu) += i;
    free(word);
    if (redirectionChecker(s) == REDIRECTION_APPEARS) {
        char * output_file = NULL;
        int j = 0;
        while (commandL[j] != NULL) {
            if (strcmp(commandL[j], ">") == 0) {
                output_file = commandL[j + 1];
                commandL[j] = NULL;
                break;
            }
            j++;
        }
        if (output_file != NULL) {
            FILE * fp = freopen(output_file, "w", stdout);
            if (fp == NULL) {
                perror("freOpen Error\n");
                exit(EXIT_FAILURE);
            }

        }
    }
    if (execvp(commandL[0], commandL) == -1) {
        perror("the execvp() failed, the generalCommand not found! or there are to many commands!");
        sleep(1);
    }
    exit(EXIT_FAILURE);
}

void removeSpaces(char * str) {
    unsigned int len = strlen(str);

    // Remove leading spaces
    while (isspace(str[0])) {
        for (int i = 0; i < len; i++) {
            str[i] = str[i + 1];
        }
        len--;
    }

    // Remove trailing spaces
    while (isspace(str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }

    // Replace multiple spaces with a single space
    int i, j;
    for (i = 0, j = 0; i < len; i++) {
        if (!isspace(str[i]) || (i > 0 && !isspace(str[i - 1]))) {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}

int countAndRemoveQuotes(char * str) {
    int count = 0;
    unsigned int len = strlen(str);

    for (int i = 0; i < len; i++) {
        if (str[i] == '\"') {
            count++;
        } else {
            str[i - count] = str[i];
        }
    }

    str[len - count] = '\0';

    return count;
}

void addSpacesToSemiColon(char * str) {
    unsigned int len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (str[i] == ';') {
            if (i > 0 && str[i - 1] != ' ') {
                memmove(str + i + 1, str + i, len - i + 1);
                str[i] = ' ';
                len++;
            }
            if (i < len - 1 && str[i + 1] != ' ') {
                memmove(str + i + 2, str + i + 1, len - i);
                str[i + 1] = ' ';
                len++;
                i++; // Skip over newly added space
            }
        }
    }
    str[len] = '\0'; // add \0

}

int count_words_without_semicolon(const char * str) {
    char * str_copy = strdup(str); // create a copy of the original string
    int count = 0;
    char * token = strtok(str_copy, " ;"); // strtok function splits string into tokens using delimiters

    while (token != NULL) {
        count++;
        token = strtok(NULL, " ;");
    }

    free(str_copy); // free the memory allocated for the copy of the string
    return count;
}

int pipesChecker(char * s) {
    char * pipeCheck = strchr(s, '|');
    if (pipeCheck) {
        return PIPE_APPEARS;
    }
    return 0;
}

int redirectionChecker(char * s) {
    char * redirectionCheck = strchr(s, '>');
    if (redirectionCheck) {
        return REDIRECTION_APPEARS;
    }
    return 0;
}

int andChecker(char * s) {
    char * andCheck = strchr(s, '&');
    if (andCheck) {
        return AND_APPEARS;
    }
    return 0;
}
int bgChecker(char * s) {
    char * bgCheck = strstr(s, "bg");
    if (bgCheck) {
        return BG_APPEARS;
    }
    return 0;
}

void replaceDollar(char * sentence) {
    int dollarCounter = 1;
    char * ptr = strchr(sentence, '$');

    while (ptr != NULL) {
        char word[SENTENCE_LENGTH];
        ptr++;
        int i = 0;
        while ( * ptr != '\0' && * ptr != ' ' && * ptr != ';' && i < SENTENCE_LENGTH) {
            word[i] = * ptr;
            ptr++;
            i++;
        }
        word[i] = '\0';
        ptr -= i;

        char * val = get_env_var(word);

        if (ptr != NULL) {
            int oldWordLength = i;
            unsigned int newWordLength = strlen(val);

            if (oldWordLength != newWordLength) {
                memmove(ptr + newWordLength, ptr + oldWordLength, strlen(ptr + oldWordLength) + 1);
            }

            memcpy(ptr, val, newWordLength);
            removeSpaces(ptr);
            ptr[strlen(sentence) - 1] = '\0';

            if (sentence[0] == '$') {
                memmove(sentence, sentence + 1, strlen(sentence + 1) + 1);
            }

            ptr = strchr(ptr, '$');
            if (ptr != NULL) {
                dollarCounter++;
            }
        }
    }

    while (dollarCounter != 0) {
        deleteFirstDollar(sentence);
        dollarCounter--;
    }
}
