#include <iostream>
#include "sock_wrap.hpp" // см. приложение
using namespace std;
using namespace ModelSQL;
const char * address = "mysocket"; // имя сокета
int main(int argc, char* argv[])
{
    try 
    {
        // создаём сокет
        UnClientSocket sock (address);
        // устанавливаем соединение
        sock.Connect();
        while(true)
        {
            std::string s;
            s = sock.GetString(); // получаем строку от сервера
            cout << s << endl; // выводим ее 
            s.clear(); //  очищаем
            std::getline(std::cin, s);
            sock.PutString(s); // отправляем запрос серверу
            s.clear(); // очищаем
        }
    } 
    catch (Exception & e) 
    {
        // ошибка --- выводим текст сообщения на экран
        e.Report();
    }
 return 0;
}