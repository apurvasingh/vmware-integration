#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifndef WIN32
#include <unistd.h>
#else
#define strcasecmp stricmp
#endif
#include <vix.h>

#include "vixconnection.hpp"
#include "vmhconfig.hpp"

bool processCmdLineArgs(int argc, char **argv);
void installOnVM(VmhConfig *config, VixConnection *vconn, char *vmxpath);
void uninstallFromVM(VmhConfig *config, VixConnection *vconn, char *vmxpath);
void printUsage(char *progName);
void finderStoreVMX(char *vmxPath);
void finderWriteListFile(char *vmxpath);
void closeListFile(void);
bool isNumber(char *str);
bool readListFile(int index);
void displayVM(VixConnection *vconn, VmhConfig *config, char *vmxpath, int count);

#ifdef WIN32
extern "C" {

char *basename(char *path)
{
   static char path_buffer[_MAX_PATH];
   static char drive[_MAX_DRIVE];
   static char dir[_MAX_DIR];
   static char fname[_MAX_FNAME];
   static char ext[_MAX_EXT];

   _splitpath(path, drive, dir, fname, ext );
   if (strlen(ext) > 0) {
       sprintf(path_buffer,"%s%s",fname,ext);
       return path_buffer;
   } else {
       return fname;
   }
}
void _forceCRTManifestCUR()
{
}

}

char *haloTestDest = ".\\halo_test";
char *dstBanner = ".\\issue.net";

#else

char *haloTestDest = "/tmp/halo_test";
char *dstBanner = "/tmp/issue.net";

#endif

static const char *configFilename1 = "vmhalo.cfg";
static const char *configFilename2 = "../vmhalo.cfg";
static char *vmxpathList[20];
static int vmxpathListMax = sizeof(vmxpathList)/sizeof(vmxpathList[0]);
static int vmxpathCount = 0;
static const char *vmlistFilename = "vmlist.txt";
static char *overrideTag = NULL;

bool verbose = false;
bool shouldUninstall = false;

int main(int argc, char **argv)
{
    if (! processCmdLineArgs(argc,argv))
        return 1;

    VmhConfig *config = new VmhConfig();
    if (config->readFromFile(configFilename1)) {
        if (verbose) {
            fprintf(stderr,"Successfully read config from %s\n",configFilename1);
            config->dump();
        }
    } else if (config->readFromFile(configFilename2)) {
        if (verbose) {
            fprintf(stderr,"Successfully read config from %s\n",configFilename2);
            config->dump();
        }
    } else {
        fprintf(stderr,"Unable to read config file (%s), exiting.\n",configFilename1);
        return 1;
    }

    VixConnection *vconn = new VixConnection();
    vconn->setServerType(config->getServerType());
    vconn->setHostInfo(config->getServerHost());
    vconn->setUserInfo(config->getServerUser(),config->getServerPass());

    if (vconn->connect()) {
        if (vmxpathCount == 0) {
            vconn->findVMs(finderWriteListFile);
            closeListFile();
            for (int i = 0; i < vmxpathCount; i++)
                displayVM(vconn,config,vmxpathList[i],i + 1);
        } else {
            if ((vmxpathCount == 1) && (strcmp(vmxpathList[0],"ALL") == 0)) {
                vmxpathCount = 0; // reset so it removes "ALL" from list
                vconn->findVMs(finderStoreVMX);
            }
            for (int i = 0; i < vmxpathCount; i++)
                if (shouldUninstall)
                    uninstallFromVM(config,vconn,vmxpathList[i]);
                else
                    installOnVM(config,vconn,vmxpathList[i]);
        }
        // sleep(2);
        vconn->disconnect();
        if (verbose) printf("Done!\n");
    }
    return 0;
}

