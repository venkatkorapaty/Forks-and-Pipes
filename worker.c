#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include "freq_list.h"
#include "worker.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* The function get_word should be added to this file */
FreqRecord* get_word(char *word, char **filenames, Node *node) {
    Node* tempNode = node;
    FreqRecord *result;
    FreqRecord frqRec[MAXFILES];
    // Finds the node with the inputted word
    while((tempNode != NULL) && !(strcmp(tempNode->word, word) == 0)) {
        tempNode = tempNode->next;
    }
    // if the word isn't in the list
    if(tempNode == NULL) {
	// Return only 1 freqrecord with a freq of 0
	FreqRecord temp;
        temp.freq = 0;
        frqRec[0] = temp;
        result = frqRec;
        return result;
    }
    int i;
    int arrSize = 0;
    // finds the amount of files
    while(filenames[arrSize] != NULL) {
	arrSize = arrSize + 1;
    }
    int j = 0;
    // loops through the array
    for(i = 0; i < arrSize; i++) {
	// adds to the returning array if freq is more than 0
	if(tempNode->freq[i] > 0) {
            FreqRecord temp;
            strcpy(temp.filename, filenames[i]);
            temp.freq = tempNode->freq[i];
            frqRec[j] = temp;
	    j++;
	}
    }
    FreqRecord temp;
    temp.freq = 0;
    frqRec[j] = temp;
    result = frqRec;
    return result;
}


/* Print to standard output the frequency records for a word.
* Used for testing.
*/
void print_freq_records(FreqRecord *frp) {
	int i = 0;
	while(frp != NULL && frp[i].freq != 0) {
		printf("%d    %s\n", frp[i].freq, frp[i].filename);
		i++;
	}
}

/* run_worker
* - load the index found in dirname
* - read a word from the file descriptor "in"
* - find the word in the index list
* - write the frequency records to the file descriptor "out"
*/
void run_worker(char *dirname, int in, int out){
    // concatentates the appropriate file names to path
    char* indexFile = malloc(strlen(dirname) + strlen("/index") + 1);
    char* namefile = malloc(strlen(dirname) + strlen("/filenames") + 1);
    Node* head = NULL;    
    char** filenames = init_filenames();
    strcpy(indexFile, dirname);
    strcat(indexFile, "/index");
    strcpy(namefile, dirname);
    strcat(namefile, "/filenames");
    read_list(indexFile, namefile, &head, filenames);
    // gets the word input
    char* buff;
    buff = malloc(MAXWORD);
    int err;
    err = read(in, buff, MAXWORD);
    if(err < 0) {
	perror("read\n");
	exit(1);
    }
    //gets the freqrec array with the words
    FreqRecord* freqRec = get_word(buff, filenames, head);
    int i = 0;
    // writes out all the freqrecs
    while(freqRec != NULL && freqRec[i].freq != 0) {
	err = write(out, &freqRec[i], sizeof(freqRec[i]));
	if(err < 0) {
		perror("write\n");
		exit(1);
	}
	i++;
    }
    // writes out the last freq 0 freqrecord
    err = write(out, &freqRec[i], sizeof(freqRec[i]));
    if(err < 0) {
	perror("write\n");
	exit(1);
    }
    free(buff);
}

