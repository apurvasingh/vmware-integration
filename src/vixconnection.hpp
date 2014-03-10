#ifndef VIX_CONNECTION_HPP
#define VIX_CONNECTION_HPP

class VixConnection;
class VmhConfig;

typedef void (*FinderCallback)(char *vmxPath);

class VixConnection
{
    public:
        VixConnection();
        bool connect();
        void setHostInfo(char *hostname, int port = 0);
        void setUserInfo(char *username, char *password);
        void setServerType(int type) { serverType = type; }
        void setServerType(const char *type);
        void disconnect();
        void findVMs(FinderCallback cb);
        bool openVM(char *vmxpath, char *guestUsername, char *guestPassword);
        bool runCmd(char *cmd, char *args);
        bool runScript(char *shell, char *scriptText);
        bool copyFromVM(char *remotePath, char *localPath);
        bool copyToVM(char *localPath, char *remotePath);
        bool fileExistsInVM(char *remotePath);
    private:
        char hostname[256];
        int  port;
        char username[32];
        char password[32];
        int  serverType;
        VixHandle hostHandle;
        VixHandle vmHandle;
        bool retryPasswordAllowed;
};

#endif
