#ifndef __WORK_HPP__
#define __WORK_HPP__
#include "sock_wrap.hpp"
#include <vector>
#include <iostream>
#include <cstdio>
#include <memory>
#include <stack>
#include <string>
#include <regex>
#include "dbms.hpp"
#include "str_switch.hpp"
using namespace std;

enum lex_type_t { IDN, ENT, NUMBER, COMMA, ALL, OPEN, CLOSE,  END, 
                CREATE, INSERT, DELETE, DROP, SELECT, UPDATE, WHERE, WHERE_ALL, SET, 
                FROM, INTO, LIKE, IN, TABLE, TEX, LON, AND, OR , NOT, EXIT, 
                PEER, ADD, SUB, DIV, MOD, LESS_PEER, LESS, LARGER_PEER, LARGER, NOT_PEER, BOOL};
enum state_t {H, A, B, C, D, E, F,  G, I, J, K, L, M, N, O, P, OK};
enum lex_type_t cur_lex_type;
std::string cur_lex_text, message_server;
string request;
string indent = "    ";
int c;
int numb;
vector<string> vec; // вектор для хранения выражения 
vector<lex_type_t> t_vec; // вектор для хранения типов, которые есть внутри выражения
lex_type_t WordToType(string & s)
{
    // используем switch для строки из str_switch.hpp
    SWITCH(s)
    {
        CASE("CREATE"):
            return CREATE;
            break;
        CASE("INSERT"):
            return INSERT;
        CASE("DELETE"):
            return DELETE;
        CASE("DROP"):
            return DROP;
        CASE("UPDATE"):
            return UPDATE;
        CASE("SELECT"):
            return SELECT;
        CASE("WHERE"):
            return WHERE;
        CASE("ALL"):
            return WHERE_ALL;
        CASE("SET"):
            return SET;
        CASE("FROM"):
            return FROM;
        CASE("LIKE"):
            return LIKE;
        CASE("INTO"):
            return INTO;
        CASE("TABLE"):
            return TABLE;
        CASE("IN"):
            return IN;
        CASE("LONG"):
            return LON;
        CASE("TEXT"):
            return TEX;
        CASE("AND"):
            return AND;
        CASE("OR"):
            return OR;
        CASE("NOT"):
            return NOT;
        CASE("EXIT"):
            return EXIT;
        default: 
            return IDN;
    } 
}
void logical_expression(ITable * Table);
void logical_term(ITable * Table);
void logical_factor(ITable * Table);
void match(ITable * Table);
void term(ITable * Table);
void mult(ITable * Table);
void factor(ITable * Table);
// сначала идут вспомогательные функции, потом основные
// есть ли строка в векторе строк
bool StrIn(string & s, vector <string> & str)
{
    for (auto i: str)
    {
        if (i == s)
        {
            return true;
        }
    }
    return false;
}
// отправка сообщений в случае всех полей
void MesAll(vector<int>& sizes, vector<struct TableContent> & columns, string & message_server)
{
    for (int i = 0; i < columns.size(); i++)
    {
        sizes.push_back(max<int>(columns[i].size, columns[i].name.size()));
        message_server += columns[i].name;
        for (int j = 0; j < (sizes[i] - columns[i].name.size()); j++)
        {
            message_server += " ";
        }
        message_server += indent;
    }
}
// отправка сообщений без всех полей
void Mes(vector<int>& sizes, vector<struct TableContent> & columns, vector<string> fields, string & message_server)
{
    for (int i = 0; i < fields.size(); i++)
    {
        for (int j = 0; j < columns.size(); j++)
        {
            if (columns[j].name == fields[i])
            {
                sizes.push_back(max<int>(columns[j].size, columns[j].name.size()));
                message_server += columns[j].name;
                for (int k = 0; k < (sizes[j] - columns[j].name.size()); j++)
                {
                    message_server += " ";
                }
                message_server += indent;
            }
        }  
    }     
}
// последняя отправка в обоих случаях
void SecondMes(vector<int>& sizes, vector<string> fields, string & message_server, bool all, ITable * Table)
{
    IField * Field;
    long leng = 0;
    if (all)
    {
        leng = Table->GetAboutTable().amount_columns;
    }
    else
    {
        leng = fields.size();
    }
    for (int j = 0; j < leng; j++)
    {
        if (all)
        {
            Field = Table->GetField(j);
        } 
        else
        {
            Field = Table->GetField(fields[j]);
        }
        if (Field->OfType() == TEXT)
        {
            message_server += Field->Text();
            for (int k = 0; k < (sizes[j] - Field->Text().size()); k++)
            {
                message_server += " ";    
            }
            message_server += indent;
        }
        else
        {
            string str = to_string(Field->Long());
            message_server +=  str;
            for (int k = 0; k < (sizes[j] - str.size()); k++)
            {
                message_server += " ";
            }
            message_server += indent;
        }
    }
}
// изменения строки для регулярного класса
void NiceStr(string & str)
{
    for(int i = 0; i < str.size(); i++)
    {
        if (str[i] == '%')
        {
            str[i] = '*';
            str.insert(str.begin() + i, '.');
            i++;
        }
        else if (str[i] == '_')
        {
            str[i] = '.';
        }
        else if ((str[i] == '.') || (str[i] == '*'))
        {
            str.insert(str.begin() + i, '\\');
        }
    }
}
// обновление таблицы по массиву bool
void AllUpdate(ITable * Table, bool * flag, string & field_name, vector<string> & fields_update)
{
    int i = 0;
    for (Table->ReadFirst(); !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); Table->ReadNext(), i++)
    {
        if (flag[i])
        {
            if (Table->GetField(field_name)->OfType() == TEXT)
            {
                Table->GetField(field_name)->Text() = fields_update[i];
            }
            else
            {
                Table->GetField(field_name)->Long() = stol(fields_update[i]);
            }
            Table->Add();
        }
    }
}
// специальный switch для bool преобразований вектора
void Sw(vector <string>& new_vec, vector <lex_type_t>& vectyp, lex_type_t sym, bool flag, string prestr_last, string str_last)
{
    switch (sym)
    {
        case ADD:
            if(flag)
            {
                new_vec.push_back( to_string( stol(prestr_last) + stol(str_last) ) );
                vectyp.push_back(NUMBER);
            }
            break;
        case SUB:
            if(flag)
            {   
                new_vec.push_back( to_string( stol(prestr_last) - stol(str_last) ) );
                vectyp.push_back(NUMBER);
            }
            break;
        case ALL:
            if(flag)
            {   
                new_vec.push_back( to_string( stol(prestr_last) * stol(str_last) ) );
                vectyp.push_back(NUMBER);
            }
            break;
        case MOD:
            if(flag)
            {
                new_vec.push_back( to_string( stol(prestr_last) % stol(str_last) ) );
                vectyp.push_back(NUMBER);
            }
            break;
        case DIV:
            if(flag)
            {
                new_vec.push_back( to_string( stol(prestr_last) / stol(str_last) ) );
                vectyp.push_back(NUMBER);
            }
            break;
        case LESS_PEER:
            vectyp.push_back(BOOL);
            if (flag)
            {
                if (stol(prestr_last) <= stol(str_last))
                {
                    new_vec.push_back("TRUE");
                }
            }
            else
            {   
                if (prestr_last <= str_last)
                {
                    new_vec.push_back("TRUE");
                }
                else
                {
                    new_vec.push_back("FALSE");
                }
            }
            break;
        case LESS:
            vectyp.push_back(BOOL);
            if (flag)
            {
                if (stol(prestr_last) < stol(str_last))
                {
                    new_vec.push_back("TRUE");
                }
            }
            else
            {   
                if (prestr_last < str_last)
                {
                    new_vec.push_back("TRUE");
                }
                else
                {
                    new_vec.push_back("FALSE");
                }
            }
            break;
        case LARGER:
            vectyp.push_back(BOOL);
            if (flag)
            {
                if (stol(prestr_last) > stol(str_last))
                {
                    new_vec.push_back("TRUE");
                }
            }
            else
            {   
                if (prestr_last > str_last)
                {
                    new_vec.push_back("TRUE");
                }
                else
                {
                    new_vec.push_back("FALSE");
                }
            }
            break;
        case LARGER_PEER:
            vectyp.push_back(BOOL);
            if (flag)
            {
                if (stol(prestr_last) >= stol(str_last))
                {
                    new_vec.push_back("TRUE");
                }
            }
            else
            {   
                if (prestr_last >= str_last)
                {
                    new_vec.push_back("TRUE");
                }
                else
                {
                    new_vec.push_back("FALSE");
                }
            }
            break;
        case NOT_PEER:
            vectyp.push_back(BOOL);
            if (flag)
            {
                if (stol(prestr_last) != stol(str_last))
                {
                    new_vec.push_back("TRUE");
                }
            }
            else
            {   
                if (prestr_last != str_last)
                {
                    new_vec.push_back("TRUE");
                }
                else
                {
                    new_vec.push_back("FALSE");
                }
            }
            break;
        case PEER:
            vectyp.push_back(BOOL);
            if (flag)
            {
                if (stol(prestr_last) == stol(str_last))
                {
                    new_vec.push_back("TRUE");
                }
            }
            else
            {   
                if (prestr_last == str_last)
                {
                    new_vec.push_back("TRUE");
                }
                else
                {
                    new_vec.push_back("FALSE");
                }
            }
            break;
        default:
            message_server += "Error with operations\n";
            break;                   
    }
}
// чтение слов запроса
void next()
{
    cur_lex_text.clear(); // очищаем текущий текст
    state_t state = H; // начальное состояние H
    while (state != OK) 
    {
        switch (state) 
        {
            case H:
                if (std::isspace(c)) {
                } 
                else if (c == ','){
                    state = A; 
                    cur_lex_text += c;
                } 
                else if (c == '(') {
                    state = B;
                    cur_lex_text += c;
                } 
                else if (c == ')') {
                    state = C;
                    cur_lex_text += c;
                } 
                else if (c == '\'') {
                    state = E;
                } 
                else if (c == '*') {
                    state = G;
                    cur_lex_text += c;
                } 
                else if (c == '+') {
                    state = I;
                    cur_lex_text += c;
                } 
                else if (c == '-') {
                    state = J;
                    cur_lex_text += c;
                } 
                else if (c == '/') {
                    state = K;
                    cur_lex_text += c;
                } 
                else if (c == '%') {
                    state = L;
                    cur_lex_text += c;
                }       
                else if (c == '=') {
                    state = M;
                    cur_lex_text += c;
                }
                else if (c == '<') {
                    state = N;
                    cur_lex_text += c;
                }  
                else if (c == '>') {
                    state = O;
                    cur_lex_text += c;
                } 
                else if (c == '!') {
                    state = P;
                    cur_lex_text += c;
                } 
                else if (std::isdigit(c))
                {
                    state = F;
                    cur_lex_text += c;
                } else if ( ((c >= 'a') && (c <= 'z') )|| ( (c >= 'A') && (c <= 'Z') ))
                {
                    state = D;
                    cur_lex_text += c;
                } else if (c == '\n' || c == EOF || c == 0) {
                    cur_lex_type = END;
                    state = OK;
                }  
                else
                {
                    throw ("Error with lexem");   
                }
                break;
            case D:
                if ( ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || (c == '_') || (isdigit(c))) {
                    cur_lex_text += c;
                } 
                else {
                    cur_lex_type = WordToType(cur_lex_text);
                    state = OK;
                }
                break;
            case E:
                if ( c != '\'') {
                    cur_lex_text += c;
                } else {
                    cur_lex_type = ENT;
                    state = OK;
                    numb++;
                    if (numb < request.size())
                    {
                        c = request[numb];
                    }
                }
                break;
            case F:
                if (std::isdigit(c)) {
                    cur_lex_text += c;
                } else {
                    cur_lex_type = NUMBER;
                    state = OK;
                }
                break;
            case A:
                cur_lex_type = COMMA;
                cur_lex_text = ",";
                state = OK;
                break;
            case G:
                cur_lex_type = ALL;
                cur_lex_text = "*";
                state = OK;
                break;
            case B:
                cur_lex_type = OPEN;
                cur_lex_text = "(";
                state = OK;
                break;
            case C:
                cur_lex_type = CLOSE;
                cur_lex_text = ")";
                state = OK;
                break;
            case M:
                cur_lex_type = PEER;
                cur_lex_text = "=";
                state = OK;
                break;
            case I:
                cur_lex_type = ADD;
                cur_lex_text = "+";
                state = OK;
                break;
            case J:
                cur_lex_type = SUB;
                cur_lex_text = "-";
                state = OK;
                break;
            case K:
                cur_lex_type = DIV;
                cur_lex_text = "/";
                state = OK;
                break;
            case L:
                cur_lex_type = MOD;
                cur_lex_text = "%";
                state = OK;
                break;
            case N:
                if (c == '=') {
                    cur_lex_text += c;
                    cur_lex_type = LESS_PEER;
                    state = OK;
                    numb++;
                    if (numb < request.size())
                    {
                        c = request[numb];
                    }
                    else
                    {
                        throw ("Error with lexem");
                    } 
                }
                else
                {
                    cur_lex_type = LESS;
                    state = OK;
                }
                break;
            case O:
                if (c == '=') {
                    cur_lex_text += c;
                    cur_lex_type = LARGER_PEER;
                    state = OK;
                    numb++;
                    if (numb < request.size())
                    {
                        c = request[numb];
                    }
                    else
                    {
                        throw ("Error with lexem");
                    }
                }
                else
                {
                    cur_lex_type = LARGER;
                    state = OK;
                }
                break; 
            case P:
                if (c == '=') {
                    cur_lex_text += c;
                    cur_lex_type = NOT_PEER;
                    state = OK;
                    numb++;
                    if (numb < request.size())
                    {
                        c = request[numb];
                    }
                    else
                    {
                        throw ("Error with lexem");
                    } 
                }
                else
                {
                    throw ("Error with lexem");
                }
                break;      
            case OK:
                break;
            default:
                throw ("Error with lexem"); 
                break;
        }
        if (state != OK)
        {
            if (c == '\n') {
                cur_lex_type = END;
                state = OK;
            } else {
                numb++;
                if (numb < request.size())
                {
                    c = request[numb];
                }
                else
                {
                    cur_lex_type = END;
                    state = OK;
                }
            }   
        }
    }      
} 
vector<string> ExpressionToVec(ITable * Table);
vector<bool> LogicalToBool(ITable * Table);
void Create();
void Insert();
void Select();
void Update();
void Delete();
void Drop();
void Exit();

