Работал над вариантом клиент и сервер на одной машине.
Программа server.cpp отвечает за действия сервера.
Программа client.cpp отвечает за действия клиента.
В sock_wrap.hpp описаны основные взаимодействия клиента и сервера, за основу взята оболочка sock_wrap.h из задания.
В dbms.hpp описаны основные действия с таблицами, за основу взята оболчка dbms.h из задания.
str_switch.hpp используется для того, чтобы можно было делать switch(string)
В work.hpp происходит основная работа: анализируется запрос клиента, сервер пытается ответить на запрос клиента
Чтобы протестировать:
1) make
2) ./server
3) ./client (в другом терминальном окне)
4) писать запросы от клиента, например:
CREATE TABLE Students (First_name TEXT (10), Surname TEXT (15), Age LONG,Phone TEXT (9) )
INSERT INTO Students ( 'Sergey', 'Ivanov', 18, '145-45-45' )
INSERT INTO Students ( 'Alexey', 'Petrov', 20, '343-65-45' )
INSERT INTO Students ( 'Andrey', 'Fedorov', 23, '123-45-18' )
INSERT INTO Students ( 'Alexandre', 'Zaharov', 20, '450-33-33' )
SELECT First_name, Surname FROM Students WHERE Age IN (18, 19, 20)
SELECT * FROM Students WHERE Phone LIKE '%-%-45'
SELECT * FROM Students WHERE Phone LIKE '%45%'
SELECT Phone FROM Students WHERE Surname = 'Ivanov'
SELECT Surname FROM Students WHERE ALL
SELECT * FROM Students WHERE First_name LIKE '[ABC][^mno]_x%'
SELECT Surname, Phone FROM Students WHERE (Age > 19) AND (Surname > 'M')
UPDATE Students SET Age = Age + 1 WHERE ALL
DROP TABLE Students 
(Это пример из учебника)

Чтобы отобразить все записи: SELECT * FROM Students WHERE ALL