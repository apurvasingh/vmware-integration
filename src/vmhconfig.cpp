#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <vix.h>

#include "vmhconfig.hpp"

VmhConfig::VmhConfig()
{
    strcpy(serverType,"player"); // provide default value
    strcpy(serverHost,"");
    strcpy(serverUser,"");
    strcpy(serverPass,"");
    strcpy(guestUser,"");
    strcpy(guestPass,"");
    guestTypeCount = 0;
    strcpy(halokey,"");
}

char linebuf[4096];

static bool startsWith(char *str, const char *prefix)
{
    if (strncmp(str,prefix,strlen(prefix)) == 0)
        return true;
    else
        return false;
}

void trim(char *str)
{
    int n;

    if ((str == NULL) || (strlen(str) < 1))
        return;
    while ((n = strlen(str)) > 0) {
        char last = str[n - 1];
        if ((last == '\n') || (last == '\r'))
            str[n - 1] = '\0';
        else
            break;
    }
}

void VmhConfig::initGuest(GuestTypeInfo *guest, char *name)
{
    strcpy(guest->typeName,name);
    strcpy(guest->installPackage,"");
    strcpy(guest->uninstallCommand,"");
    strcpy(guest->uninstallArgs,"");
    strcpy(guest->guestUser,"");
    strcpy(guest->guestPass,"");
    strcpy(guest->hostTag,"");
}

bool VmhConfig::readFromFile(const char *filename)
{
    char *line = NULL;
    FILE *fp = fopen(filename,"r");
    if (fp == NULL) {
        // perror(filename);
        return false;
    }
    while ((line = fgets(linebuf,4095,fp)) != NULL) {
        while ((*line == ' ') || (*line == '\t'))
            line++;
        trim(line);

        if (startsWith(line,"#"))
            continue;
        else if (startsWith(line,"type="))
            strncpy(serverType,line + 5,255);
        else if (startsWith(line,"host="))
            strncpy(serverHost,line + 5,255);
        else if (startsWith(line,"user="))
            strncpy(serverUser,line + 5,31);
        else if (startsWith(line,"username="))
            strncpy(serverUser,line + 9,31);
        else if (startsWith(line,"pass="))
            strncpy(serverPass,line + 5,31);
        else if (startsWith(line,"password="))
            strncpy(serverPass,line + 9,31);
        else if (startsWith(line,"guestuser="))
            strncpy(guestUser,line + 10,31);
        else if (startsWith(line,"guestpass="))
            strncpy(guestPass,line + 10,31);
        else if (startsWith(line,"guesttype=")) {
            initGuest(guestTypes + guestTypeCount,line + 10);
            guestTypeCount++;
        } else if (startsWith(line,"installer="))
            strncpy(guestTypes[guestTypeCount - 1].installPackage,line + 10,511);
        else if (startsWith(line,"uninstall="))
            strncpy(guestTypes[guestTypeCount - 1].uninstallCommand,line + 10,511);
        else if (startsWith(line,"uninstallArgs="))
            strncpy(guestTypes[guestTypeCount - 1].uninstallArgs,line + 14,511);
        else if (startsWith(line,"tag="))
            strncpy(guestTypes[guestTypeCount - 1].hostTag,line + 4,255);
        else if (startsWith(line,"halokey="))
            strncpy(halokey,line + 8,255);
    }
    fclose(fp);
    return true;
}

GuestTypeInfo *VmhConfig::getTypeInfo(char *type)
{
    if (type != NULL) {
        for (int i = 0; i < guestTypeCount; i++)
            if (strcmp(guestTypes[i].typeName,type) == 0)
                return guestTypes + i;
    }
    return NULL;
}

char *VmhConfig::getInstaller(char *type)
{
    GuestTypeInfo *typeInfo = getTypeInfo(type);
    if (typeInfo != NULL)
        return typeInfo->installPackage;
    else
        return NULL;
}

char *VmhConfig::getUninstallCmd(char *type)
{
    GuestTypeInfo *typeInfo = getTypeInfo(type);
    if (typeInfo != NULL)
        return typeInfo->uninstallCommand;
    else
        return NULL;
}

char *VmhConfig::getUninstallArgs(char *type)
{
    GuestTypeInfo *typeInfo = getTypeInfo(type);
    if (typeInfo != NULL)
        return typeInfo->uninstallArgs;
    else
        return NULL;
}

char *VmhConfig::getHostTag(char *type)
{
    GuestTypeInfo *typeInfo = getTypeInfo(type);
    if (typeInfo != NULL)
        return typeInfo->hostTag;
    else
        return NULL;
}

char *VmhConfig::getGuestUser(char *type)
{
    GuestTypeInfo *typeInfo = getTypeInfo(type);
    if ((typeInfo != NULL) && (strlen(typeInfo->guestUser) > 0))
        return typeInfo->guestUser;
    else
        return guestUser;
}

char *VmhConfig::getGuestPass(char *type)
{
    GuestTypeInfo *typeInfo = getTypeInfo(type);
    if ((typeInfo != NULL) && (strlen(typeInfo->guestPass) > 0))
        return typeInfo->guestPass;
    else
        return guestPass;
}

void VmhConfig::dump(void)
{
    printf("--config--\n");
    printf("--config--\ntype=%s\nhost=%s\nuser=%s\npass=%s\n",
           serverType,serverHost,serverUser,serverPass);
    printf("guest.user=%s\nguest.pass=%s\n",guestUser,guestPass);
    printf("daemon.key=%s\n",halokey);
    for (int i = 0; i < guestTypeCount; i++) {
        printf("installer[%s]=%s\n",guestTypes[i].typeName,guestTypes[i].installPackage);
        printf("uninstaller[%s]=\"%s\" \"%s\"\n",guestTypes[i].typeName,guestTypes[i].uninstallCommand,guestTypes[i].uninstallArgs);
        printf("tag[%s]=%s\n",guestTypes[i].typeName,guestTypes[i].hostTag);
    }
    printf("--end--\n");
}
