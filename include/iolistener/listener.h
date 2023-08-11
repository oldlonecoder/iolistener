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

#include "iolistener/ifd.h"
#include <logbook/expect.h>
#include <logbook/object.h>
#include <logbook/notify.h>

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <sys/types.h>

#include <sys/epoll.h>

#include <fcntl.h>
#include <thread>
#include <mutex>


using book::notify;
using book::expect;

namespace io
{


class  listener :public book::object
{

    ifd::list   _ifds;
    int         _maxifd = 3;
    epoll_event _epoll_event;
    int         _epollfd = -1;
    int         _epollnumfd = -1;
    bool        _terminate = false;
    notify<> _idle_signal{"idle"};
    notify<ifd&> _hup_signal{"hup"}, _error_signal{"error"}, _zero_signal{"zero"};


    int msec = -1; ///< default to infinite.
public:
    listener()  = default;
    explicit listener(object* parent_, int msec_=-1);
    ~listener() override;

    expect<> run();
    expect<> add_ifd(int fd_, uint32_t opt_);
    expect<> remove_ifd(int fd_);
    expect<> pause_ifd(int fd_);
    expect<> init();
    expect<> shutdown();
    notify<>& idle_signal() { return _idle_signal; }
    notify<>& hup_signal() { return _idle_signal; }
    notify<>& error_signal() { return _idle_signal; }
    notify<>& zero_signal() { return _idle_signal; }
    ifd::iterator query_fd(int fd_);
    expect<> start();
    void err_hup(ifd& f);
    expect<> epoll_data_in(ifd& i);
    expect<> epoll_data_out(ifd& i);


private:

};

}

