#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "errors.h"

#define LOGFILE "sntpclient.log"
#define BUFLEN 512

int silent = 0;

FILE *pLog;
char logBuffer[BUFLEN];

time_t unixtime;
struct tm* timeinfo;

void wfail(char *WinApiCall, char *file, int line) {
        DWORD err = GetLastError();
        char errmsg[512];
        memset(errmsg, 0, sizeof(char) * 512);

        sprintf(errmsg, "La fonction WinAPI %s a echoue: no d'erreur %d, a %s:%d.\nConsultez \"net helpmsg %d\" pour plus d'informations.\n\n", WinApiCall, (int)err, file, line, (int)err);
        writelog(errmsg, 1);
}

void wfail_exit(char *WinApiCall, char *file, int line) {
    wfail(WinApiCall, file, line);
    exit(1);
}

void fail_exit(char *text) {
    writelog(text, 1);
    exit(1);
}

void echo(char *text) {
    if (silent == 0) printf(text);
}

void log_init() {
    //pLog = fopen(LOGFILE, (append == 1 ? "a+" : "w+")) ;
    pLog = fopen(LOGFILE, "a+") ;
    if (pLog == NULL) {
        printf("*** ERROR *** Opening log file failed!\n");
    }
}

void log_close() {
   if (pLog != NULL && fclose(pLog) != 0) printf("Log file couldn't be opened correctly.");
}

void writelog(char *text, int timestamp) {
    echo(text);
    writetologfile(text, timestamp);
}

void writetologfile(char *text, int timestamp) {
    char strTime[32];
    if (pLog == NULL) return;

    time(&unixtime);
    timeinfo = localtime(&unixtime);

    memset(&logBuffer,0,BUFLEN * sizeof(char));
    memset(&strTime, 0, sizeof(char) * 32);

    if (timestamp != 0) strftime(strTime, 32, "[%Y-%m-%d %H:%M:%S] ",timeinfo);
    sprintf(logBuffer, "%s%s", strTime, text);

    fwrite(logBuffer, sizeof(char), strlen(logBuffer),pLog);
}
