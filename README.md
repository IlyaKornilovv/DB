<<<<<<< HEAD
# DB
=======
# DB

DB — отдельная самостоятельная учебная клиент-серверная СУБД на C++17 с GUI-клиентом на WPF. Проект сделан как независимая реализация: код и структура внутренних файлов отличаются, но сценарий демонстрации совпадает с типовым запуском из видео.

## Что внутри

- C++17 TCP-сервер `customdb_server`.
- Порт по умолчанию: `5432`.
- SQL batch-запросы через `;`.
- JSON-ответы для SELECT, DDL, DML и ошибок.
- Файловое хранилище в `data/` с сохранением после перезапуска.
- C API: `db_connect`, `db_execute`, `db_disconnect`, `db_free_string`.
- WPF GUI в `src/gui`, запускается отдельно командой `dotnet run`.
- CMake, Dockerfile, docker-compose, unit/integration/functional tests.

## Быстрая демонстрация как на видео

### 1. Собрать сервер

В WSL/Linux из корня проекта:

```bash
chmod +x scripts/build_full.sh run_all_tests.sh
./scripts/build_full.sh
```

### 2. Запустить сервер

```bash
./build/customdb_server --host 0.0.0.0 --port 5432
```

Ожидаемый вывод:

```text
DB server listening on 0.0.0.0:5432
```

### 3. Запустить GUI отдельно

Открой Windows PowerShell во второй вкладке/окне и перейди в папку GUI. Если проект лежит в WSL, путь будет похож на:

```powershell
cd \\wsl.localhost\Ubuntu\home\<user>\projects\DB\src\gui
```

Запуск:

```powershell
dotnet run
```

В GUI оставь:

```text
Host: localhost
Port: 5432
```

Нажми **Connect**, затем **Execute**.

## Проверка тестами

Linux/macOS/WSL:

```bash
./run_all_tests.sh
```

Windows:

```bat
run_all_tests.bat
```

## SQL, который можно показать в GUI

```sql
CREATE DATABASE company;
USE company;
CREATE TABLE employees (id INT AUTO_INCREMENT, name TEXT, email TEXT UNIQUE, salary FLOAT);
INSERT INTO employees (name, email, salary) VALUES ('Alice', 'alice@mail.com', 50000);
INSERT INTO employees (name, email, salary) VALUES ('Bob', 'bob@mail.com', 60000);
SELECT * FROM employees;
```

## Поддерживаемые команды

```sql
CREATE DATABASE name;
DROP DATABASE name;
USE name;
CREATE TABLE users (id INT AUTO_INCREMENT, name TEXT UNIQUE, age INT);
DROP TABLE users;
INSERT INTO users (name, age) VALUES ('Alice', 25);
SELECT * FROM users;
SELECT name FROM users WHERE age >= 18;
UPDATE users SET age = 26 WHERE name = 'Alice';
DELETE FROM users WHERE name = 'Alice';
SHOW DATABASES;
SHOW TABLES;
```

Типы колонок: `INT`, `INTEGER`, `FLOAT`, `DOUBLE`, `BOOL`, `BOOLEAN`, `TEXT`, `VARCHAR(n)`, `TEXT[]`.

## Протокол

Запрос:

```text
[4 байта длины SQL в big-endian][SQL UTF-8]
```

Ответ:

```text
[4 байта статус][4 байта длины JSON в big-endian][JSON UTF-8]
```

## Docker

```bash
docker compose up --build
```

Если Docker не установлен, `scripts/build_full.sh` просто пропустит сборку образа.
>>>>>>> 49b1923 (DB v1.0)
