#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>


int LoadFile(char* buf, int bufsize, const char* szProgram, const char* argv[] )
{
	int p[2], pid;
	if (pipe(p) == -1) {
		perror("pipe");
		return 0;
	}
	
	switch ( pid = fork() ) {
		case -1:
			perror("fork");
			return 0;
		case 0:
			printf("C: %s", szProgram);
			
			usleep(5000);

			close(1);
			dup(p[1]);
			close(p[0]);
		
			execvp(szProgram,(char**)argv);
			
			close(p[1]);
			perror("exec");
			exit(1);
			return 666;
		
		default:
			close(p[1]);
			puts("A");
			while (wait(&pid) < 0);
			puts("B");
			int l=1,r=0;

			l = read(p[0], buf, bufsize);
			while (l>0) {
				buf += l;		
				r += l;
				l = read(p[0], buf, bufsize);
			}
			
			close(p[0]);

			return r;
	}
}



/*!
	\brief	Get result of file execution
	\param	aBuf		outputbuffer
	\param	iBufsize	buffersize
	\param	szArgs		executable with all parameters
	\todo	REFACTOR ME
*/
int LoadFile(char* aBuf, int iBufsize, const char* szArgs)
{
	if (!szArgs)
		return 1;
		
	printf("Executing '%s'\n", szArgs);

#define MAX_ARGS	20

	const char* argv[MAX_ARGS];
	int argc = 0;
	
	for (int i=0; i<MAX_ARGS; i++)
		argv[i] = 0;


	int arglen = strlen(szArgs);

	char *buf = strdup(szArgs);
	char *b = buf;
	
	
	while (argc<MAX_ARGS && *b) {
		//printf(". %d -- '%s'\n", argc, b);
		argv[argc] = b;
		
		//printf("prewhile");
		while (*b) {
			//printf(".\n");
			char ch = *b;
			//count = b - buf;
			//printf("%d: '%c'\n", count, ch);
			
			if (ch=='\\') {
				printf("escape!");
				b++;
				if (!*b)
					break;
				
				b++;
				continue;
			}
			else if (ch==' ' || ch=='\t') {
				break;
			}
			
			b++;
		} 

		if (*b) {
			*b = 0;
			b++;
		}
		
		argc++;
	}
	
	//printf("!!!end\n");
	
	if (argc>=MAX_ARGS) {
		printf("Error: too many arguments!\n");
		free(buf);
		return 3;
	}	
	
	argv[argc] = NULL;

	/*
	for (int i=0; i<argc; i++) {
		if (argv[i])
			printf("Arg #%d = %s\n", i, argv[i]);
		else
			printf("Arg #%d = (NULL)\n", i);
	}
	*/
	
	int ret = LoadFile( aBuf, iBufsize, argv[0], argv );

	free(buf);
	return ret;
}

int
LoadFortune(char* buf, int bufsize)
{
	const char* argv[2];
	argv[0] = "fortune";
	argv[1] = NULL;
	
	return LoadFile( buf, bufsize, "fortune", argv );
}
