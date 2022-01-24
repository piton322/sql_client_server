#ifndef _DBMS_HPP_
#define _DBMS_HPP_
#include <vector>
#include <string>
#include <map>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#define MAX 256
using namespace std;
// за основу взята оболочка dbms.h из методички с заданием 
//(убраны некотые абстрактные классы и виртуальные функции, добавлены вспомогательные функции)
enum Type { TEXT, LONG }; // перечисление вынесено вне классы в отличие от шаблона из методички
// класс, который будет использоваться для вывода различных сообщений об ошибках
class Xception {
public:
    string Message;
    virtual void PrintMessage ()
    {
        cerr << Message << endl;
    }
    Xception () {}
    Xception ( const string& aMessage ) {
        Message = aMessage; 
    }
    Xception ( const Xception& xception ) { 
        Message = xception.Message; 
    }
    virtual ~ Xception () {}
};
// абстрактный класс для полей таблицы
class IField {
public:
    virtual Type OfType () = 0;
    virtual string & Text () = 0;
    virtual long & Long () = 0;
    virtual ~IField(){}
};
// класс с текстовым полем
class ITextField: public IField {
    string text;
public:
    long & Long () { 
        throw Xception("Error with long"); 
    }
    ITextField(const string & Text)
    {
        text = Text;
    }
    ITextField()
    {
        text = "";
    }
    virtual Type OfType ()
    {
        enum Type f_type = TEXT;
        return f_type;
    }
    virtual string & Text ()
    {
        return text;
    }
    ~ITextField()
    {
        text.clear();
    }
};
// класс с полем типа long
class ILongField: public IField {
    long value;
public:
    string & Text () { 
        throw Xception("Error with Text"); 
    }
    ILongField(long Value = 0)
    {
        value = Value;
    }
    virtual Type OfType ()
    {
        enum Type f_type = LONG;
        return f_type;
    }
    virtual long & Long ()
    {
        return value;
    }
    ~ILongField(){}
};
// структура для хранения некоторой информации о полях таблицы
struct TableContent
{
    //char name[MAX];
    string name;
    int size;
    enum Type type;
    TableContent operator=(TableContent x)
    {
        name = x.name;
        size = x.size;
        type = x.type;
        return *this;
    }
};
// структура таблицы, формирование ее полей
class ITableStruct {
public:
    vector<struct TableContent> list; // массив полей таблицы
    string name; // название таблицы
    ITableStruct()
    {
        list.clear();
        name = "";
    }
    ITableStruct * AddText (const string & n, int l)
    {
        struct TableContent tmp;
        tmp.name = n;
        tmp.type = TEXT;
        tmp.size = l;
        list.push_back(tmp);
        return this;
    }
    ITableStruct * AddLong (const string & n)
    {
        struct TableContent tmp;
        tmp.name = n;        
        tmp.type = LONG;
        tmp.size = sizeof(long);
        list.push_back(tmp);
        return this;
    }
    ITableStruct * SetName (const string & n)
    {
        list.clear();
        name = n;
        return this;
    }
    vector<struct TableContent> & GetList()
    {
        return list;
    }
    string & GetName()
    {
        return name;
    }
    ~ITableStruct()
    {
        list.clear();
        name.clear();
    }
    //virtual ~ITableStruct() {}
};
// вспомогательная информация о таблице
struct AboutTable
{
    long amount_columns; // число столбцов
    long first_record; // первая запись
    long last_record; // последняя запись
    long current_record; // текущая запись
    long size; // количество записей
};

