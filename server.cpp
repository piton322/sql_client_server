#include <iostream>
#include "sock_wrap.hpp"
#include "dbms.hpp"
#include "work.hpp"
using namespace std;
using namespace ModelSQL;

const char * address = "mysocket"; // имя сокета
class MyServerSocket : public UnServerSocket 
{
    public: 
        MyServerSocket () : UnServerSocket (address) {}
    protected:
        void OnAccept (BaseSocket * pConn)
        { 
            pConn->PutString("Введите запрос "); // просьба ввести запрос отобразится у клиента
            try
            {
                while (true)
                {
                    string req = pConn->GetString(); //  получаем сообщение от клиента
                    Req(req); // обрабатываем запрос
                    pConn->PutString(message_server); 
                    pConn->PutString("Введите запрос ");
                } 
            }
            catch(...)
            {
                cout << "Something wrong" << endl;
            }
            delete pConn;
        }
}; 
int main(int argc, char* argv[])
{
    try 
    {
        MyServerSocket sock; // создаем сокет 
        sock.Accept(); // слушаем запросы на соединение
    } 
    catch (Exception& e) 
    {
        e.Report();
    }
    return 0;
}