char *getGuestType(VixConnection *vconn, char *vmxpath)
{
    static char *srcBanner = "/etc/issue.net";
    static char bannerPrefix[1024];

    FILE *fp = NULL;
    if (! vconn->fileExistsInVM(srcBanner))
        return "win64";

    if (vconn->copyFromVM(srcBanner,dstBanner) && ((fp = fopen(dstBanner,"r")) != NULL)) {
        fscanf(fp,"%s",bannerPrefix);
        if ((strcasecmp(bannerPrefix,"CentOS") == 0) || (strcasecmp(bannerPrefix,"Fedora") == 0) ||
            (strcasecmp(bannerPrefix,"CentOS") == 0)) {
            return "redhat";
        } else if ((strcasecmp(bannerPrefix,"Debian") == 0) || (strcasecmp(bannerPrefix,"Ubuntu") == 0)) {
            return "debian";
        } else
            return "redhat"; // for now, just assume
    }
    return "win64";
}

bool guestOsIsLinux(char *guestOSname)
{
    return strncmp("win",guestOSname,3) != 0;
}

bool alreadyInstalledOnVM(VixConnection *vconn, char *guestOSname)
{
    if (guestOsIsLinux(guestOSname))
        return vconn->fileExistsInVM("/etc/init.d/cphalod");
    else
        return vconn->fileExistsInVM("c:\\Program Files\\CloudPassage\\LICENSE");
}

void installOnVM(VmhConfig *config, VixConnection *vconn, char *vmxpath)
{
    char destName[512];
    char srcNameCopy[512];
    char script[1024];
    if (vconn->openVM(vmxpath,config->getGuestUser(NULL),
                              config->getGuestPass(NULL))) {
        char *guestType = getGuestType(vconn,vmxpath);
        if (verbose) fprintf(stderr,"Detected OS %s on %s\n",guestType,vmxpath);
        if (alreadyInstalledOnVM(vconn,guestType)) {
            fprintf(stderr,"Halo already installed on guest %s\n",vmxpath);
            return;
        }
        char *srcName = config->getInstaller(guestType);
        if (srcName == NULL) {
            fprintf(stderr,"couldn't find installer name for guest type %s\n",guestType);
            return;
        }
        char *tag = config->getHostTag(guestType);
        if (overrideTag != NULL)
            tag = overrideTag;

        strcpy(srcNameCopy,srcName);
        if (guestOsIsLinux(guestType)) {
            sprintf(destName,"/tmp/%s",basename(srcNameCopy));
            vconn->copyToVM(srcName,destName);

            sprintf(script,"%s %s %s > /tmp/halo_install.log 2>&1\n",destName,config->getHaloKey(),tag);
            vconn->runScript("/bin/sh",script);
        } else {
            sprintf(destName,"c:\\%s",basename(srcNameCopy));
            sprintf(script,"/S /DAEMON-KEY=%s /TAG=%s",config->getHaloKey(),tag);
            vconn->copyToVM(srcName,destName);
            vconn->runCmd(destName,script);
        }
    } else {
        fprintf(stderr,"Failed to login to %s\n",vmxpath);
    }
}

void uninstallFromVM(VmhConfig *config, VixConnection *vconn, char *vmxpath)
{
    if (vconn->openVM(vmxpath,config->getGuestUser(NULL),
                              config->getGuestPass(NULL))) {
        char *guestType = getGuestType(vconn,vmxpath);
        if (verbose) fprintf(stderr,"Detected OS %s on %s\n",guestType,vmxpath);
        if (! alreadyInstalledOnVM(vconn,guestType)) {
            fprintf(stderr,"Halo not installed on guest %s\n",vmxpath);
            return;
        }
        char *cmd  = config->getUninstallCmd(guestType);
        char *args = config->getUninstallArgs(guestType);
        vconn->runCmd(cmd,args);
    } else {
        fprintf(stderr,"Failed to login to %s\n",vmxpath);
    }
}

