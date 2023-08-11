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

#include <unordered_map>
#include <thread>
#include <mutex>
#include "iolistener/listener.h"
#include <termios.h>
#include <unistd.h>

/* Pour platforme universelle :

struct IO_PUBLIC key_data
{
    using stack = std::vector<key_data>;
    enum enumerated
    {
        A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Z,
        K1,K2,K3,K4,K5,K6,K7,K8,K9,K0,
        F1,F2,F3,V4,V5,F6,F7,F8,F9,F10,F11,F12,
        TAB,BACK,SPACE,BSLASH,FSLASH,ENTER,PIPE,INX,HOME,PGUP,PGDN,END,DEL,
        ARLEFT,ARRIGHT,ARUP,ARDOWN,MINUS,EQ,SQUOTE,DQUOTE,SEMICOLON,COLON,
        ESC,PPLUS,PENTER,DOT,COMA,LESSER,GREATER,QUESTION,
        EXCLA,AROBAS,DOLLARD,PERCENT,CARRET,AMP,STAR,PARLEFT,PARRIGHT,PLUS,UNDER
    };
    enum state : uint8_t {
        CTRL=1,
        WIN =2,
        SHIFT=3,
        ALT=4
    };
    struct IO_PUBLIC data
    {
        char name[25];          ///< key's name  ex.: "CTRL+1"
        key_data::enumerated e; ///< key mnemonic
        key_data::state st;

    };
    char seq[10]{}; ///< zero-terminated bytes array

};


*/
using book::notify;
using book::expect;
using book::rem;

namespace io {

/*!
 * \brief The console_io class
 * ...
 */
class  console_io : public book::object
{
    // ------------- io thread -------------------------
    // delegate/slot:
    std::mutex inmtx{};
    std::thread io_thread;

    expect<> key_in(ifd& fd);
    expect<> idle();
    termios raw;
    termios con;
    console_io* _self;
    listener io_listener{};
    notify<> _idle_signal{"oldcc::io idle notifier"};
    notify<char*> kbhit_notifier;

public:

    console_io();
    console_io(object* parent_obj);
    ~console_io();
    std::thread& thread_id() { return io_thread;}
    rem::code start();
    rem::code fin();
    void operator()(); ///< Callable object as Set as the (std::)thread starter;
    notify<>& idle_notifier() { return _idle_signal; }
    notify<char*>& kbhit_notify(char* seq) { return kbhit_notifier; }

};


}

