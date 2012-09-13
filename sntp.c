#include <winsock2.h>
#include <stdio.h>

#include "errors.h"
#include "sntp.h"

#define NTP2FILETIME 9435484800ULL
#define NTP_TIMEOUT 10

int force_sync = 0;
extern int silent;

SYSTEMTIME SecsToSystemTime(unsigned long timestamp, int convertToLocalTimeZone) {
    /*
    This functions handles the seconds part of the NTP reply into a SYSTEMTIME
    struct, and optionally converts it into an UTC timezone format
    */
    unsigned long long utcTimestamp = 0;
    SYSTEMTIME tSystemTime;
    FILETIME localtime;
    memset(&tSystemTime, 0, sizeof(SYSTEMTIME));

    //Windows starts its timestamp era on Jan 1st 1601
    //NTP starts its timestamp era on Jan 1st 1900
    //This will add the right time interval to conform to the WinAPI.
    utcTimestamp = timestamp + NTP2FILETIME; //<--see header.

    //Conversion into a FILETIME struct
    //source: http://support.microsoft.com/kb/188768
    //ERMAHGERD, a Microsoft KB article which makes sense!
    utcTimestamp *= 10000000;

    //Re-conversion into SYSTEMTIME...
    //...to permit UTC->Local Time conversion
    if (convertToLocalTimeZone == 1) {
        if (FileTimeToLocalFileTime((FILETIME*)&utcTimestamp, &localtime) == 0) {
            wfail("FileTimeToLocalFileTime", __FILE__, __LINE__);
            return tSystemTime;
        }
    }
    else {
        //Ugly Hack (C) but functionnal to bypass type checking:
        memcpy(&localtime, &utcTimestamp, sizeof(FILETIME));
        //the following commented line (and variants of it) didn't pass compilation:
        //localtime = (FILETIME)*utcTimestamp;
    }


    //...aaand reconverting into SYSTEMTIME in non-UTC local time.
    if (FileTimeToSystemTime(&localtime, &tSystemTime) == 0) {
        wfail("FileTimeToSystemTime", __FILE__, __LINE__);
        return tSystemTime;
    }

    return tSystemTime; //aaaand it's gone.
}
SYSTEMTIME NTPPacketToSystemTime(unsigned long secs, unsigned long msecs, int convertToLocalTimeZone) {
    /*
    Converts the raw packet value into a WinAPI SYSTEMTIME struct value.

    Params secs and msecs are already converted into little-endian byte order
    prior calling this function.

    convertToLocalTimeZone specifies whether we should convert time into LOCAL,
    non UTC time or not. 1=yes 0=non
    */
    float msecsFloat;
    unsigned int millisecs, overflowsecs;
    SYSTEMTIME systemTime;

    //SNTP RFC says raw ms values doesnt really represent the actual ms values,
    //and that we should divide that value by 4294967295 to get the real value.
    msecsFloat = (float)msecs / (float)4294967295.0f;

    //On se conforme ici aux exigence de Windows pour l'alignement des secondes.
    //This line conforms Windows' second value alingment.
    msecsFloat *=  1000;

    //The ms value could result into something > 1000, so we're adding the
    //overflow BEFORE converting into SYSTEMTIME. This makes sure that secs,
    //mins etc won't go over their respective upper bound value limit later on.
    overflowsecs = msecsFloat / 1000;
    millisecs = (int)msecsFloat % 1000;
    systemTime = SecsToSystemTime(secs+overflowsecs, convertToLocalTimeZone);
    systemTime.wMilliseconds = millisecs;

    return systemTime;
}

