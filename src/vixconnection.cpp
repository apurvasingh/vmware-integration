#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#define strcasecmp stricmp
#endif

#include <vix.h>

#include "vixconnection.hpp"
#include "vmhconfig.hpp"

VixConnection::VixConnection()
{
    strcpy(hostname,"");
    port = 0;
    strcpy(username,"");
    strcpy(password,"");
    hostHandle = VIX_INVALID_HANDLE;
    retryPasswordAllowed = true;
}

bool VixConnection::connect()
{
    VixError    err;
    VixHandle jobHandle = VixHost_Connect(VIX_API_VERSION, serverType, hostname, port,
                                          username, password, 0,
                                          VIX_INVALID_HANDLE, NULL, NULL);
    err = VixJob_Wait(jobHandle, VIX_PROPERTY_JOB_RESULT_HANDLE, &hostHandle, VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);
    if (VIX_FAILED(err)) {
        fprintf(stderr, "Connect failure: %s\n", Vix_GetErrorText(err, NULL));
        return false;
    } else
        return true;
}

void VixConnection::setHostInfo(char *hostname, int port)
{
    strncpy(this->hostname,hostname,256);
    this->port = port;
}

void VixConnection::setUserInfo(char *username, char *password)
{
    strncpy(this->username,username,32);
    strncpy(this->password,password,32);
}

typedef struct s_server_type {
    int typeNum;
    const char *typeName;
} ServerType;

static ServerType type_list[] = {
    { VIX_SERVICEPROVIDER_VMWARE_SERVER, "server" },
    { VIX_SERVICEPROVIDER_VMWARE_WORKSTATION, "workstation" },
    { VIX_SERVICEPROVIDER_VMWARE_PLAYER, "player" },
    { VIX_SERVICEPROVIDER_VMWARE_VI_SERVER, "vi" },
    { VIX_SERVICEPROVIDER_VMWARE_WORKSTATION_SHARED, "shared" }
};
static int type_count = sizeof(type_list)/sizeof(type_list[0]);

void VixConnection::setServerType(const char *type)
{
    for (int i = 0; i < type_count; i++) {
        if (strcasecmp(type_list[i].typeName,type) == 0) {
            serverType = type_list[i].typeNum;
            return;
        }
    }
    fprintf(stderr,"Unknown vm host type: %s\n",type);
}

typedef struct s_callback_info {
    FinderCallback    userCB;
    VixConnection     *vconn;
    VmhConfig         *config;
} CallbackInfo;
static CallbackInfo cbinfo = { NULL, NULL, NULL };

static void internalFinderCallback(VixHandle job, VixEventType event, VixHandle moreInfo, void *cd)
{
    char *location = NULL;
    if (event != VIX_EVENTTYPE_FIND_ITEM)
        return;

    VixError err = Vix_GetProperties(moreInfo, VIX_PROPERTY_FOUND_ITEM_LOCATION, &location, VIX_PROPERTY_NONE);
    if (VIX_SUCCEEDED(err)) {
        if (cbinfo.userCB != NULL) {
            (*cbinfo.userCB)(location);
        } else {
            fprintf(stderr,"Found VM: %s\n",location);
        }
    } else {
        fprintf(stderr,"GetProperties failed: %s",Vix_GetErrorText(err,NULL));
    }
}

void VixConnection::findVMs(FinderCallback cb)
{
    cbinfo.userCB = cb; // store info for later, should pass as "Callback Data" (cd) argument
    cbinfo.vconn = this;
    VixHandle jobHandle = VixHost_FindItems(hostHandle,VIX_FIND_RUNNING_VMS,VIX_INVALID_HANDLE,
                                            -1,internalFinderCallback,NULL);
    VixError err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);
    if (VIX_FAILED(err)) {
        fprintf(stderr,"Find VMs failed: %s\n",Vix_GetErrorText(err,NULL));
        return;
    }
}

void VixConnection::disconnect()
{
    VixHost_Disconnect(hostHandle);
}

