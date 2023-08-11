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


#include "iolistener/listener.h"
#include <sys/socket.h>
#include <errno.h>
#include <error.h>

using namespace book;

namespace io
{




listener::listener(object *parent_, int msec_) : object(parent_, "listener"), msec(msec_)
{
    init();
}

listener::~listener()
{
    //shutdown();
    _idle_signal.disconnect_all();
    _hup_signal.disconnect_all();
    _zero_signal.disconnect_all();
    _error_signal.disconnect_all();

    _ifds.clear();
}

expect<> listener::run()
{

    rem::push_debug(HERE) << " _epoll_event.events:" << color::Yellow << "%08b" << _epoll_event.events << color::Reset << ":";
    if(!_epoll_event.events)
        return rem::push_info(HERE) << "events poll empty - dismissing this listener";

    auto num = _ifds.size();
    epoll_event events[_maxifd];
    int ev_count=0;

    ifd::iterator i;

    do{
        //rem::push_debug(HERE) << " epoll_wait:";
        ev_count = epoll_wait(_epollfd,events,num,msec);
        //rem::push_debug(HERE) << " epoll_wait:[" << color::Yellow << ev_count << color::Reset << "]:";

        if(!ev_count)
        {
            //rem::push_debug(HERE) << "Invoke _idle_signal(): " << color::Yellow << (_idle_signal.empty() ? "no hook..." : "");
            _idle_signal();
            continue;
        }

        for(int e=0; e < ev_count; e++)
        {
            uint32_t ev = events[e].events;
            int fd = events[e].data.fd;
            //rem::push_info(HERE) << rem::stamp <<  " event on fd " << color::Red4 << fd << color::Reset;
            auto i = query_fd(fd);
            if(i==_ifds.end())
            {
                rem::push_error(HERE) << " event triggered on descriptor which is not in this listener...";
                break;
            }
            expect<> R;
            if(ev & (EPOLLERR | EPOLLHUP))
            {
                err_hup(*i);
                continue;
            }
            if(ev & EPOLLOUT) {
                R = epoll_data_out(*i);
                ///@todo handle R;
                continue;
            }
            if (ev & EPOLLIN) {
                R = epoll_data_in(*i);
                if(!R)
                {
                    rem::push_status(HERE) << " epoll_data_in(" << color::Yellow << i->fd << color::Reset << ") breaks.";
                    continue;
                }
                if(*R == rem::end) shutdown();
                ///@todo handle R;
                continue;
            }
        }// epoll events iteration
    }while(!_terminate);
    rem::push_info(HERE) << color::PaleVioletRed1 << " exited from the main loop of the listener: ";
    return rem::ok;
}




expect<> listener::add_ifd(int fd_, uint32_t opt_)
{
    rem::push_debug(HERE) << " fd = " << color::Yellow << fd_;
    auto i = query_fd(fd_);
    if(i != _ifds.end())
        return rem::push_error(HERE) << " file descriptor" << fd_ << " already in the epoll set ";

    _ifds.emplace_back(fd_, opt_);

    epoll_event ev;
    ev.events = _epoll_event.events;
    auto &fd = _ifds.back();
    fd.state.active = true;
    ev.data.fd = fd.fd;
    epoll_ctl(_epollfd, EPOLL_CTL_ADD, fd.fd, &ev );
    rem::push_info(HERE) << " added ifd[fd=" << fd.fd << "]";
    return rem::ok;
}

expect<> listener::remove_ifd(int fd_)
{
    auto i = query_fd(fd_);
    if(i==_ifds.end())
        return rem::push_error(HERE) << " fd " << fd_ << " not in this epoll set";


    rem::push_info() << " removing ifd from the epoll set" << rem::endl << " fd:" << i->fd;

    auto fdi = i->fd;
    epoll_event ev;// prend pas de chance pour EPOLL_CTL_DEL - selon la doc, ev doit etre non-null dans la version 2.6.9- du kernel....
    // completement ignor&eacute; dans 2.6.9+
    ev.events = _epoll_event.events;
    ev.data.fd = i->fd;
    epoll_ctl(_epollfd, EPOLL_CTL_DEL, i->fd, &ev );
    _ifds.erase(i);
    rem::push_info(HERE) << " removed fd[" << fdi << "] from the epoll set, and destroyed.";
    return rem::ok;
}

expect<> listener::pause_ifd(int fd_)
{
    auto i = query_fd(fd_);
    if(i==_ifds.end())
        return rem::push_error(HERE) << " fd " << fd_ << " not in this epoll set";
    epoll_event ev;// prend pas de chance pour EPOLL_CTL_DEL - selon la doc, ev doit etre non-null dans la version 2.6.9- du kernel....
    // completement ignor&eacute; dans 2.6.9+
    ev.events = _epoll_event.events;
    ev.data.fd = i->fd;
    epoll_ctl(_epollfd, EPOLL_CTL_DEL, i->fd, &ev );
    rem::push_info(HERE) << " fd[" << fd_ <<"] is paused";
    return rem::ok;
}

expect<> listener::init()
{
    rem::push_debug(HERE) << ":";
    _terminate = false;
    _epollfd = epoll_create(_maxifd);
    _epoll_event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
    return rem::ok;
}


expect<> listener::shutdown()
{
    _terminate = true;
    //close/shutdown all ifd's
    for(auto &f : _ifds)
    {
        if(f.fd > 2) // NEVER-EVER shutdown STDIN, STDOUT, or STDERR !!! LOL
            ::shutdown(f.fd, SHUT_RDWR);
    }
    close(_epollfd);
    return rem::accepted;
}

ifd::iterator listener::query_fd(int fd_)
{
    for(auto it = _ifds.begin(); it != _ifds.end(); it++)
        if(it->fd == fd_) return it;
    return _ifds.end();
}



expect<> listener::start() {
//    if(_epollfd >=0 ){
//        rem::push_warning(HERE) << "this listener has already been initialized and is probably running as of now! - review this listener management!";
//        return false;
//    }
    return this->run();
}

expect<> listener::epoll_data_in(ifd &i)
{
    //log_debugfn << log_end;
    i.state.readable = true;
    i.state.writeable = false;
    expect<> E;
    if(i.state.active){
        //rem::push_debug(HERE) << " reading on fd " << i.fd;
        E = i.data_in();
        if(!E) {
            rem::push_status(HERE) << " rejected expect from the ifd[" << i.fd << "]; - returning;";
            return E;
        }
    }

    // re-enable listening:
    epoll_event e;
    e.data.fd = i.fd;
    e.events = (i.options & ifd::O_READ) ? EPOLLIN: 0 | (i.options & ifd::O_WRITE) ? EPOLLOUT: 0 |EPOLLERR | EPOLLHUP;
    epoll_ctl(_epollfd, EPOLL_CTL_MOD, i.fd,&e);
    return E;
}

expect<> listener::epoll_data_out(ifd &i)
{
    if(!(i.options & ifd::O_WRITE))
        return rem::rejected;

    i.state.readable = false;
    i.state.writeable = true;
    expect<> E;
    if(i.state.active){
        //rem::push_debug(HERE) << " writting on fd " << i.fd;
        E = i.write_signal(i);
    }
    epoll_event e;
    e.data.fd = i.fd;
    e.events = (i.options & ifd::O_READ) ? EPOLLIN: 0 | (i.options & ifd::O_WRITE) ? EPOLLOUT: 0 |EPOLLERR | EPOLLHUP;
    epoll_ctl(_epollfd, EPOLL_CTL_MOD, i.fd,&e);
    return E;
}

void listener::err_hup(ifd &f)
{
    rem::push_error(HERE) << color::White << " fd[" << color::Yellow << f.fd << color::White << "] error or hangup." << rem::endl
        << " removing file descriptor";
    remove_ifd(f.fd);
}


}