class ITable {
    string name; // название таблицы
    vector<struct TableContent> columns; // массив содержимого столбцов
    int fd; // для работы с файлом
    struct AboutTable info; // информация о таблице
    vector <IField *> tabl; // массив полей
    bool flag; // флаг: true, если читали, иначе false
private:    
    ITable(int d, vector<struct TableContent> & list, string n, struct AboutTable inf)
    {
        flag = false; // не читали
        name = n;
        columns.clear();
        for (auto i: list)
        {
            columns.push_back(i); 
        }
        info = inf;
        fd = d;
        tabl.clear();
        for (auto j: columns)
        {
            if(j.type == TEXT)
            {
                IField * new_field = new ITextField("");
                tabl.push_back(new_field); //заполняем поля пустым текстом
            
            }
            else
            {
                IField * new_field = new ILongField(0);
                tabl.push_back(new_field); //заполняем поля нулями
            }
        }
        lseek(fd, info.first_record, SEEK_SET); //устанавливаем курсор на первую запись таблицы
    }
    void Update()
    {
        lseek(fd, 0L, SEEK_SET); //пытаемся записать в начало инофрмацию о таблице
        if (write(fd, &info, sizeof(info)) < 0)
        {
            throw Xception("Error with update information about table " + name);
        } 
        lseek(fd, info.current_record, SEEK_SET); //возвращаем курсор
    }
    string GetName()
    {
        return name;
    }  
public:
    static ITableStruct * CreateTableStruct ()
    {   //Для формирования новой таблицы необходимо задать структуру будущей таблицы, используя интерфейс ITableStruct. 
        ITableStruct * Struct = new ITableStruct();
        return Struct;
    }
    static ITable * Create (ITableStruct * tablestruct) // создание таблицы, основываясь на структуре
    {
        int fd;
        fd = open(tablestruct->GetName().c_str(), O_CREAT | O_RDWR | O_EXCL, 0666); // пытаемся создать файл файл с нужным названием
        if (fd == -1)
        {
            throw Xception ("Error with opening file " + tablestruct->GetName());
        }
        struct AboutTable about = {0};
        about.amount_columns = tablestruct->GetList().size(); // заполняем информацию о таблцие (число столбцов)
        long r_size = 0;
        if (write(fd, &about, sizeof(about)) < 0) // пытаемся записать информацию о таблице в файл
        {
            throw Xception("Error with adding info about table " + tablestruct->GetName());
        }
        for (auto i: tablestruct->GetList()) 
        {
            if (write(fd, &i, sizeof(i)) < 0)
            {
                throw Xception("Error with adding names to table " + tablestruct->GetName());
            }
            r_size += i.size; // считаем размер записи
        }
        about.first_record = lseek(fd, 0L, SEEK_CUR); // заполняем оставушуюся информацию о таблице
        about.last_record = lseek(fd, 0L, SEEK_CUR);
        about.current_record = lseek(fd, 0L, SEEK_CUR);
        about.size = r_size;
        lseek(fd, 0L, SEEK_SET); // курсор на начало
        if (write(fd, &about, sizeof(about)) < 0) // пытаемся записать информацию о таблице в файл
        {
            throw Xception("Error with adding info about table " + tablestruct->GetName());
        }
        lseek(fd, about.first_record, SEEK_SET);
        ITable * Table = new ITable(fd, tablestruct->GetList(), tablestruct->GetName(), about);
        return Table;
    }    

    static ITable * Open (const string & nam)
    {
        int fd;
        vector<struct TableContent> list;
        struct TableContent tabl;
        struct AboutTable about;
        fd = open(nam.data(), O_RDWR); // открываем на чтение и запись
        if (fd == -1)
        {
            throw Xception("Error with opening file " + nam);
        }
        if (read(fd, &about, sizeof(about)) < 0) // пытаемся прочитать информацию о таблице
        {
            throw Xception("Error with reading file " + nam);
        }
        for(int i = 0; i < about.amount_columns; i++)
        {
            if (read(fd, &tabl, sizeof(tabl)) < 0)
            {
                throw Xception("Error with reading names from table " + nam);
            }
            list.push_back(tabl); // заполняем поля
        }
        about.current_record = lseek(fd, 0L, SEEK_CUR); // запоминаем, где находится курсор в текущей записи
        ITable * Table = new ITable(fd, list, nam, about);
        return Table;;           
    }
    void Add()
    {
        int k = 0;
        for(auto i: tabl)
        {
            if (i->OfType() == TEXT)
            {   
                if (i->Text().size() > columns[k].size)
                {
                    throw Xception("Error with size " + name);
                }
                if (write(fd, i->Text().data(), columns[k].size) < 0)
                {
                    throw Xception("Error with write to table " + name);
                }             
            }
            else
            {
                if (write(fd, &(i->Long()), sizeof(long)) < 0)
                {
                    throw Xception("Error with write to table " + name);
                }
            }
            k++;
        }
        lseek(fd, info.current_record, SEEK_SET); //устанавливаем курсор на текущую запись таблицы
        if (info.current_record == info.last_record)
        {
            info.last_record = lseek(fd, info.size, SEEK_CUR);  
            Update();
        }
    }  
    
