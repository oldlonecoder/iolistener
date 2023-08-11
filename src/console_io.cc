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

#include "iolistener/console_io.h"
#include <sys/ioctl.h>
#include <unistd.h>



namespace io
{



/*!
 * \brief console_io::key_in read the console input buffer in raw mode.
 * \param fd reference to the instance of ifd this console_io is notified.
 *
 * \note if input pksize > 6 then there is buffer overflow - bail and reject.
 *
 * \return instance of expect<>: rem::rejected to stop this thread io_loop, or rem::accepted to let the io_loop continue to wait/read keys input
 * \author &copy;2023, oldlonecoder/Serge Lussier  (serge.lussier@oldlonecoder.club)
 */
expect<> console_io::key_in(ifd &fd)
{

// 1: check packet size ( p > 0, p <= 6 )
    if(fd.pksize > 6)
    {
        return rem::push_except(HEREF) << rem::rejected << "  - Unexpected packet size larger than 6 bytes.";
    }

    //let's do some debugs:
    stracc msg = "input ";
    msg << color::Yellow , fd.pksize , color::Reset , " bytes in]";

    //format :
    const char* fmt = "|0x%02X";
    if((fd.pksize == 1) && (fd.internal_buffer[0] == 27))
    {
        rem::push_message(HERE) << color::White << "[ESC] " << color::Reset <<"pressed - Terminating the console_io loop!";
        return rem::end;
    }

    // Get the key sequence data:
    char keyseq[10]={}; // I think the compiler fills in with zero's
    std::size_t x = 0;
    for(; x< fd.pksize; x++)
    {
        char c= *(fd.internal_buffer + x);
        msg << fmt << c;
        keyseq[x] = c;
    }
    keyseq[fd.pksize] = 0; // Never know if keyseq[]={} does not fill with 0's...
    auto e = kbhit_notify(keyseq);
    // ...

    rem::push_info(HERE) << " seq:[" << color::Yellow << msg << color::Reset << ']';
    return rem::accepted;
}

expect<> console_io::idle()
{
    return _idle_signal();
}

console_io::console_io(): object(nullptr, "console_io") { }

console_io::console_io(object *parent_obj):object(parent_obj,"console_io") { }

console_io::~console_io()
{
    //???
}


/*!
 * \brief console_io::begin console_io initializing routine.
 *
 *     Creates the input-loop thread.
 *
 * \return
 */
rem::code console_io::start()
{
    struct winsize win;


    ioctl(fileno(stdout), TIOCGWINSZ, &win);
    tcgetattr(STDIN_FILENO, &con);
    raw = con;
    raw.c_iflag &= ~( BRKINT | PARMRK |ISTRIP);
    raw.c_iflag &= ~(PARMRK);
    raw.c_cflag |= (CS8|IGNBRK);
    raw.c_lflag &= ~(ICANON | ECHO | IEXTEN | TOSTOP);

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    raw.c_cc[VSTART] = 0;
    raw.c_cc[VSTOP] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        //... To be continued

    io_listener = listener(this, 1000);
    (void)io_listener.add_ifd(STDIN_FILENO, ifd::O_READ| ifd::I_AUTOFILL);
    auto i = io_listener.query_fd(STDIN_FILENO);
    i->read_signal.connect(this, &console_io::key_in);
    io_listener.idle_signal().connect(this, &console_io::idle);
    rem::push_info(HERE) << color::DarkGreen << "starting the io loop thread :";
    io_thread = std::thread([this](){
        auto e = io_listener.run();
        //... dependant du code de retour ...

        // ...
    });

    return rem::ok;
}

rem::code console_io::fin()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &con);
    return rem::ok;
}



}