void Req(string & req)
{
    vec.clear();
    t_vec.clear();
    message_server.clear();
    request = req;
    numb = 0;
    c = request[numb];  
    next(); 
    // читаем первое слово запроса, ищем нужный вариант, если подходящего варианта нет, то запрос ошибочный
    switch (cur_lex_type)
    {
        case DROP:
            Drop();
            break;
        case DELETE:                   
            Delete();
            break;
        case CREATE:
            Create();
            break;
        case INSERT:
            Insert();
            break;
        case SELECT:
            Select();
            break;
        case UPDATE:
            Update();
            break;
        default:
            message_server += "Error with command " + cur_lex_text + "\n";
            break;                               
    }   
}
void Create() // +++
{
    next();
    if (cur_lex_type != TABLE) // после слова CREATE должно идти слово TABLE
    {
        message_server += "Error with CREATE clause\n";
    }
    next(); // далее должно идти название таблицы
    if (cur_lex_type != IDN) 
    {
        message_server += "Error with CREATE clause\n";
    }
    ITableStruct * Struct = ITable::CreateTableStruct(); // пытаемся создать структуру для таблицы
    Struct->SetName(cur_lex_text);
    next(); 
    if (cur_lex_type != OPEN) // далее должен идти символ (
    {
        message_server += "Error with CREATE clause\n";
    }
    next();
    if (cur_lex_type == CLOSE) // если в скобках ничего нет, то запрос ошибочный
    {
        message_server += "Error with CREATE clause\n";
    }
    while(cur_lex_type != CLOSE) // идем по содержимому записи в скобках
    {
        if (cur_lex_type != IDN)
        {
            message_server += "Error with CREATE clause\n";
        }
        std::string column_name = cur_lex_text; // запоминаем название предпологаемого столбца таблицы
        next(); 
        if (cur_lex_type == TEX) // первый случай (текстовый тип поля)
        {
            next();
            if (cur_lex_type != OPEN) // следующий символ должен быть (
            {
                message_server += "Error with CREATE clause\n";
            }
            next();
            if (cur_lex_type != NUMBER) // далее должно идти число
            {
                message_server += "Error with CREATE clause\n";
            }
            long j = stol(cur_lex_text); // переводим в число
            next();
            if (cur_lex_type != CLOSE) // следующим должен быть символ )
            {
                message_server += "Error with CREATE clause\n";
            }
            Struct->AddText(column_name, j); // если все хорошо, то добавляем поле в таблицу
        }
        else if (cur_lex_type == LON) // 2 случай, поле типа LONG
        {  
            Struct->AddLong(column_name); 
        }
        else
        {
            message_server += "Error with CREATE clause\n"; // никаких других типов полей быть не может
        }        
        next();
        if (cur_lex_type == COMMA) // далее следует либо ',', либо ')'
        {
            next(); // продолжим идти по циклу, пока не дойдем до )
        }
        else if (cur_lex_type == CLOSE) // запрос окончен
        {
            break;
        } 
        else
        {
            message_server += "Error with CREATE clause\n";
        }
    }
    next();
    if (cur_lex_type != END)
    {
        message_server += "Error with CREATE clause\n";
    }
    ITable * Table = ITable::Create(Struct); // если все прошло успешно, то создаем таблицу с заданными полями
    delete Table;
}
void Insert() // +++
{
    int i = 0;
    next();
    if (cur_lex_type != INTO) // следующим словом должно быть INTO
    {
        message_server += "Error with INSERT clause\n";
    }
    next();
    if (cur_lex_type != IDN) // далее должно идти названии таблицы
    {
        message_server += "Error with INSERT clause\n";
    }
    std::string table_name = cur_lex_text;
    ITable * Table =ITable::Open(table_name); // пытаемся открыть нужный файл
    next();
    if (cur_lex_type != OPEN) // следующим должен быть символ (
    {
        message_server += "Error with INSERT clause\n";
    }
    next();
    if (cur_lex_type == CLOSE) // если скобки пустые, то запрос ошибочный
    {
        message_server += "Error with INSERT clause\n";
    }
    AboutTable inf = Table->GetAboutTable(); //  получаем информацию о таблице
    lseek(Table->GetFd(), inf.last_record, SEEK_SET); // устанавливаем курсор на последнюю запись
    inf.current_record = inf.last_record; // устанавливаем текущей последнюю запись
    Table->SetAboutTable(inf); // обновляем информацию
    Table->SetFlag(false);
    //Table->LastRecord();
    while(cur_lex_type != CLOSE) // пока не встретим )
    {
        IField * Field = Table->GetField(i); // пытаемся получить iое поле таблицы
        if (Field->OfType() == TEXT) // если тип поле текстовый, то проверяем вставляем ли мы текстовую информацию
        {
            if (cur_lex_type != ENT)
            {
                message_server += "Error with INSERT clause\n";
            }
            Field->Text() = cur_lex_text.c_str();
        }
        else
        {
            if (cur_lex_type != NUMBER) // иначе проверяем вставляем ли мы числовую информацию
            {
                message_server += "Error with INSERT clause\n";
            }
            Field->Long() = stol(cur_lex_text); // перевод в число
        }
        i++; // для следующего поля
        next();
        if (cur_lex_type == COMMA) // если ',', то продолжаем обрабатывать запрос
        {
            next();
        }
        else if (cur_lex_type == CLOSE) // запрос закончился
        {
            break;
        } 
        else
        {
            message_server += "Error with INSERT clause\n";
        }
        
    }
    next();
    if (cur_lex_type != END)
    {
        message_server += "Error with INSERT clause\n";
    }
    Table->Add(); // если все хорошо, то добавим новые данные и обновим таблицу
    delete Table;
}
void Update() // +++
{
    bool notfl = false; // будет использоваться, чтобы определять по какому пути идем
    std::vector<std::string> fields_update;
    next();
    if (cur_lex_type != IDN) // пытаемся прочитать название таблицы
    {
        message_server += "Error with UPDATE clause\n";
    }
    std::string table_name = cur_lex_text;
    ITable * Table = ITable::Open(table_name); // пытаемся открыть нужную таблицу
    next();
    if (cur_lex_type != SET) // далее должно идти SET
    {
        message_server += "Error with UPDATE clause\n";
    }
    next();
    if (cur_lex_type != IDN) // следующим должно быть название поля
    {
        message_server += "Error with UPDATE clause\n";
    }
    string field_name = cur_lex_text; // запоминаем поле, которое будем обновлять
    next();
    if (cur_lex_type != PEER) // далее должно быть =
    {
        message_server += "Error with UPDATE clause\n";
    }   
    AboutTable inf = Table->GetAboutTable();  // получаем информацию о таблице
    lseek(Table->GetFd(), inf.last_record, SEEK_SET); // устанавливаем курсор на последнюю запись
    inf.current_record = inf.last_record; // делаем последнюю запись текущей
    Table->SetAboutTable(inf); // обновляем информацию о таблице
    Table->SetFlag(false);
    next(); 
    if (Table->GetField(field_name)->OfType() == TEXT) // если у нас текстовое поле, то просто заполняем update поля
    {   
        for(Table->ReadFirst(); !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); Table->ReadNext())
        {
            if (cur_lex_type == ENT)
            {
                fields_update.push_back(cur_lex_text);
            }
            else
            {
                fields_update.push_back(Table->GetField(cur_lex_text)->Text());
            }
        }  
        vec.clear();
        t_vec.clear();
        next();
    }
    else
    {
        term(Table);
        fields_update = ExpressionToVec(Table); // если у нас поле типа LONG, то заполняем выражение в вектор при помощи спец функции
    }       
    if (cur_lex_type != WHERE) // далее должно быть слово WHERE
    {
        message_server += "Error with UPDATE clause\n";
    }
    next();
    if (cur_lex_type == WHERE_ALL) // случай с выбором всех полей
    {
        int n = Table->GetAboutTable().amount_columns;
        bool mastrue[n];
        for (int l = 0; l < sizeof(mastrue); l++)
        {
            mastrue[l] = true;
        }
        AllUpdate(Table, mastrue, field_name, fields_update); // обновляем таблицу
    }
    else 
    {
        logical_expression(Table); // читаем логическое выражение
        if (cur_lex_type == NOT) // случай с NOT
        {
            notfl = true;
            next();
        }
        if (cur_lex_type == LIKE) // случай с LIKE
        {
            if (vec.size() != 1 || t_vec[0] != IDN)
            {
                message_server += "Error with WHERE clause\n";
            }
            AboutTable inf = Table->GetAboutTable(); // получаем информацию о таблице
            lseek(Table->GetFd(), inf.last_record, SEEK_SET); // ищем последнюю запись
            inf.current_record = inf.last_record; // делаем последнюю запись текущей
            Table->SetAboutTable(inf); // обновляем информацию о таблице
            Table->SetFlag(false);
            string check_field;
            if (Table->GetField(vec[0])->OfType() == TEXT ) // проверяем тип поля, должно быть текст
            {
                check_field = vec[0];
            }
            else
            {
                message_server += "Error with WHERE clause\n";
            }        
            vec.clear();
            t_vec.clear();
            next();
            if (cur_lex_type != ENT) // далее должно идти регулярное выражение
            {
                message_server += "Error with WHERE clause\n";
            } 
            string text_to_reg = cur_lex_text; // запоминаем текст, будем его преобразовывать
            next();
            if (cur_lex_type != END)
            {
                message_server += "Error with WHERE clause\n";   
            }
            NiceStr(text_to_reg); // некоторые преобразования строки для последующего использования регулярного класса regex
            std::regex reg_final(text_to_reg); // наконец можем использовать класс regex для регулярных выражений
            // обновляем информацию в таблице только там, где строка подходит под регулярное выражение
            bool mas[Table->GetAboutTable().amount_columns];
            int t = 0;
            for(Table->ReadFirst(); !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); Table->ReadNext()) 
            {
                mas[t] = regex_match(Table->GetField(check_field)->Text(), reg_final) ^ notfl;
                t++;
            }
            AllUpdate(Table, mas, field_name, fields_update); // обновляем таблицу
        }
        else if (cur_lex_type == IN) // случай с IN
        {
            vector<string> fields_update_second;
            bool long_fl = true;
            if (t_vec.size() == 1)
            {
                for(Table->ReadFirst(); !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); Table->ReadNext()) 
                {   // заполняем поля для обновления в текстовом случае
                    if (t_vec.back() == ENT)
                    {
                        fields_update_second.push_back(vec[0]);
                    }
                    else if (Table->GetField(vec.back())->OfType() == TEXT)
                    {
                        fields_update_second.push_back(Table->GetField(vec[0])->Text());
                    }
                    else
                    {
                        message_server += "Error with WHERE clause\n";
                    }   
                }
                vec.clear();
                t_vec.clear();
                long_fl = false;
            }
            else
            {   
                long_fl = true;
                fields_update_second = ExpressionToVec(Table); // если у нас поле типа LONG, то заполняем выражение в вектор при помощи спец функции
                vec.clear();
                t_vec.clear();
            }
            next();
            if (cur_lex_type != OPEN) // далее должен идти (
            {
                message_server += "Error with WHERE clause\n";   
            }
            next();
            if (cur_lex_type == CLOSE) // скобки не должны быть пустыми
            {
                message_server += "Error with WHERE clause\n";
            }
            std::vector<std::string> field_names;
            while (cur_lex_type != CLOSE) // дальше будем читать, пока не дойдем до закрывающей скобки
            {
                if (long_fl) // случай с LONG
                {
                    if (cur_lex_type != NUMBER) // должно быть число
                    {
                        message_server += "Error with WHERE clause\n";
                    }
                    field_names.push_back(to_string(stol(cur_lex_text)));
                }
                else
                {
                    if (cur_lex_type != ENT) // если не название, то ошибка
                    {
                        message_server += "Error with WHERE clause\n";
                    }
                    field_names.push_back(cur_lex_text);
                }
                next();
                if (cur_lex_type == COMMA) // далее может идти либо запятая, либо )
                {
                    next();
                }
                else if (cur_lex_type == CLOSE)
                {
                    break;
                }
                else
                {
                    message_server += "Error with WHERE clause\n";
                }        
            }
            bool mas_sec[Table->GetAboutTable().amount_columns];
            int l = 0;
            for(Table->ReadFirst(); !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); Table->ReadNext()) 
            {
                mas_sec[l] = StrIn(fields_update_second[l], field_names) ^ notfl; // StrIn проверяет вхождение строки в вектор строк
                l++;
            }
            AllUpdate(Table, mas_sec, field_name, fields_update); // обновляем таблицу
        } 
        else
        {
            if (notfl)
            {
                message_server += "Error with WHERE clause\n";
            }
            bool mas[Table->GetAboutTable().amount_columns];
            vector <bool> m = LogicalToBool(Table); // по логическому выражению проставляем в массив bool, какие поля нам нужно будет записывать в сообщения
            for (int t = 0; t < m.size(); t++)
            {
                mas[t] = m[t];
            }
            AllUpdate(Table, mas, field_name, fields_update); // обновляем таблицу
        }
    }
    next();
    if (cur_lex_type != END)
    {
        message_server += "Error with UPDATE clause\n";
    }
    delete Table;
}
void Select() // +++
{
    next();
    vector<string> fields;
    fields.clear();
    message_server.clear();
    bool all = false;
    if (cur_lex_type == ALL) // попытка выбрать все поля таблицы
    {   
        all = true; // запоминаем, что выбраны все поля
        next();
        if (cur_lex_type != FROM) // следующее слово должно быть FROM
        {
            message_server += "Error with SELECT clause\n";   
        } 
    }
    else if (cur_lex_type == IDN) // будем выбирать не все поля таблицы
    {
        while(true)
        {
            fields.push_back(cur_lex_text); // запоминаем названия полей
            next();
            if (cur_lex_type == FROM) // если дошли до FROM, то мы прочитали все поля, которые есть в запросе
            {
                break;  
            }
            else if (cur_lex_type == COMMA) // в другом случае, поля идут через запятую и проверяем их на правильность
            {
                next();
                if (cur_lex_type != IDN)
                {
                    message_server += "Error with SELECT clause\n";
                }
            }
            else // иначе ошибка
            {
                message_server += "Error with SELECT clause\n"; 
            } 
        }
    }
    else
    {
        message_server += "Error with SELECT clause\n"; 
    }
    next();
    if (cur_lex_type != IDN) // далее должно идти название таблицы
    {
        message_server += "Error with SELECT clause\n";
    }
    std::string table_name = cur_lex_text;
    ITable * Table = ITable::Open(table_name); // пытаемся открыть нужную таблицу
    next();
    if (cur_lex_type != WHERE) // далее должно идти слово WHERE
    {
        message_server += "Error with SELECT clause\n";
    }
    bool notfl = false;
    next();
    if (cur_lex_type == WHERE_ALL) // случай со *, т е выбраны все поля
    {
        vector<struct TableContent> columns = Table->GetColumns();
        vector<int> sizes;
        if (all)
        {
            MesAll(sizes, columns, message_server); // собираем сообщение от сервера (случай всех полей)
        }
        else
        {
            Mes(sizes, columns, fields, message_server); // собираем сообщение от сервера
        }
        message_server += "\n";
        for (Table->ReadFirst(); !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); Table->ReadNext())
        {   // идем от первой записи до последней
            SecondMes(sizes, fields, message_server, all, Table); // добавляем в сообщение нужные поля
            message_server += "\n";
        }
    }
    else 
    {
        logical_expression(Table); // пытаемся прочитать логическое выражение
        if (cur_lex_type == NOT) // запоминаем если случай с NOT
        {
            notfl = true;
            next();
        }
        if (cur_lex_type == LIKE) // далее либо вариант LIKE
        {
            if (vec.size() != 1 || t_vec[0] != IDN)
            {
                message_server += "Error with WHERE clause\n";
            }
            AboutTable inf = Table->GetAboutTable(); // получаем информацию о таблице
            lseek(Table->GetFd(), inf.last_record, SEEK_SET); // ищем последнюю запись
            inf.current_record = inf.last_record; // делаем последнюю запись текущей
            Table->SetAboutTable(inf); // обновляем информацию о таблицк
            Table->SetFlag(false);
            string check_field;
            if (Table->GetField(vec[0])->OfType() == TEXT ) // проверяем тип столбца, должен быть текст
            {
                check_field = vec[0];
            }
            else
            {
                message_server += "Error with WHERE clause\n";
            }
            vec.clear();
            t_vec.clear();
            next();
            if (cur_lex_type != ENT) // далее должно идти регулярное выражение
            {
                message_server += "Error with WHERE clause\n";
            } 
            string text_to_reg = cur_lex_text; // запоминаем регулярное выражение
            next();
            if (cur_lex_type != END)
            {
                message_server += "Error with WHERE clause\n";   
            }
            NiceStr(text_to_reg); // некоторые операции по преобразованию регулярного выражения
            std::regex reg_final(text_to_reg); // переход к классу regex
            vector<struct TableContent> columns = Table->GetColumns();
            vector<int> sizes;
            if (all)
            {
                MesAll(sizes, columns, message_server); // собираем сообщение от сервера (случай всех полей)
            }
            else
            {
                Mes(sizes, columns, fields, message_server); // собираем сообщение от сервера
            }
            message_server += "\n";
            for (Table->ReadFirst(); !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); Table->ReadNext())
            {
                if (regex_match(Table->GetField(check_field)->Text(), reg_final) ^ notfl) // проверяем на отличие от регулярного выражения
                {
                    SecondMes(sizes, fields, message_server, all, Table); // добавляем в сообщение нужные поля
                    message_server += "\n";
                }
            }
        }
        else if (cur_lex_type == IN) // случай с IN
        {
            vector<string> in_fields;
            bool long_fl = true;
            if (t_vec.size() == 1 && t_vec.back() == ENT) 
            {
                long_fl = false;   
                for(Table->ReadFirst(); !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); Table->ReadNext())
                {
                    in_fields.push_back(vec[0]);
                }
            }
            else if (t_vec.size() == 1 && t_vec.back() == IDN && Table->GetField(vec.back())->OfType() == TEXT) // случай с текстовым полем
            {
                long_fl = false;
                for(Table->ReadFirst(); ! (Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); Table->ReadNext())
                {
                    in_fields.push_back(Table->GetField(vec[0])->Text());
                }
            }
            else
            {   
                long_fl = true; // случай с полем LONG
                in_fields = ExpressionToVec(Table); // если у нас поле типа LONG, то заполняем выражение в вектор при помощи спец функции
            }
            vec.clear();
            t_vec.clear();
            next();
            if (cur_lex_type != OPEN) // далее должен идти символ (
            {
                message_server += "Error with WHERE clause\n";   
            }
            next();    
            if (cur_lex_type == CLOSE) // если скобки пустые, то запрос ошибочный
            {
                message_server += "Error with WHERE clause\n";
            }
            std::vector<std::string> field_names;
            while (cur_lex_type != CLOSE) // читаем внутренность IN
            {
                if (long_fl) // случай с типом LONG
                {
                    if (cur_lex_type != NUMBER) // должно быть число
                    {
                        message_server += "Error with WHERE clause\n";
                    }
                    field_names.push_back(to_string(stol(cur_lex_text)));
                }
                else // случай с типом TEXT
                {
                    if (cur_lex_type != ENT) // должно быть название
                    {
                        message_server += "Error with WHERE clause\n";
                    }
                    field_names.push_back(cur_lex_text);
                }
                next();
                if (cur_lex_type == COMMA) // далее может идти либо запятая, либо )
                {
                    next();
                }
                else if (cur_lex_type == CLOSE)
                {
                    break;
                }
                else
                {
                    message_server += "Error with WHERE clause\n";
                }
            }
            int j = 0; 
            vector<struct TableContent> columns = Table->GetColumns();
            vector<int> sizes;
            if (all) // случай с выбором всех полей
            {
                MesAll(sizes, columns, message_server); // собираем сообщение от сервера (случай всех полей)
            }
            else
            {
                Mes(sizes, columns, fields, message_server); // собираем сообщение от сервера 
            }  
            message_server += "\n";
            for (Table->ReadFirst(), j = 0; !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); Table->ReadNext(), j++)
            {
                if (StrIn(in_fields[j], field_names) ^ notfl) // проверяем нахождение полей
                {    
                    SecondMes(sizes, fields, message_server, all, Table); // добавляем в сообщение нужные поля
                    message_server += "\n";
                }         
            }
        } 
        else
        {
            if (notfl)
            {
                message_server += "Error with WHERE clause\n";
            }
            bool mas[Table->GetAboutTable().amount_columns];
            vector <bool> m = LogicalToBool(Table); // по логическому выражению проставляем в массив bool, какие поля нам нужно будет записывать в сообщения
            for (int t = 0; t < m.size(); t++)
            {
                mas[t] = m[t];
            }
            int j = 0;
            vector<struct TableContent> columns = Table->GetColumns();
            vector<int> sizes;
            if (all)
            {
                MesAll(sizes, columns, message_server); // собираем сообщение от сервера (случай всех полей)
            }
            else
            {
                Mes(sizes, columns, fields, message_server); // собираем сообщение от сервера     
            }
            message_server += "\n";
            for (Table->ReadFirst(); !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); Table->ReadNext(), j++)
            {
                if (mas[j]) // нужные поля определили до этого
                {
                    SecondMes(sizes, fields, message_server, all, Table); // добавляем в сообщение нужные поля
                    message_server += "\n";
                }
            }
        }
    }
    next();
    if (cur_lex_type != END)
    {
        message_server += "Error with SELECT clause\n";
    }
    delete Table;
}
void Drop() // +++
{
    next();
    if (cur_lex_type != TABLE) // после слова DROP должно идти TABLE
    {
        message_server += "Error with DROP clause\n";
    }
    next();
    if (cur_lex_type != IDN) // далее должно быть название таблицы
    {
        message_server += "Error with DROP clause\n";
    }
    string name = cur_lex_text; // запоминаем название
    next();
    if (cur_lex_type != END)
    {
        message_server += "Error with DROP clause\n";
    }
    ITable::Drop(name); // пытаемся выполнить DROP
}
void Delete() // +++
{
    next();
    if (cur_lex_type != FROM) // следующим должно быть слово FROM
    {
        message_server += "Error with DELETE clause\n";
    }
    next();
    if (cur_lex_type != IDN) // далее должно быть название таблицы
    {
        message_server += "Error with DELETE clause\n";
    }
    ITable * Table = ITable::Open(cur_lex_text); // пытаемся открыть нужную таблицу
    next();
    if (cur_lex_type != WHERE) // далее должно идти слово WHERE 
    {
        message_server += "Error with DELETE clause\n";
    }
    bool notfl = false;
    next();
    if (cur_lex_type == WHERE_ALL) // случай с удалением всего
    {
        for (Table->ReadFirst(); !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); )
        {
            Table->Delete();            
        }
    }
    else 
    {
        logical_expression(Table); // пытаемся прочитать логическое выражение
        if (cur_lex_type == NOT) // случай с NOT
        {
            notfl = true;
            next();
        }
        if (cur_lex_type == LIKE) // случай с LIKE
        {
            if (vec.size() != 1 || t_vec[0] != IDN)
            {
                message_server += "Error with WHERE clause\n";
            }
            AboutTable inf = Table->GetAboutTable(); //  получаем информацию о таблице
            lseek(Table->GetFd(), inf.last_record, SEEK_SET); // устанавливаем курсор на последнюю запись
            inf.current_record = inf.last_record; // устанавливаем текущей последнюю запись
            Table->SetAboutTable(inf); // обновляем информацию
            Table->SetFlag(false);
            string check_field;
            if (Table->GetField(vec[0])->OfType() == TEXT ) // поле должно быть типа текст
            {
                check_field = vec[0];
            }
            else
            {
                message_server += "Error with WHERE clause\n";
            }           
            vec.clear();
            t_vec.clear();
            next();
            if (cur_lex_type != ENT) // далее должно идти регулярное выражение
            {
                message_server += "Error with WHERE clause\n";
            } 
            string text_to_reg = cur_lex_text; // запоминаем регулярное выражение
            next();
            if (cur_lex_type != END)
            {
                message_server += "Error with WHERE clause\n";   
            }
            NiceStr(text_to_reg); // некоторые операции по изменению выражения
            std::regex reg_final(text_to_reg); // приводим к классу regex регулярных выражений        
            for (Table->ReadFirst(); !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record);)
            {
                if (regex_match(Table->GetField(check_field)->Text(), reg_final) ^ notfl) // в случае совпадения с регулярным выражением удаляем
                {
                    Table->Delete();
                }
                else
                {
                    Table->ReadNext(); // иначе читаем следующую запись
                }      
            }
        }
        else if (cur_lex_type == IN)  // случай с IN
        {
            vector<string> in_fields;
            bool long_fl = true;
            if (t_vec.size() == 1)
            {
                for(Table->ReadFirst(); ! (Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); Table->ReadNext())
                {
                    if (t_vec.back() == ENT) // должно быть название или текст
                    {
                        in_fields.push_back(vec[0]);
                    }
                    else if (Table->GetField(vec.back())->OfType() == TEXT)
                    {
                        in_fields.push_back(Table->GetField(vec[0])->Text());
                    }
                    else
                    {
                        message_server += "Error with WHERE clause\n"; 
                    }
                }  
                vec.clear();
                t_vec.clear();
                long_fl = false;
            }
            else
            {   
                long_fl = true;
                in_fields = ExpressionToVec(Table); // если у нас поле типа LONG, то заполняем выражение в вектор при помощи спец функции
                vec.clear();
                t_vec.clear();
            }
            next();
            if (cur_lex_type != OPEN) // далее должен идти символ (
            {
                message_server += "Error with WHERE clause\n";   
            }
            next();
            if (cur_lex_type == CLOSE) // если скобки пустые, то запрос ошибочный
            {
                message_server += "Error with WHERE clause\n";
            }
            std::vector<std::string> field_names;
            while (cur_lex_type != CLOSE) // будем читать, пока не дойдем до )
            {
                if (long_fl)
                {
                    if (cur_lex_type != NUMBER) // должно идти число
                    {
                        message_server += "Error with WHERE clause\n";
                    }
                    field_names.push_back(to_string(stol(cur_lex_text)));
                }
                else
                {
                    if (cur_lex_type != ENT) // должно идти название
                    {
                        message_server += "Error with WHERE clause\n";
                    }
                    field_names.push_back(cur_lex_text);
                }
                next();
                if (cur_lex_type == COMMA) // далее либо запятая, либо )
                {
                    next();
                }
                else if (cur_lex_type == CLOSE)
                {
                    break;
                }
                else // иначе ошибка
                {
                    message_server += "Error with WHERE clause\n";
                }
            }
            int i = 0;
            for (Table->ReadFirst(); !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); i++)
            {
                if (StrIn(in_fields[i], field_names) ^ notfl) // удаляем встреченные
                {    
                    Table->Delete();                
                }
                else // иначе переходим к следующему полю
                {
                    Table->ReadNext();
                }   
            }         
        } 
        else
        {
            if (notfl)
            {
                message_server += "Error with WHERE clause\n";
            }
            bool mas[Table->GetAboutTable().amount_columns];
            vector <bool> m = LogicalToBool(Table); // по логическому выражению проставляем в массив bool, какие поля нам нужно будет записывать в сообщения
            for (int t = 0; t < m.size(); t++)
            {
                mas[t] = m[t];
            }
            int i = 0;
            for (Table->ReadFirst(); !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); i++)
            {
                if (mas[i]) // удаляем те, которые были выбраны для удаления
                {
                    Table->Delete();
                }
                else
                {
                    Table->ReadNext(); // иначе переходим к следующему полю
                }
            }
        }
    }
    next();
    if (cur_lex_type != END)
    {
        message_server += "Error with DELETE clause\n";
    }
    delete Table;
}
vector<string> ExpressionToVec(ITable * Table)
{
    int last, prelast;
    vector<string> new_vec;
    vector <int> help_vec;
    for (auto i: t_vec) // проверяем, чтобы не было логических элементов
    {
        if (i == AND || i == OR || i == NOT 
            || i == LESS_PEER ||i == LESS || i == LARGER || i == LARGER_PEER 
            || i == PEER || i == NOT_PEER)
        {
            message_server += "Error with logical elements\n";
        }   
    }
    for (Table->ReadFirst(); !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); Table->ReadNext())
    {
        help_vec.clear(); // идем по каждому полю таблицы
        int k = 0;
        for (auto j: vec) 
        {
            if (t_vec[k] == IDN) // возможны 3 случая название, число или операция
            {
                help_vec.push_back(Table->GetField(j)->Long());
            } 
            else if (t_vec[k] == NUMBER)
            {
                help_vec.push_back(stol(j));
            }
            else
            {
                last = help_vec.back();
                help_vec.pop_back();
                prelast = help_vec.back();
                help_vec.pop_back();
                switch (t_vec[k])
                {
                    case ADD:
                        help_vec.push_back(prelast + last);
                        break;
                    case SUB:
                        help_vec.push_back(prelast - last);
                        break;
                    case ALL:
                        help_vec.push_back(prelast * last);
                        break;
                    case MOD:
                        help_vec.push_back(prelast % last);
                        break;
                    case DIV:
                        help_vec.push_back(prelast / last);
                        break;
                    default:
                        message_server += "Error with operations\n";
                        break;                   
                }
            }
            k++;
        }
        new_vec.push_back(to_string(help_vec.back()));
    }
    return new_vec;
}
//+
void WorkWithRecordToBool(vector <string>& new_vec, vector <lex_type_t>& vectyp, ITable * Table)
{
    for (int j = 0; j < vec.size(); j++)
    {
        if (t_vec[j] == IDN)
        {
            if (Table->GetField(vec[j])->OfType() == TEXT) // случай с текстом
            {
                new_vec.push_back(Table->GetField(vec[j])->Text());
                vectyp.push_back(ENT);
            }
            else
            {
                new_vec.push_back(to_string(Table->GetField(vec[j])->Long()));
                vectyp.push_back(NUMBER);
            }
        } 
        else if (t_vec[j] == NUMBER) // случай с числом
        {
            new_vec.push_back(vec[j]);
            vectyp.push_back(NUMBER);
        }
        else if (t_vec[j] == ENT) // случай с выражением
        {
            new_vec.push_back(vec[j]);
            vectyp.push_back(ENT);
        }
        else
        {
            string str_last = new_vec.back();
            new_vec.pop_back();
            lex_type_t last_typ = vectyp.back();
            vectyp.pop_back();
            if (t_vec[j] == NOT) // случай с NOT
            {
                if (last_typ == BOOL)
                {
                    if (str_last == "TRUE")
                    {
                        new_vec.push_back("FALSE");
                        vectyp.push_back(BOOL);
                    }
                    else
                    {
                        new_vec.push_back("TRUE");
                        vectyp.push_back(BOOL);
                    }    
                }
                else
                {
                    message_server += "Error with operations\n";
                } 
            }
            else
            {
                string prestr_last = new_vec.back();
                new_vec.pop_back();
                lex_type_t prestr_typ = vectyp.back();
                vectyp.pop_back();
                if (prestr_typ != last_typ)
                {
                    message_server += "Error with operations\n";
                }
                if (prestr_typ == ENT)
                {                       
                    Sw(new_vec, vectyp, t_vec[j], false, prestr_last, str_last); // делаем специальный switch и заполняем векторы
                }
                else if (prestr_typ == BOOL)
                {
                    switch(t_vec[j])
                    {
                        case AND:
                            if (prestr_last == "TRUE" && str_last == "TRUE")
                            {
                                vectyp.push_back(BOOL);
                                new_vec.push_back("TRUE");
                            }
                            else
                            {
                                vectyp.push_back(BOOL);
                                new_vec.push_back("FALSE");
                            }
                            break;
                        case OR:
                            if (prestr_last == "TRUE" || str_last == "TRUE")
                            {
                                vectyp.push_back(BOOL);
                                new_vec.push_back("TRUE");
                            }
                            else
                            {
                                vectyp.push_back(BOOL);
                                new_vec.push_back("FALSE");
                            }
                            break;    
                        default:
                            message_server += "Error with operations\n";
                            break;
                    }   
                }
                else
                {
                    Sw(new_vec, vectyp, t_vec[j], true, prestr_last, str_last); // делаем специальный switch и заполняем векторы
                }
            }
        }    
    }
}
vector<bool> LogicalToBool(ITable * Table)
{
    vector<bool> mas;
    vector <string> new_vec;
    vector <lex_type_t> vectyp;
    for (Table->ReadFirst(); !(Table->GetAboutTable().current_record == Table->GetAboutTable().last_record); Table->ReadNext())
    {
        new_vec.clear();
        vectyp.clear();
        WorkWithRecordToBool(new_vec, vectyp, Table);
        if (vectyp.back() != BOOL)
        {
            message_server += "Error with WHERE clause1\n";
        }
        else
        {
            if (new_vec.back() == "TRUE")
            {
                mas.push_back(true);
            }
            else
            {
                mas.push_back(false);
            }
        }
    } 
    return mas;
}
/*
<логическое выражение> ::= <логическое слагаемое> { OR <логическое слагаемое> }
<логическое слагаемое> ::= <логический множитель> { AND <логический множитель> }
<логический множитель> ::= NOT <логический множитель> | ( <логическое выражение> ) | (<отношение>)
<отношение> ::= <Text-отношение> | <Long-отношение> 
<Text-отношение> ::= <Text-выражение> <операция сравнения> <Text-выражение>
<Long-отношение> ::= <Long-выражение> <операция сравнения> <Long-выражение>
<операция сравнения> ::= = | > | < | >= | <= | != 
грамматика из методички в:
logical_expression -> logical_term { OR logical_term }
logical_term -> logical_factor { AND logical_factor }
logical_factor -> NOT logical_factor | match
match -> term [ = term | != term | > term | < term | >= term | <= term ]
term -> mult { ADD mult | SUB  mult } | TEXT | ENT
mult-> factor {  MULT factor | DIV factor | MOD factor }
factor -> LONG | NUMBER | ( logical_expression )
*/

