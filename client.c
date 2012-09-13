#include <stdio.h>
#include <windows.h>
#include <winsock2.h>
//#include <winbase.h>

#include "resource.h"
#include "errors.h"
#include "sntp.h"

#define HOST_NAME_MAX 64

extern int silent;
extern int force_sync;

void usage() {
    printf("\nParameters: sntpclient.exe [-f] [-s] <hostname>\n\t-f\tforce a local time update\n\t\teven if SNTP server isnt trustworthy\n\t-s\tsilent mode (no text on the console)\n");
}

char* resolve (char *host) {
    DWORD dwError;

    struct hostent *remoteHost;
    struct in_addr addr;
    remoteHost = gethostbyname(host);

    if (remoteHost == NULL) {
        dwError = WSAGetLastError();
        if (dwError != 0) {
            if (dwError == WSAHOST_NOT_FOUND) {
                printf("*** ERROR ***: IP address not found.\n");
                return NULL;
            } else if (dwError == WSANO_DATA) {
                printf("*** ERROR ***: IP or address doesn't exist (WSANO_DATA)\n");
                return NULL;
            } else {
                printf("*** ERROR ***: Unable to resolve %s: %ld\n", host, dwError);
                return NULL;
            }
        }
    } else {
        //printf("DNS Name: %s\n", remoteHost->h_name);
        if (remoteHost->h_addrtype == AF_INET) {
            addr.s_addr = *(u_long *) remoteHost->h_addr_list[0];
            return inet_ntoa(addr);
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    char *target;
    target = malloc(sizeof(char) * HOST_NAME_MAX);
    target = memset(target, 0, HOST_NAME_MAX);
    char *ip;
    ip = malloc(sizeof(char) * HOST_NAME_MAX);
    ip = memset(ip, 0, HOST_NAME_MAX);

    log_init();

    int i = 1;
    for (i = 1;i<argc;i++) {
        if (strcmp(argv[i],"-s") == 0) {
            silent = 1;
        }
        else if (strcmp(argv[i],"-f") == 0) {
            force_sync = 1;
            writelog("*** INFO *** There won't be any time/date checks.\nThis means this program will blindly set whatever the server sends back as a reply.\n", 0);
        }
        else if (*argv[i] != '-' && *target == '\0') target = argv[i];
        else if (*target != '\0') {
            printf("*** ERROR ***: Server was already defined on the commandline (too much parameters passed)\n");
            usage();
            exit(1);
        }
    }

    if (*target == '\0') {
        printf("*** ERROR ***: No SNTP server specified on the commandline.\n");
        usage();
        exit(1);
    }
    else {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(1,1), &wsaData) != 0) fail_exit("WSAStartup failed.");

        ip = resolve(target);
        if (ip != NULL) {
            printf("Hostname %s -> IP %s\n", target, ip);
            sntp_connect(ip);
        }
        WSACleanup();
        log_close();
    }
    return 0;
}
