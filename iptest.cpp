//随机端口
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

using namespace std;

int main()
{
    struct sockaddr_in name;
    int port = 0;
    int mysock = socket(PF_INET,SOCK_STREAM, 0);
    
    memset(&name ,0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(mysock, (struct sockaddr*)&name, sizeof(name));


    socklen_t namelen = sizeof(name);
    getsockname(mysock, (struct sockaddr*)&name, &namelen);

    port = ntohs(name.sin_port);

    cout<<port<<endl;


}
