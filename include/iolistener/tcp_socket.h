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

#pragma once
#include <logbook/expect.h>
#include <logbook/object.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include <netdb.h>
#include <syslog.h>
#include <fcntl.h>


#include "iolistener/ifd.h"


namespace io
{

class tcp_socket : public book::object
{
    hostent*                m_hentry;
    sockaddr_in             m_addr_in;
    std::string             m_node;
    std::string             m_ip;
    int                     m_herror;
    int                     m_fd;
    bool                    m_connected;
    uint                    m_port;
    ifd*                    m_ifd;

public:
    tcp_socket(object* parent,const std::string& ii);
    tcp_socket();

    static u_int32_t tolocal ( u_int32_t n )
    {
        return ntohl ( n );
    }
    static u_int32_t toinet ( u_int32_t n )
    {
        return htonl ( n );
    }

    static u_int16_t tolocal(u_int16_t n) {
        return ntohs(n);
    }

    static u_int16_t toinet(u_int16_t n) {
        return htons(n);
    }

    static hostent* host ( const char* node, uint port, sockaddr_in* _addr_in, std::string& NodeIP );
    static int mkaddr(void* addr, int* addr_len, const char* addr_str, const char* proto);
    static char* machine_hostname();
    int create();
    void set_sockfd(int fd);
    ifd* i_fd(){ return m_ifd; }

    ~tcp_socket() override;
};

} // namespace oldcc::io


