/*
 * File:   main.c
 * Author: Chris MacPherson
 * 
 * This programme will modify the PHP xdebug remote_host to the current machine IP.
 * It's also me trying to learn some C so probably could be done in a million better ways.
 *
 * Created on 21 June 2012, 20:03
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

/*
 *
 */
int main(int argc, char** argv, char *envp[])
{
    // Try and work out IP of SSH client
    char *clientIp, *xdebugIni, *sshClient, *sshConnection;

    // Make sure xdebug.ini file path has been passed
    if (argc < 2) {
        printf("Usage: xdebug-komfig <path-to-ini-file-containing-xdebug-settings>\n");
        return 1;
    }

    xdebugIni = argv[1];

    sshClient = getenv("SSH_CLIENT");
    sshConnection = getenv("SSH_CONNECTION");

    // Get client IP from SSH Session
    if (sshClient != NULL) {
        clientIp = strtok(sshClient, " ");
    } else if (sshConnection != NULL) {
        clientIp = strtok(sshConnection, " ");
    }

    // Set root permission
    int current_uid = getuid();

    // Try and elevate privileges so we can edit root owned PHP ini files
    if (setuid(0))
    {
        printf("Setup error: ensure the xdebug-komfig binary perms are: -rws--x--x 1 root root\n");
        return 1;
    }


    // Open file to check if xdebug.remote_host seeting exists
    FILE *fp;
    fp = fopen(xdebugIni, "r");

    if (fp == NULL) {
        printf("Couldn't open %s\n", xdebugIni);
        return 1;
    }

    // Search through ini file for remote_host setting
    char line[512];
    bool match = false;

    while (fgets(line, 512, fp) != NULL) {
        if(strstr(line, "xdebug.remote_host") != NULL && strstr(line, clientIp) == NULL) {
            match = true;
        }
    }

    // Tell user that we are attempting to update settings.
    // We don't want to tell them every SSH login, only when an update is needed
    if (match) {
        printf("Configuring X-Debug for your IP: %s\n", clientIp);
    } else {
        return 1;
    }

    // Found match so create update file by creating a temp and then renaming
    rewind(fp);

    FILE *fpTemp;
    fpTemp = fopen("/tmp/xdebug-komfig.tmp", "w");

    while ((fgets(line, 512, fp)) != NULL) {
        if (strstr(line, "xdebug.remote_host") != NULL) {
            fprintf(fpTemp, "xdebug.remote_host = %s\n", clientIp);
        } else {
            fprintf(fpTemp, line);
        }
    }

    // Close files
    if (fp != NULL) {
        fclose(fp);
    }

    if (fpTemp != NULL) {
        fclose(fpTemp);
    }

    // Overwrite the original file with the temp file
    rename("/tmp/xdebug-komfig.tmp", xdebugIni);

    // Try and restart apache to pick up the new settings
    system("/etc/init.d/httpd-php5 restart");

    //Time to drop back to regular user privileges
    setuid(current_uid);

    return 0;
}