bool VixConnection::runCmd(char *cmd, char *args)
{
    VixHandle jobHandle = VixVM_RunProgramInGuest(vmHandle,cmd,args,0,VIX_INVALID_HANDLE,NULL,NULL);
    VixError err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);
    if (VIX_FAILED(err)) {
        fprintf(stderr,"Failed to run program %s: %s\n",cmd,Vix_GetErrorText(err,NULL));
        return false;
    }
    return true;
}

bool VixConnection::runScript(char *shell, char *scriptText)
{
    VixHandle jobHandle = VixVM_RunScriptInGuest(vmHandle,shell,scriptText,
                                                 0,VIX_INVALID_HANDLE,NULL,NULL);
    VixError err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);
    if (VIX_FAILED(err)) {
        fprintf(stderr,"Login to VM failed: %s\n",Vix_GetErrorText(err,NULL));
        return false;
    }
    return true;
}

bool VixConnection::openVM(char *vmxpath, char *guestUsername, char *guestPassword)
{
    char username_buf[4096];
    char password_buf[4096];
    VixHandle jobHandle = VixVM_Open(hostHandle,vmxpath,NULL,NULL);
    VixError err = VixJob_Wait(jobHandle, VIX_PROPERTY_JOB_RESULT_HANDLE, &vmHandle, VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);
    if (VIX_FAILED(err)) {
        fprintf(stderr,"Open VM failed: %s\n",Vix_GetErrorText(err,NULL));
        return false;
    } else {
        jobHandle = VixVM_LoginInGuest(vmHandle, guestUsername, guestPassword, 0, NULL, NULL);
        err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
        Vix_ReleaseHandle(jobHandle);
        if (VIX_FAILED(err)) {
            fprintf(stderr,"Login to VM failed: %s\n",Vix_GetErrorText(err,NULL));
            if (retryPasswordAllowed) {
                printf("To retry login with different credentials, enter new username: ");
                fflush(stdout);
                fgets(username_buf,4000,stdin);
                trim(username_buf);
                if (strlen(username_buf) > 0) {
                    printf("Enter new password: ");
                    fflush(stdout);
                    fgets(password_buf,4000,stdin);
                    trim(password_buf);
                    jobHandle = VixVM_LoginInGuest(vmHandle, username_buf, password_buf, 0, NULL, NULL);
                    err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
                    Vix_ReleaseHandle(jobHandle);
                    if (VIX_FAILED(err)) {
                        fprintf(stderr,"Login to VM failed again: %s\n",Vix_GetErrorText(err,NULL));
                        return false;
                    }
                    return true; // retry succeeded
                }
            }
            return false;
        }
        return true;
    }
}

bool VixConnection::copyFromVM(char *remotePath, char *localPath)
{
    VixHandle jobHandle = VixVM_CopyFileFromGuestToHost(vmHandle, remotePath, localPath,
                                                        0, VIX_INVALID_HANDLE, NULL, NULL);
    VixError err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);
    if (VIX_FAILED(err)) {
        fprintf(stderr,"File copy from VM failed: %s: %s\n",remotePath,Vix_GetErrorText(err,NULL));
        return false;
    }
    return true;
}

bool VixConnection::copyToVM(char *localPath, char *remotePath)
{
    VixHandle jobHandle = VixVM_CopyFileFromHostToGuest(vmHandle, localPath, remotePath,
                                                        0, VIX_INVALID_HANDLE, NULL, NULL);
    VixError err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);
    if (VIX_FAILED(err)) {
        fprintf(stderr,"File copy to VM failed: %s: %s\n",localPath,Vix_GetErrorText(err,NULL));
        return false;
    }
    return true;
}

bool VixConnection::fileExistsInVM(char *remotePath)
{
    int fileExists = 0;
    VixHandle jobHandle = VixVM_FileExistsInGuest(vmHandle, remotePath, NULL, NULL);
    VixError err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    if (VIX_FAILED(err)) {
        Vix_ReleaseHandle(jobHandle);
        return false;
    }
    err = Vix_GetProperties(jobHandle,VIX_PROPERTY_JOB_RESULT_GUEST_OBJECT_EXISTS,&fileExists,VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);
    if (VIX_FAILED(err)) {
        return false;
    }
    return fileExists != 0;
}