void printpkt_timestamp(SYSTEMTIME st) {
    char *temp_printf;
    temp_printf = malloc(sizeof(char) * 512);
    memset(temp_printf,0,sizeof(char)*512);

    sprintf(temp_printf, "%02d-%02d-%02d %02d:%02d:%02d.%03u\n", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    writelog(temp_printf,0);

    free(temp_printf);
}
void printpkt(struct ntp_packet *pkt) {
    /*

    Verbose function displaying every timestamps recieved from a raw packet
    from the SNTP server.
    */
    SYSTEMTIME st;

    writelog("\t'Server's Reference' Timestamp: ", 1);
    st = NTPPacketToSystemTime(ntohl(pkt->reference_timestamp_secs), ntohl(pkt->reference_timestamp_fraq), 0);
    printpkt_timestamp(st);

    writelog("\t'Client Original' Timestamp: ", 1);
    st = NTPPacketToSystemTime(ntohl(pkt->originate_timestamp_secs), ntohl(pkt->originate_timestamp_fraq), 0);
    printpkt_timestamp(st);

    writelog("\t'Server Received' Timestamp: ", 1);
    st = NTPPacketToSystemTime(ntohl(pkt->receive_timestamp_secs), ntohl(pkt->receive_timestamp_fraq), 0);
    printpkt_timestamp(st);

    writelog("\t'Server Transmit' Timestamp: ", 1);
    st = NTPPacketToSystemTime(ntohl(pkt->transmit_timestamp_secs), ntohl(pkt->transmit_timestamp_fraq), 0);
    printpkt_timestamp(st);
}

void prepare_packet(unsigned long *secs, unsigned long *msecs, SYSTEMTIME *clientSystemTime) {
    /*
    Translates Windows' local time into UTC with values ready to be injected
    into the SNTP protocol.

    All 3 parameters will be accessed outside the function call.
    */
    FILETIME ftFileTime;
    ULONGLONG qwMSecs;
    unsigned long long qwSecs;
    float fmsecs;
    GetSystemTime(clientSystemTime); //WinAPI call returning the actual time

    //millisecondes calculation:
    fmsecs = (float)clientSystemTime->wMilliseconds / 1000.0f;
    qwMSecs = (float)4294967295.0f * fmsecs;

    //Seconds calculation
    SystemTimeToFileTime(clientSystemTime, &ftFileTime);

    //This isn't great, but that's the only way I found to bypass data checks
    //at compile time:
    memcpy(&qwSecs, &ftFileTime, sizeof(FILETIME));
    //This should have passed as a more legit way, but doesn't compile:
    //qwSecs = (unsigned long long)*ftUTC;

    qwSecs /= 10000000;
    qwSecs -= NTP2FILETIME;

    //returning values:
    *secs = qwSecs;
    *msecs = qwMSecs;
}

int sntp_connect(char *ipAddress) {
    /*

    The big part of the code which deals sending a time value to the SNTP server
    and deals with the server's reply values.

    ipAddress is obviously the IP address of the server. We assume we already
    nameresolved the host (if any) at that point.
    */
    unsigned long sec = 0;      //TODO renommer
    unsigned long msec = 0;     //TODO renommer
    int returnvalue;            //TODO renommer?

    SYSTEMTIME clientSystemTime, serverSystemTime;
    FILETIME clientFileTime, serverFileTime;
    SOCKADDR_IN sktDest;
    SOCKADDR_IN sktLocal;

    long unsigned int qwClient, qwServer;
    float timetmp = 0;          //TODO renommer
    struct ntp_packet pkt;
    char *temp_printf;

    //variables pour select():
    fd_set fds;
    struct timeval tv;

    DWORD err = 0; //variable d'erreur


    sktLocal.sin_family = AF_INET;
    sktLocal.sin_addr.s_addr = 0;
    sktLocal.sin_port = 0; //leaves the local system the job of picking a LPORT
    SOCKET ipSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if (ipSocket == -1 ) wfail_exit("Fonction socket() a echoue", __FILE__, __LINE__);
    if (bind( ipSocket, (SOCKADDR *)&sktLocal, sizeof(sktLocal) ) != 0) {
        wfail_exit("Fonction bind() a echoue", __FILE__, __LINE__);
    }

    temp_printf = malloc(sizeof(char) * 512);
    memset(temp_printf, 0, sizeof(char) * 512);

    memset(&clientFileTime, 0, sizeof(FILETIME));
    memset(&serverFileTime, 0, sizeof(FILETIME));
    memset(&clientSystemTime, 0, sizeof(SYSTEMTIME));
    memset(&serverSystemTime, 0, sizeof(SYSTEMTIME));
    qwClient = 0;
    qwServer = 0;

    //Initialisation de la demande du client
    memset(&pkt, 0, sizeof(pkt));
    pkt.vn = 3;
    pkt.mode = 3;

    prepare_packet(&sec, &msec, &clientSystemTime); // see above.
    pkt.transmit_timestamp_fraq = htonl(msec); //conversion host to network byte order
    pkt.transmit_timestamp_secs = htonl(sec);

    writelog("Sending SNTP client packet:\t", 1);
    returnvalue = 0;
    sktDest.sin_family = AF_INET;
    sktDest.sin_addr.s_addr = inet_addr( ipAddress );
    sktDest.sin_port = htons(123);

    returnvalue = sendto(ipSocket, (char *)&pkt, sizeof(pkt), 0, (SOCKADDR *)&sktDest, sizeof(sktDest));
    if (returnvalue != sizeof(pkt)) {
        printf("*** ERROR *** #%d\n", errno);
        return 1;
    }
    else writelog("OK.\n", 0);

    writelog("Receiving SNTP packet response:\t", 1);
    //Setup timeout (doc: http://stackoverflow.com/questions/1824465/set-timeout-for-winsock-recvfrom )
    tv.tv_sec=NTP_TIMEOUT;
    tv.tv_usec=0;
    FD_ZERO(&fds); // Set up the file descriptor set.
    FD_SET(ipSocket, &fds);

    returnvalue = select(ipSocket, &fds, NULL, NULL, &tv);
    if (returnvalue == 0) {
        writelog("Timeout (no reponse).", 0);
        return 1;
    }
    if (returnvalue == -1) {
        sprintf(temp_printf, "*** ERROR *** #%d", errno);
        writelog(temp_printf, 0);
        return 1;
    }

    //Assuming from that point in the code that the server responded in time.
    returnvalue = recvfrom(ipSocket, (char *)&pkt, sizeof(pkt), 0, NULL, NULL);
    if (returnvalue != sizeof(pkt)) {
        sprintf( temp_printf, "*** ERROR *** in server's packet. errno #%d\n", errno);
        writelog(temp_printf, 0);
        return 1;
    }
    else writelog("OK.\n", 0);


    //Safe checks from server's reply packets
    if (pkt.li == 3 && force_sync == 0) {
        writelog("*** ERROR *** server is NOT in sync!\n", 1);
        writelog("Refusing to change local time from an out-of-sync server.\nType -f on the command-line to force syncing.\n", 1);
        exit(1);
    }
    else {
        //TODO: le serveur non-synchronise 40.9 dit qu'il est en stratum 1 lorsqu'il est flagged alarm?
        sprintf(temp_printf,"Server in-sync, stratum %d.\n", pkt.stratum);
        writelog(temp_printf, 1);
    }

    serverSystemTime = NTPPacketToSystemTime(ntohl(pkt.transmit_timestamp_secs), ntohl(pkt.transmit_timestamp_fraq), 0);
    writelog("PC local time (client): ", 1);
    printpkt_timestamp(clientSystemTime);
    writelog("Server's time (reponse): ", 1);
    printpkt_timestamp(serverSystemTime);

    //TODO: calcul de difference de temps a l'air de chier lorsque alarm
    //Transfering times into FILETIME values for arithmetics. Bill Clinton would be proud.
    if (SystemTimeToFileTime(&clientSystemTime, &clientFileTime) == 0) wfail_exit("SystemTimeToFileTime", __FILE__, __LINE__);
    if (SystemTimeToFileTime(&serverSystemTime, &serverFileTime) == 0) wfail_exit("SystemTimeToFileTime", __FILE__, __LINE__);
    qwClient = (((ULONGLONG) clientFileTime.dwHighDateTime) << 32) + clientFileTime.dwLowDateTime;
    qwServer = (((ULONGLONG) serverFileTime.dwHighDateTime) << 32) + serverFileTime.dwLowDateTime;
    timetmp = (float)(qwServer > qwClient ? (float)(qwServer - qwClient) : (float)(qwClient - qwServer));
    timetmp /= 10000000;

    sprintf(temp_printf,"Time difference between client and server: %f second(s)\n",timetmp); //attention: unsigned. will only tell a positive time difference.
    writelog(temp_printf, 1);

    memset(&serverSystemTime, 0, sizeof(SYSTEMTIME));
    if (FileTimeToSystemTime(&serverFileTime, &serverSystemTime) == 0) wfail_exit("FileTimeToSystemTime", __FILE__, __LINE__);


    //Start token request to change system time. Taken from MSDN somewhere.
    HANDLE hnd;
    memset(&hnd, 0, sizeof(HANDLE));
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &hnd) == 0) wfail_exit("OpenProcessToken", __FILE__, __LINE__);

    TOKEN_PRIVILEGES tkp, tkpo;
    memset(&tkp, 0, sizeof(TOKEN_PRIVILEGES));
    memset(&tkpo, 0, sizeof(TOKEN_PRIVILEGES));
    DWORD buffer = 0;

    LookupPrivilegeValue(NULL, "SE_SYSTEMTIME_NAME",&tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (AdjustTokenPrivileges(hnd, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), &tkpo, &buffer) == 0) wfail_exit("AdjustTokenPrivileges", __FILE__, __LINE__);
    CloseHandle(hnd);
    if (SetSystemTime(&serverSystemTime) == 0) {
        //Fails with error no #1314 if WinXP+ and that the user isn't a member of the administrator group.
        err = GetLastError();
        if (err == 1314) writelog("\n**** ERROR *** Unable to change time without a process elevation\non WinXP and above.\n\nYou need to run this program as Administrator (or equivalent).\n", 1);
        else wfail_exit("SetSystemTime", __FILE__, __LINE__);
        writetologfile("==========================================\n", 0);
        exit(1);
    }
    else writelog("Time changed.\n", 1);
    writetologfile("==========================================\n", 0);
    exit(0);
}
