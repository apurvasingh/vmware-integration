#ifndef VMH_CONFIG_HPP
#define VMH_CONFIG_HPP

typedef struct s_guest_type_info {
    char typeName[20];
    char installPackage[512];
    char uninstallCommand[512];
    char uninstallArgs[512];
    char guestUser[32];
    char guestPass[32];
    char hostTag[256];
} GuestTypeInfo;

void trim(char *str);

class VmhConfig
{
    public:
        VmhConfig();
        bool readFromFile(const char *filename);
        void dump(void);
        char *getServerType() { return serverType; }
        char *getServerHost() { return serverHost; }
        char *getServerUser() { return serverUser; }
        char *getServerPass() { return serverPass; }
        char *getGuestUser(char *type = NULL);
        char *getGuestPass(char *type = NULL);
        char *getInstaller(char *type);
        char *getUninstallCmd(char *type);
        char *getUninstallArgs(char *type);
        char *getHostTag(char *type);
        char *getHaloKey() { return halokey; }
    private:
        void initGuest(GuestTypeInfo *guest, char *name);
        GuestTypeInfo *getTypeInfo(char *type);
        char halokey[256];
        char serverType[256];
        char serverHost[256];
        char serverUser[32];
        char serverPass[32];
        char guestUser[32];
        char guestPass[32];
        GuestTypeInfo guestTypes[6];
        int guestTypeCount;
};

#endif