    void Delete ()
    {
        if (info.current_record != info.last_record && ((info.current_record + info.size) == info.last_record))
        {   //устанавливаем длину файлового дескриптора fd в info.current_record байт.
            ftruncate(fd, info.current_record);
            info.last_record = info.current_record; // последняя запись - текущая
            Update(); // обновляем информацию о таблице
        }
        else if (info.current_record != info.last_record)
        {
            long buf = info.current_record; //запоминаем текущую запись
            for (; !(info.current_record == info.last_record); ReadNext()) // идем до последней записи, читаем по одной
            {
                ReadNext();
                GetField(columns[0].name); 
                if (info.current_record == info.first_record)
                {
                    throw Xception("Error: previous record does not exist, table " + name);
                }
                lseek(fd, - info.size, SEEK_CUR); // читаем назад
                info.current_record = lseek(fd, 0L, SEEK_CUR); // обновляем информацию
                flag = false;
                int k = 0;
                for(auto i: tabl)   
                {
                    if (i->OfType() == TEXT)
                    {   
                        if (write(fd, i->Text().data(), columns[k].size) < 0)
                        {
                            throw Xception("Error with write to table " + name);
                        }             
                    }
                    else
                    {
                        if (write(fd, &(i->Long()), sizeof(long)) < 0)
                        {
                            throw Xception("Error with write to table " + name);
                        }
                    }
                    k++;
                }
                lseek(fd, info.current_record, SEEK_SET); //устанавливаем курсор на текущую запись таблицы
            }   
            if (info.current_record == info.first_record)
            {
                throw Xception("Error: previous record does not exist, table " + name);
            }
            lseek(fd, - info.size, SEEK_CUR); // читаем назад
            info.current_record = lseek(fd, 0L, SEEK_CUR); // обновляем информацию
            flag = false;
            info.last_record = info.current_record;
            ftruncate(fd, info.last_record);
            info.current_record = buf;
            Update();
        }
    }
    static void Drop ( const string & Name )
    {
        remove(Name.c_str());
    }
    IField * GetField (const string & n)
    {
        if (flag == false)
        {
            flag = true; // флаг чтения
            int k = 0;
            for (auto j: columns)
            {
                if (j.type == TEXT)
                {
                    char buf[MAX] = {0};
                    if (read(fd, buf, j.size) < 0)
                    {
                        throw Xception("Error with read from table " + name);   
                    }
                    tabl[k]->Text() = buf;
                }
                else
                {
                    if (read(fd, &(tabl[k]->Long()), sizeof(long)) < 0)
                    {
                        throw Xception("Error with read from table " + name);   
                    }
                }
                k++;
            }
            lseek(fd, info.current_record, SEEK_SET); //устанавливаем курсор на текущую запись таблицы
        }
        int p = 0;
        for(auto i: columns) // пытаемся найти поле n
        {
            if (i.name == n)
            {
                return tabl[p];
            }
            p++;
        }
        throw Xception("Error: record " + n + " does not exist " + " in table: " + name);
    }
    IField * GetField (int n)
    {
        if (flag == false)
        {
            flag = true; // флаг чтения
            int k = 0;
            for (auto i: columns)
            {
                if (i.type == TEXT)
                {
                    char buf[MAX] = {0};
                    if (read(fd, buf, i.size) < 0)
                    {
                        throw Xception("Error with read from table " + name);   
                    }
                    tabl[k]->Text() = buf;
                }
                else
                {
                    if (read(fd, &(tabl[k]->Long()), sizeof(long)) < 0)
                    {
                        throw Xception("Error with read from table " + name);   
                    }
                }
                k++;
            }
            lseek(fd, info.current_record, SEEK_SET); //устанавливаем курсор на текущую запись таблицы
        }
        if (n > tabl.size()) // пытаемся найти поле с номером n
        {
            throw Xception("Error: record " + to_string(n) + " does not exist " + " in table: " + name);
        }
        return tabl[n];
    }
    void ReadFirst()
    {
        flag = false;
        lseek(fd, info.first_record, SEEK_SET); // ищем первую запись
        info.current_record = lseek(fd, 0L, SEEK_CUR); // обновляем информацию
    }
    void ReadNext()
    {
        if (info.current_record == info.last_record)
        {
            throw Xception("Error: next record does not exist, table " + name);
        }
        lseek(fd, info.size, SEEK_CUR); // читаем вперед
        info.current_record = lseek(fd, 0L, SEEK_CUR); // обновляем информацию
        flag = false;
    }
    // далее идут вспомогательные методы, которые понадобились в ходе решения задачи
    void SetAboutTable(const AboutTable & inf)
    {
        info = inf;
    }
    void SetFlag(const bool & fl)
    {
        flag = fl;
    }
    AboutTable GetAboutTable()
    {
        return info;
    }
    int GetFd()
    {
        return fd;
    }
    vector<struct TableContent> GetColumns()
    {
        return columns;
    }   
    // деструктрор, удаляем все, что надобавляли
    ~ITable()
    {
        for (auto i: tabl)
        {
            delete i;
        }
        columns.clear();
        tabl.clear();
        close(fd);
    } 
};

#endif