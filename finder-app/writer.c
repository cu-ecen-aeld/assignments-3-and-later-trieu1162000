#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>

int main(int argc, char *argv[]) {
    openlog("writer", LOG_PID | LOG_CONS, LOG_USER);

    // Check if both arguments are provided
    if (argc != 3) {
        syslog(LOG_ERR, "Usage: %s <writefile> <writestr>", argv[0]);
        closelog();
        return 1;
    }

    // Extract arguments
    const char *writefile = argv[1];
    const char *content = argv[2];

    char writestr[strlen(content) + 2];
    sprintf(writestr, "%s\n", content);

    // Open the file for writing
    FILE *file = fopen(writefile, "w");
    if (file == NULL) {
        syslog(LOG_ERR, "File '%s' does not exist. Generating new file instead.", writefile);
        closelog();
        return 1;
    }

    // Write the text string to the file
    if (fputs(writestr, file) == EOF) {
        syslog(LOG_ERR, "Error: Could not write to file '%s'", writefile);
        fclose(file);
        closelog();
        return 1;
    }

    // Write successfully
    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);

    // Close the file
    fclose(file);

    closelog();
    return 0; // Success
}
