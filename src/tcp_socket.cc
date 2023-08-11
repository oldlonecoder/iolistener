/***************************************************************************
 *   Copyright (C) 1965/1987/2023 by Serge Lussier                         *
 *   serge.lussier@oldlonecoder.club                                       *
 *                                                                         *
 *                                                                         *
 *   Unless otherwise specified, all code in this project is written       *
 *   by the author (Serge Lussier)                                         *
 *   and no one else then not even {copilot, chatgpt, or any other AI}     *
 *   --------------------------------------------------------------------- *
 *   Copyrights from authors other than Serge Lussier also apply here      *
 ***************************************************************************/


#include "iolistener/tcp_socket.h"
#include <chrtools/stracc.h>
#include <chrtools/strbrk.h>


using namespace book;

namespace io {


tcp_socket::tcp_socket(object* parent, const std::string& ii): object(parent,ii)
{
    m_ifd = 0;
}

tcp_socket::tcp_socket()
{
    m_ifd = 0;
}

tcp_socket::~tcp_socket()
{
}

int tcp_socket::create()
{
    m_fd = ::socket(PF_INET,SOCK_STREAM, 0);
    //u_int32_t options = ifd::O_BLOCK|ifd::O_BUF|ifd::O_READ|ifd::O_WRITE|ifd::O_WINDOWED;
    stracc str = "%s:%s";
    str << id() << m_fd;
    m_ifd = new ifd(m_fd, 0);
    //m_ifd->set_options(options);
    m_ifd->fd = m_fd;

    return m_fd;
}

void tcp_socket::set_sockfd(int fd)
{
    m_fd = fd;
    //u_int32_t options = ifd::O_BLOCK|ifd::O_BUF|ifd::O_READ|ifd::O_WRITE|ifd::O_WINDOWED;
    m_ifd = new ifd(m_fd,0);
    //m_ifd->set_options(options);
    m_ifd->fd = fd;
}


hostent* tcp_socket::host(const char* node, uint port, sockaddr_in* _addr_in, std::string& NodeIP)
{
    hostent* he = gethostbyname(node);

    int a,b,c,d;

    stracc SNodeIP = "%d.%d.%d.%d";


    if (!he ) {
        rem::push_error(HERE) << "gethostbyname failed!!";
        ///@todo traiter l'erreur du DNS
        return 0l;
    }

    // Pas de IPV6... c'est pas demain que cette version va arriver.... bon gre - malgre ...
    a = (int)((unsigned char)he->h_addr_list[0][0]);
    b = (int)((unsigned char)he->h_addr_list[0][1]);
    c = (int)((unsigned char)he->h_addr_list[0][2]);
    d = (int)((unsigned char)he->h_addr_list[0][3]);

    // Juste  pour le deboggage:
    rem::push_debug(HERE)
        << "host:"
        << he->h_name
        << " Adresse IP effective: " << a << "." << b <<"." << c<< "." << d
        << "   contenu de h_addr_list[0]:" << (unsigned long)he->h_addr_list[0] << " PORT:" << port;
    ////////////////////////////////////////////////////////////////////////////////
    SNodeIP << a << b << c << d;
    NodeIP = SNodeIP();
    rem::push_debug(HERE) << "node=" << NodeIP;


    if (_addr_in) {
        memset(_addr_in, 0, sizeof(sockaddr_in));
        _addr_in->sin_family = AF_INET;
        _addr_in->sin_port   = htons(port);
        _addr_in->sin_addr.s_addr = inet_addr(NodeIP.c_str());
    }

    return he;
}

char* tcp_socket::machine_hostname()
{
    char h [128];
    int R = gethostname(h,127);
    if(!R){
        return strdup(h);
    }
    return 0;
}

int tcp_socket::mkaddr(void* addr, int* addr_len, const char* addr_str, const char* proto)
{
    std::string saddress = addr_str;
    strbrk::token_t::list htokens;
    strbrk brk{saddress};
    int n = brk(htokens,":",true);
    if (n!=2) return -1;
    std::string inaddress = (*htokens.begin())();
    std::string serviceport = htokens.back()();
    std::string protoc = proto;
    sockaddr_in* ap = (sockaddr_in*)addr;
    hostent* hp = 0;
    servent* sp = 0;
    char *cp;
    long lv;


    // Attribuer les valeurs par default

    protoc = protoc.empty() ? protoc : "tcp";


    // Initialiser la structure de l'adresse
    memset(ap,0, *addr_len);
    ap->sin_family  = AF_INET;
    ap->sin_port    = 0;
    ap->sin_addr.s_addr = INADDR_ANY;

    // integrer l'adresse hôte
    if ( inaddress != "*" ) {
        if (isdigit(inaddress[0])) {
            ap->sin_addr.s_addr = inet_addr(inaddress.c_str());
            if (ap->sin_addr.s_addr == INADDR_NONE) return -1;
        }
        else {
            // Assumer le host name
            if ( !(hp = gethostbyname(inaddress.c_str())) ) return  -1;
            if (hp->h_addrtype != AF_INET) return -1;
            ap->sin_addr = *(in_addr*)hp->h_addr_list[0];
        }
    }

    // Traiter le # du port

    if (serviceport != "*") {
        if ( isdigit(serviceport[0])) {
            lv=strtol(serviceport.c_str(),&cp,10);
            if (cp && *cp) return -1; // for now do nothing with cp. I  may use it later if detailed error is needed.
            if (lv<0 || lv>=65536) return -2;
            ap->sin_port = htons((short)lv);
        }
        else
        {
            std::cerr << "sp=" << serviceport << std::endl;
            sp = getservbyname(serviceport.c_str(), proto);
            if (sp) ap->sin_port = sp->s_port;
        }
    }

    *addr_len = sizeof(*ap);

    rem::push_info(HERE) << "finalized socket addr infos [port]::" << ntohs(ap->sin_port);
    return 0;
}

} // namespace oldcc::io