void printUsage(char *progName)
{
    fprintf(stderr,"Usage: %s [<flags>] <vmxpath> [<vmxpath> ...]\n",progName);
    fprintf(stderr,"   or: %s [<flags>] ALL\n",progName);
    fprintf(stderr,"\n");
    fprintf(stderr,"Where <vmxpath> identifies an individual VM, or ALL specifies all VMs on a host\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"And <flags> can be one or more of the following:\n");
    fprintf(stderr,"-h\t\tThis message\n");
    fprintf(stderr,"-v\t\tSpecify verbose output messages\n");
    fprintf(stderr,"-u\t\tUninstall Halo daemon from VM\n--uninstall\tsame as above\n");
    fprintf(stderr,"--tag=<tag>\tOverride tag value in config file\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"Config file is normally %s or %s\n",configFilename1,configFilename2);
}

bool processCmdLineArgs(int argc, char **argv)
{
    for (int i = 0; i < vmxpathListMax; i++)
        vmxpathList[i] = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-v") == 0) {
            verbose = true;
        } else if ((strcmp(argv[i],"-h") == 0) || (strcmp(argv[i],"-?") == 0)) {
            printUsage(argv[0]);
            return false;
        } else if ((strcmp(argv[i],"-u") == 0) || (strcmp(argv[i],"--uninstall") == 0)) {
            shouldUninstall = true;
        } else if (strncmp(argv[i],"--tag=",6) == 0) {
            overrideTag = argv[i] + 6;
            if (verbose)
                fprintf(stderr,"Using \"%s\" to tag guests\n",overrideTag);
        } else {
            if (vmxpathCount < vmxpathListMax) {
                if (isNumber(argv[i])) {
                    readListFile(atoi(argv[i]));
                } else {
                    vmxpathList[vmxpathCount++] = argv[i];
                }
            }
        }
    }
    return true;
}

void finderStoreVMX(char *vmxpath)
{
    if (vmxpathCount < vmxpathListMax)
        vmxpathList[vmxpathCount++] = vmxpath;
}


static FILE *listFP = NULL;
static int listCount = 0;

void finderWriteListFile(char *vmxpath)
{
    char *installed = "";
    if (listFP == NULL) {
        if ((listFP = fopen(vmlistFilename,"w")) == NULL) {
            perror(vmlistFilename);
            return;
        }
    }
    listCount++;
    fprintf(listFP,"%s\n",vmxpath);
    finderStoreVMX(vmxpath);
}

void displayVM(VixConnection *vconn, VmhConfig *config, char *vmxpath, int count)
{
    if (verbose) fprintf(stderr,"Attempting to connect to: %s\n",vmxpath);
    char *installed = "";
    if (vconn->openVM(vmxpath,config->getGuestUser(NULL),
                              config->getGuestPass(NULL))) {
        char *guestType = getGuestType(vconn,vmxpath);
        if (verbose) fprintf(stderr,"Detected OS %s on %s\n",guestType,vmxpath);
        if (alreadyInstalledOnVM(vconn,guestType)) {
            installed = " (Halo installed)";
        }
    }
    fprintf(stderr,"Found VM: %d: %s%s\n",count,vmxpath,installed);
}

void closeListFile(void)
{
    if (listFP != NULL)
        fclose(listFP);
}

bool readListFile(int index)
{
    char buf[4096];
    if (listFP == NULL) {
        if ((listFP = fopen(vmlistFilename,"r")) == NULL) {
            perror(vmlistFilename);
            return false;
        }
    } else {
        rewind(listFP);
    }
    for (int i = 0; i < index; i++) {
        if (fgets(buf,4095,listFP) == NULL) {
            fprintf(stderr,"Specified index %d is greater than lines in file %d\n",index,i);
            return false;
        }
        // trim(buf); fprintf(stderr,"Reading list file: %d: %s\n",i,buf);
    }
    trim(buf);
    finderStoreVMX(strdup(buf));
    return true;
}

bool isNumber(char *str)
{
    for (; *str; str++)
        if (! isdigit(*str))
            return false;
    return true;
}
