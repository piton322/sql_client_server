#ifndef __SOCK_WRAP_HPP__
#define __SOCK_WRAP_HPP__
#include <string>
#include <vector>
#include <iostream>
#include <cstdio>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#define BACK_LOG 3 //кол-во запросов в очереди к серверу 
#define MAXL 1000

using namespace std;
//за основу sql взята оболчка Modelsql из методички
namespace ModelSQL 
{
    // Exception --- класс исключений, генерируемых библиотекой
    class Exception {
    protected: 
        int m_ErrCode;
    public: 
        Exception (int errcode ) : m_ErrCode(errcode) {} 
        void Report ()
        {
            cout << "Error " << m_ErrCode << endl;
        };
    };
    // SocketException --- класс исключений
    class SocketException : public Exception 
    {
        static char * m_Message[];
        public:
            enum SocketExceptionCode 
            { 
                ESE_SUCCESS, ESE_SOCKCREATE, ESE_SOCKCONN, ESE_SOCKBIND, 
                ESE_SOCKRECV, ESE_SOCKILLEGAL, 
                ESE_SOCKHOSTNAME, ESE_SOCKSEND, ESE_SOCKLISTEN, ESE_SOCKACCEPT,
            };
        SocketException (SocketExceptionCode errcode): Exception(errcode){}
    };
    // SocketAddress --- базовый абстрактный класс для представления
    // сетевых адресов
    class SocketAddress 
    {
        protected:
            struct sockaddr_un * m_pAddr;
            int len;
        public:
            SocketAddress (): m_pAddr(NULL) {}
        virtual ~ SocketAddress () {} 
        virtual int GetLength() = 0;
        virtual SocketAddress * Clone() = 0;
        operator struct sockaddr * ()
        {
            return (struct sockaddr *) m_pAddr;
        }
    };
    // UnSocketAddress --- представление адреса семейства AF_UNIX 
    class UnSocketAddress : public SocketAddress 
    {
        public:
            UnSocketAddress (const char * SockName)
            {
                // адресс напрямую
                struct sockaddr_un * sa = new sockaddr_un();
                sa->sun_family = AF_UNIX;
                strcpy (sa->sun_path, SockName); 
                len = sizeof ( sa->sun_family) + strlen (sa->sun_path);
                m_pAddr = sa;
            }
            UnSocketAddress (const UnSocketAddress & x)
            {
                // копирование адресса
                m_pAddr = new sockaddr_un();
                m_pAddr->sun_family = AF_UNIX;
                strcpy (m_pAddr->sun_path, x.m_pAddr->sun_path); //?
            }
            //дестркутор
            ~ UnSocketAddress ()
            {
                delete m_pAddr;
            }
            // получение длины структуры сокета
            int GetLength ()
            {
                return sizeof(*m_pAddr);
            }
            // клонирование адреса
            SocketAddress * Clone()
            {
                UnSocketAddress * tmp = new UnSocketAddress(*this);
                return tmp;
            }
    };

// BaseSocket --- базовый класс для сокетов
    class BaseSocket {
    public:
        explicit BaseSocket (int sd = -1, SocketAddress * pAddr = NULL): m_Socket(sd), m_pAddr(pAddr) {}
        void PutString(const char * str) // пытаемся отправить одну строку
        {
            if (send(m_Socket, str, strlen(str), 0) < 0)
            {
                perror(0);
                throw SocketException(SocketException::ESE_SOCKSEND);
            }
            
        }
        void PutString(const std::string& s)
        {
            if (send(m_Socket, s.data(), s.size(), 0) < 0)
            {
                perror(0);
                throw SocketException(SocketException::ESE_SOCKSEND);
            }
        } 
        std::string GetString()
        {
            std::vector<char> buffer(MAXL, 0);
            std::string rcv;   
            int bytesReceived = 0;
            bytesReceived = recv(m_Socket, &buffer[0], buffer.size(), 0);
            if ( bytesReceived == -1 ) 
            { 
                throw SocketException(SocketException::ESE_SOCKRECV);
            } 
            else 
            {
                rcv.append( buffer.cbegin(), buffer.cend() );
            }
            return rcv;
        }
        // получение дескриптора
        int GetSockDescriptor()
        {
            return m_Socket;
        }
        // деструктор
        virtual ~ BaseSocket()
        {
            close(m_Socket);
            delete m_pAddr;
        }
    protected:
        int m_Socket;
        SocketAddress * m_pAddr;
    };

    // ClientSocket --- базовый класс для клиентских сокетов
    class ClientSocket: public BaseSocket {
    public:
        void Connect()
        {
            socklen_t len = m_pAddr->GetLength();
            if ( connect( m_Socket, (struct sockaddr *) *m_pAddr, len) < 0 ) 
            {
                perror(0);
                throw SocketException(SocketException::ESE_SOCKCONN);
            }
        }
    };

    // ServerSocket --- базовый класс для серверных сокетов
    class ServerSocket: public BaseSocket {
    public:
        BaseSocket * Accept()
        {
            socklen_t len = m_pAddr->GetLength();
            int d;
            if((d = accept ( m_Socket, (struct sockaddr *) *m_pAddr, &len)) < 0){
                perror (0);
                throw SocketException(SocketException::ESE_SOCKACCEPT);
            }
            BaseSocket * s = new BaseSocket(d, m_pAddr->Clone());
            OnAccept(s); 
            return s;

        }
    protected:
        void Bind()
        {
            unlink("mysocket");
            int a = ::bind(  m_Socket, (struct sockaddr *) *m_pAddr, m_pAddr->GetLength());
            if ( a < 0 ){
                perror(0);
                throw SocketException(SocketException::ESE_SOCKBIND);
            }       
        }
        void Listen(int BackLog)
        {
            if ( listen ( m_Socket, BackLog) < 0 ) {
                perror(0);
                throw SocketException(SocketException::ESE_SOCKLISTEN);
            }
        }
        virtual void OnAccept (BaseSocket * pConn) {}
    };

    // UnClientSocket --- представление клиентского сокета семейства
    // AF_UNIX
    class UnClientSocket: public ClientSocket {
    public:
        UnClientSocket(const char * SockName)
        {
            m_pAddr = new UnSocketAddress(SockName);
            if ((m_Socket = socket (AF_UNIX, SOCK_STREAM, 0)) < 0) {
                throw SocketException(SocketException::ESE_SOCKCREATE);
            }
        }
    };

    // UnServerSocket --- представление серверного сокета семейства
    // AF_UNIX
    class UnServerSocket: public ServerSocket {
    public:
        UnServerSocket(const char * Address)
        {
            m_pAddr = new UnSocketAddress(Address);
            if ((m_Socket = socket (AF_UNIX, SOCK_STREAM, 0)) < 0) {
                throw SocketException(SocketException::ESE_SOCKCREATE);
            }
            Bind();
            Listen(BACK_LOG);
        }
    };
}; // конец namespace ModelSQL
#endif