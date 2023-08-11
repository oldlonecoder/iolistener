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
#include "iolistener/public.h"
#include <logbook/notify.h>
#include <cstdint>
#include <vector>
#include <iostream>
#include <unistd.h>


namespace io
{



struct ifd final
{

public:
    using list = std::vector<ifd>;
    using iterator = ifd::list::iterator;

    book::notify<ifd&>
        read_signal,
        write_signal,
        idle_signal,
        zero_signal,
        window_complete_signal;

    // Option flags - yes static constexpr:
    static constexpr uint32_t O_READ  = 0x01; ///< readeable
    static constexpr uint32_t O_WRITE = 0x02; ///< writeable
    static constexpr uint32_t P_RDWR  = 0x03; ///< same as (I_READ + I_WRITE);
    static constexpr uint32_t O_BLOCK = 0x04; ///< fills internal or external buffer bytes before signaling dataready to delegates ( I_BUF option automaticaly set )
    static constexpr uint32_t O_BUF   = 0x08; ///< Uses internal buffer : size determined with provided buffer size req.
    static constexpr uint32_t O_XBUF  = 0x10; ///< sets internal buffer ptr to external address - do not auto-delete
    static constexpr uint32_t O_IMM   = 0x20; ///< Notify to delegates immediately when a read is made.
    static constexpr uint32_t O_WINDOWED = 0x40; ///< Wait/Window size to be received/sent/written (from internal automatic buffer/ or external temp file) enabled. ifd::signal_t emitted only when window filled/flushed @note anything past m_wsize is discarded/ignored
    static constexpr uint32_t I_AUTOFILL = 0x80; ///< Auto-fill internal/or external buffer before sending read or write signal. So the triggered read and write are done after the data bloc is read or written.
    int fd = -1;
    std::size_t max_pksize = 1024 * 1024; ///< 1 megabytes by default. You have to set this value to your own limits for what you think is secure.
    // For example, keyboard input would never-ever send more than 8 bytes into the input stream at once.
    // So if you get more than 7 bytes it means something wrong is happening from the tty/pty/stdin stream.
    uint32_t options;   ///< see option flags
    std::size_t pksize;    ///< current packet size toread.
    std::size_t wsize;     ///< Wait/Windodw size
    std::size_t wpos;      ///< Wait/Window index where wpos >= 0 < wsize. | wpos == wsize-1 => Wait/Window:
                           ///  receive/send/write complete.
    u_int8_t* internal_buffer;
    struct state_flags
    {
        uint8_t active  :1;    ///< This descriptor is active or not
        uint8_t destroy :1;    ///< this descriptor is marked to be deleted
        uint8_t writeable:1;   ///< this descriptor's fd is ready for write ( socketfd write ready event from epoll_wait )
        uint8_t readable:1;    ///< this descriptor's fd is ready for read ( socketfd read ready event from epoll_wait )
    }state = {0,0,0,0};

    ifd();
    ifd(int fd_, uint32_t options_);
    ifd(ifd&&) noexcept;
    ifd(const ifd&) = default;
    ifd& operator=(ifd&&) noexcept;
    ifd& operator=(const ifd&) = default;

    book::expect<std::size_t> set_window_size(uint32_t sz);
    book::expect<> data_in();
    uint32_t set_options(u_int32_t opt);
    std::size_t toread();
    book::expect<> clear();
    book::expect<std::size_t> out(uint8_t* datablock, std::size_t sz, bool wait_completed=true) const;

    ~ifd();

};

}



