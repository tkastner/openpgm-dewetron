/* vim:ts=8:sts=8:sw=4:noai:noexpandtab
 *
 * Copyright (c) DEWETRON GmbH 2017
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#pragma once

int(*priv_sendto)(SOCKET, const char*, int, int, const struct sockaddr*, int);
int(*priv_recvfrom)(SOCKET, char*, int, int, const struct sockaddr*, int);

static int empty_sendto(SOCKET s, const char * buf, int len, int flags, const struct sockaddr *to, int tolen)
{
    return 1;
}

static int empty_recvfrom(SOCKET s, char * buf, int len, int flags, struct sockaddr * from, int * fromlen)
{
    return 1;
}

#ifdef _WIN32
    int wsa_sendto(SOCKET s, const char * buf, int len, int flags, const struct sockaddr *to, int tolen)
    {
        WSABUF send_buf;
        DWORD bytes_sent = 0;
        int err = 0;

        send_buf.buf = (char*)buf;
        send_buf.len = len;
        int rc = WSASendTo(s, &send_buf, 1, &bytes_sent, flags, to, tolen, NULL, NULL);

        return bytes_sent;
    }

    static int wsa_recvfrom(SOCKET s, char * buf, int len, int flags, struct sockaddr * from, int * fromlen)
    {
        int err = 0;
        WSABUF rcv_buf;
        rcv_buf.buf = buf;
        rcv_buf.len = len;

        DWORD bytes_received = 0;
        DWORD lpflags = flags;

        int rc = WSARecvFrom(s, &rcv_buf, 1, &bytes_received, &lpflags, from, fromlen, NULL, NULL);

        return bytes_received;
    }


    static int default_sendto(SOCKET s, const char * buf, int len, int flags, const struct sockaddr *to, int tolen)
    {
        return sendto(s, buf, len, flags, to, tolen);
    }

    static int default_recvfrom(SOCKET s, char * buf, int len, int flags, struct sockaddr * from, int * fromlen)
    {
        return recvfrom(s, buf, len, flags, from, fromlen);
    }
#else // NOT _WIN32

    static int default_sendto(SOCKET s, const char * buf, int len, int flags, const struct sockaddr *to, int tolen)
    {
        return sendto(s, buf, len, flags, to, tolen);
    }

    static int default_recvfrom( SOCKET s, char * buf, int len, int flags, struct sockaddr * from, int * fromlen)
    {
        return recvfrom(s, buf, len, flags, from, (socklen_t*)&fromlen);
    }
#endif  // NOT WIN32

void pgm_send_recv_init()
{
    priv_sendto = &default_sendto;
    priv_recvfrom = &default_recvfrom;

    char* pgm_send;
    size_t envlen;

    const errno_t err = pgm_dupenv_s (&pgm_send, &envlen, "PGM_SEND");
    if (0 == err && envlen > 0) 
    {
        switch (pgm_send[0])
        {
#ifdef _WIN32
        case 'W':
        {
            priv_sendto = &wsa_sendto;
            priv_recvfrom = &wsa_recvfrom;
            pgm_trace(PGM_LOG_LEVEL_TRACE, "PGM_SEND: WSA");
            break;
        }
#endif
        case 'N':
        {
            priv_sendto = &wsa_sendto;
            priv_recvfrom = &wsa_recvfrom;
            pgm_trace(PGM_LOG_LEVEL_TRACE, "PGM_SEND: NONE");
            break;
        }
        default:
            pgm_trace(PGM_LOG_LEVEL_TRACE, "PGM_SEND: DEFAULT");
        }
    }
    pgm_free (pgm_send);
}
