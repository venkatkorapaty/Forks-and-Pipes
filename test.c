#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include "freq_list.h"
#include "worker.h"

int main() {
    Node *head = NULL;
    char **filenames = init_filenames();
    char *listfile = "index";
    char *namefile = "filenames";
    read_list(listfile, namefile, &head, filenames);
    FreqRecord *x = get_word("your", filenames, head);
    print_freq_records(x);
    return 0;
}
