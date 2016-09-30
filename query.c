#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "freq_list.h"
#include "worker.h"
#include <sys/wait.h>

FreqRecord* bubblesort(int size, FreqRecord* freqRec) {
	int i;
	int j;
	FreqRecord temp;
	for(i=0; i < (size-1); i++) {
		for(j=0; j < (size-1); j++) {
			if(freqRec[j].freq < freqRec[j+1].freq) {
				temp = freqRec[j];
				freqRec[j] = freqRec[j+1];
				freqRec[j+1] = temp;
			}
		}
	}
	return freqRec;
}


int main(int argc, char **argv) {
	
	char ch;
	char path[PATHLENGTH];
	char *startdir = ".";

	while((ch = getopt(argc, argv, "d:")) != -1) {
		switch (ch) {
			case 'd':
			startdir = optarg;
			break;
			default:
			fprintf(stderr, "Usage: queryone [-d DIRECTORY_NAME]\n");
			exit(1);
		}
	}
	// Open the directory provided by the user (or current working directory)
	
	DIR *dirp;
	if((dirp = opendir(startdir)) == NULL) {
		perror("opendir");
		exit(1);
	} 
	//char* originDir = startdir;
	/* For each entry in the directory, eliminate . and .., and check
	* to make sure that the entry is a directory, then call run_worker
	* to process the index file contained in the directory.
 	* Note that this implementation of the query engine iterates
	* sequentially through the directories, and will expect to read
	* a word from standard input for each index it checks.
	*/
		
	int i = 0;
	struct dirent *dp;
	while((dp = readdir(dirp)) != NULL) {
		if(strcmp(dp->d_name, ".") == 0 || 
		   strcmp(dp->d_name, "..") == 0 ||
		   strcmp(dp->d_name, ".svn") == 0){
			continue;
		}
		strncpy(path, startdir, PATHLENGTH);
		strncat(path, "/", PATHLENGTH - strlen(path) - 1);
		strncat(path, dp->d_name, PATHLENGTH - strlen(path) - 1);

		struct stat sbuf;
		if(stat(path, &sbuf) == -1) {
			//This should only fail if we got the path wrong
			// or we don't have permissions on this entry.
			perror("stat");
			exit(1);
		}
		// Only call run_worker if it is a directory
		// Otherwise ignore it.
		if(S_ISDIR(sbuf.st_mode)) {
			// Gets me the amount of directories
			i++;
		}	
	}
	// gets input from user
	char* buff;
	buff = malloc(MAXWORD);
	printf("Please enter a word to search. Enter EXIT to stop.\n");
	read(STDIN_FILENO, buff, MAXWORD-1);
	buff[MAXWORD] = '\0';
	char tempStr[5];
	strncpy(tempStr, buff, 4);
	tempStr[5] = '\0';
	// gets the word without the \n
	char* wordOnly;
	wordOnly = strtok(buff, "\n");
	// While the input isn't EXIT
	while(strcmp(wordOnly, "EXIT") != 0) {
		printf("Currently searching for %s..\n", buff);
		char path1[PATHLENGTH];
		dirp = opendir(startdir);
		if(dirp == NULL) {
			perror("opendir");
			exit(1);
		}
		// creating the pipes
		int mypipe[i][2];
		int z;
		for(z = 0; z < i; z++) {
			int ret = pipe(mypipe[z]);
			if(ret < 0 ) {
				perror("pipe\n");
				exit(1);
			}
		}

		pid_t pids[i];
		int total = i;
		i = 0;
		int err;
		while((dp = readdir(dirp)) != NULL) {
			if(strcmp(dp->d_name, ".") == 0 ||
		   	strcmp(dp->d_name, "..") == 0 ||
		   	strcmp(dp->d_name, ".svn") ==0) {
				continue;
			}
			strncpy(path1, startdir, PATHLENGTH);
			strncat(path1, "/", PATHLENGTH - strlen(path) - 1);
			strncat(path1, dp->d_name, PATHLENGTH - strlen(path) - 1);

			struct stat sbuf;
			if(stat(path1, &sbuf) == -1) {
				perror("stat");
				exit(1);
			}
			if(S_ISDIR(sbuf.st_mode)) {
				// Create a child process if it's a dir
				if((pids[i]=fork()) < 0) {
					perror("fork");
					exit(1);
				}
				else if(pids[i] == 0) {
					// If child, write inputted word into
					// and run the worker for that child/dir
					err = write(mypipe[i][1], wordOnly, sizeof(wordOnly));
					if(err < 0) {
						perror("write\n");
						exit(1);
					}
					run_worker(path1, mypipe[i][0], mypipe[i][1]);
					exit(0);
				}
				i++;
			}
		}
		int status;
		int tempTot = total;
		// wait for all the children to finish
		while(tempTot > 0) {
			 wait(&status);
			--tempTot;
		}
		FreqRecord masterFreq[MAXRECORDS];
		// keeps track of total records
		int x = 0;
		// keeps track of which pipe
		z = 0;
		// loop through all the pipes
		while(z < total) {
			FreqRecord* tempFreq = malloc(sizeof(FreqRecord));
			err = read(mypipe[z][0], tempFreq, sizeof(FreqRecord));
			if(err < 0) {
				perror("read\n");
				exit(1);
			}
			// read all the values in the current pipe
			while(tempFreq->freq != 0) {
				masterFreq[x] = *tempFreq;
				x++;
				err = read(mypipe[z][0], tempFreq, sizeof(FreqRecord));
				if(err < 0) {
					perror("read\n");
					exit(1);
				}
			}
			z++;
		}
		// sort the array
		bubblesort(x, masterFreq);
		int recs;
		//print out all the  found freqrecords
		for(recs = 0; recs < x; recs++) {
			printf("%s, %d\n", masterFreq[recs].filename, masterFreq[recs].freq);
		}
		if(x == 0)
			printf("No results\n");
		// Free the old word and get the new one
		free(buff);
		char* buff;
		buff = malloc(MAXWORD);
		printf("Please enter a word to search.Enter EXIT to stop.\n");
		read(STDIN_FILENO, buff, MAXWORD-1);
		buff[MAXWORD] = '\0';
		char* wordOnly;
		wordOnly = strtok(buff, "\n");
		if(wordOnly == NULL) {
			perror("strtok");
			exit(1);
		}
	}
	free(buff);
	return 0;
}