void logical_expression(ITable * Table)
{
    logical_term(Table);
    while (cur_lex_type == OR) {
        next();
        logical_term(Table);
        vec.push_back("||");
        t_vec.push_back(OR);
    }
}
void logical_term(ITable * Table)
{
    logical_factor(Table);
    while (cur_lex_type == AND) {
        next();
        logical_factor(Table);
        vec.push_back("&&");
        t_vec.push_back(AND);
    }
}
void logical_factor(ITable * Table)
{
    if (cur_lex_type == NOT) {
        next();
        logical_factor(Table);
        vec.push_back("!");
        t_vec.push_back(NOT);    
    } 
    else 
    {
        match(Table);
    }
}    
void match(ITable * Table)
{
    lex_type_t cmp; 
    term(Table);
    if (cur_lex_type == NOT_PEER || cur_lex_type == PEER || cur_lex_type == LESS_PEER ||
                cur_lex_type == LARGER_PEER || cur_lex_type == LESS || cur_lex_type == LARGER)
    {
        cmp = cur_lex_type;
        next();
        term(Table);
        t_vec.push_back(cmp);
        vec.push_back("?");
    }
}
void term(ITable * Table)
{
    int flag;
    if (cur_lex_type == IDN && (Table->GetField(cur_lex_text)->OfType() == TEXT))
    {
        vec.push_back(cur_lex_text);
        t_vec.push_back(IDN);
        next();   
    }
    else if (cur_lex_type == ENT)
    {
        vec.push_back(cur_lex_text);
        t_vec.push_back(ENT);
        next();
    }
    else
    {
        mult(Table);
        while (cur_lex_type == ADD || cur_lex_type == SUB) {
            if (cur_lex_type == ADD)
            {
                flag = 1;
            }
            else
            {
                flag = 0;
            }
            next();
            mult(Table);
            if (flag == 1)
            {
                vec.push_back("+");
                t_vec.push_back(ADD);
            }
            else
            {   
                vec.push_back("-");
                t_vec.push_back(SUB);
            }
        }
    }
}
void mult(ITable * Table)
{
    int flag;
    factor(Table);
    while (cur_lex_type == ALL | cur_lex_type == DIV | cur_lex_type == MOD) {
        if (cur_lex_type == ALL)
        {
            flag = 0;
        }
        else if (cur_lex_type == DIV)
        {
            flag = 1;
        }
        else
        {
            flag = 2;
        }
        next();
        factor(Table);
        if (flag == 0)
        {
            vec.push_back("*");
            t_vec.push_back(ALL);
        }
        else if (flag == 1)
        {   
            vec.push_back("/");
            t_vec.push_back(DIV);
        } 
        else
        {
            vec.push_back("%");
            t_vec.push_back(MOD);
        }
    }
}

void factor(ITable * Table)
{
    if (cur_lex_type == IDN) {
        vec.push_back(cur_lex_text);
        t_vec.push_back(cur_lex_type);
        next();  
    } else if (cur_lex_type == NUMBER) {
        vec.push_back(cur_lex_text);
        t_vec.push_back(cur_lex_type);
        next();
    } else if (cur_lex_type == OPEN) {
        next();
        logical_expression(Table);
        if (cur_lex_type != CLOSE) {
            message_server += "Error with )\n";
        }
        next();
    } else {
        message_server += "Error with WHERE clause\n";
    }
}
#endif