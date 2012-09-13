#include <windows.h>
#include <stdio.h>
//#pragma comment(lib, "cmcfg32.lib")

int SetPrivilege(
    HANDLE hToken,          // access token handle
    LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
    int bEnablePrivilege   // to enable or disable privilege
    )
{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if ( !LookupPrivilegeValue(
            NULL,            // lookup privilege on local system
            lpszPrivilege,   // privilege to lookup
            &luid ) )        // receives LUID of privilege
    {
        printf("LookupPrivilegeValue error: %u\n", (int)GetLastError() );
        return 0;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege == 1) tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else tp.Privileges[0].Attributes = 0;

    // Enable the privilege or disable all privileges.
    if ( !AdjustTokenPrivileges(hToken,FALSE,&tp,sizeof(TOKEN_PRIVILEGES),
                                (PTOKEN_PRIVILEGES) NULL,(PDWORD) NULL) ) {
          printf("AdjustTokenPrivileges error: %u\n", (int)GetLastError() );
          return 0;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED){
          printf("The token does not have the specified privilege. \n");
          return 0;
    }
    return 1;
}
