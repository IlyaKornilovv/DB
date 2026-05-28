# DB Workbench / CustomDB

> Учебная клиент-серверная СУБД на C++ с TCP-сервером, файловым хранением данных, SQL-подобным интерфейсом и WPF GUI-клиентом.

![C++](https://img.shields.io/badge/C++-17-blue)
![CMake](https://img.shields.io/badge/CMake-build-informational)
![WPF](https://img.shields.io/badge/WPF-.NET%2010-purple)
![Docker](https://img.shields.io/badge/Docker-supported-blue)
![Status](https://img.shields.io/badge/status-ready%20for%20demo-brightgreen)

---

## Содержание

- [Описание проекта](#описание-проекта)
- [Основные возможности](#основные-возможности)
- [Архитектура](#архитектура)
- [Структура проекта](#структура-проекта)
- [Требования](#требования)
- [Сборка и запуск](#сборка-и-запуск)
- [Запуск GUI](#запуск-gui)
- [SQL-примеры](#sql-примеры)
- [Тестирование](#тестирование)
- [Docker](#docker)
- [Формат сетевого протокола](#формат-сетевого-протокола)
- [Формат JSON-ответов](#формат-json-ответов)
- [Возможные проблемы](#возможные-проблемы)

---

## Описание проекта

**DB Workbench / CustomDB** — это учебная реляционная СУБД, реализованная в формате клиент-серверного приложения.

Проект состоит из:

- C++17 ядра СУБД;
- TCP-сервера для обработки SQL-запросов;
- файлового хранилища данных;
- C API / клиентской библиотеки;
- WPF GUI-клиента на C#;
- автоматических тестов;
- Docker-сборки.

Система предназначена для демонстрации базовых принципов работы СУБД: создание баз данных, создание таблиц, вставка, выборка, обновление, удаление данных и сохранение состояния между перезапусками сервера.

---

## Основные возможности

### SQL-команды

Поддерживаются базовые DDL и DML операции:

```sql
CREATE DATABASE database_name;
DROP DATABASE database_name;
USE database_name;

CREATE TABLE table_name (...);
DROP TABLE table_name;

INSERT INTO table_name (...) VALUES (...);
SELECT * FROM table_name;
SELECT * FROM table_name WHERE column = value;

UPDATE table_name SET column = value WHERE column = value;
DELETE FROM table_name WHERE column = value;

SHOW DATABASES;
SHOW TABLES;
```

### Типы данных

```text
INT
FLOAT
BOOL
TEXT
VARCHAR(n)
TEXT[]
```

### Дополнительные возможности

- `AUTO_INCREMENT` для автоматической генерации идентификаторов;
- `UNIQUE` для запрета дубликатов;
- batch-запросы через `;`;
- постоянное хранение данных в папке `data/`;
- TCP-протокол взаимодействия;
- JSON-ответы;
- WPF GUI для удобной работы с SQL;
- сборка через CMake;
- запуск через Docker;
- unit, integration и functional tests.

---

## Архитектура

Проект построен по модульному принципу.

```text
┌─────────────────────┐
│     WPF GUI Client  │
│   C# / .NET / WPF   │
└──────────┬──────────┘
           │ TCP
           ▼
┌─────────────────────┐
│    C++ TCP Server   │
│   customdb_server   │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│      SQL Layer      │
│ Parser / Executor   │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│    Storage Layer    │
│  files / JSON data  │
└─────────────────────┘
```

### Основные компоненты

| Компонент | Назначение |
|---|---|
| `customdb_server` | TCP-сервер СУБД |
| SQL Parser / Executor | Разбор и выполнение SQL-команд |
| Storage Engine | Сохранение данных между запусками |
| C API | Интерфейс для внешних клиентов |
| WPF GUI | Графический клиент для работы с базой |
| Tests | Проверка корректности работы проекта |

---

## Структура проекта

```text
DB/
├── CMakeLists.txt
├── Dockerfile
├── docker-compose.yml
├── README.md
├── functional_test.py
├── run_all_tests.sh
├── run_all_tests.bat
├── scripts/
│   ├── build_full.sh
│   └── build_full.bat
├── src/
│   ├── core/
│   ├── network/
│   ├── client/
│   ├── server/
│   └── gui/
│       ├── DbWorkbench.Client.csproj
│       ├── App.xaml
│       ├── Program.cs
│       ├── QueryWorkbench.xaml
│       └── QueryWorkbench.xaml.cs
├── include/
├── tests/
└── data/
```

Папка `data/` создаётся автоматически во время работы сервера и хранит пользовательские базы данных.

---

## Требования

### Для C++ сервера

Linux / WSL:

```bash
sudo apt update
sudo apt install -y cmake g++ python3
```

Windows:

- CMake;
- Visual Studio Build Tools или Visual Studio с компонентом C++;
- Python 3.

### Для GUI

Windows:

- .NET SDK 10 или совместимый SDK;
- Windows 10/11;
- WPF поддерживается только на Windows.

Проверка .NET:

```powershell
dotnet --version
```

---

## Сборка и запуск

### 1. Перейти в корень проекта

```bash
cd ~/OOP/projects/DB
```

### 2. Очистить старую сборку и старые данные

```bash
rm -rf build data
```

### 3. Выдать права на запуск скриптов

```bash
chmod +x scripts/build_full.sh run_all_tests.sh
```

### 4. Собрать проект

```bash
./scripts/build_full.sh
```

После успешной сборки сервер будет находиться здесь:

```text
build/customdb_server
```

### 5. Запустить сервер

```bash
./build/customdb_server --host 0.0.0.0 --port 5432
```

Ожидаемый вывод:

```text
DB server listening on 0.0.0.0:5432
```

---

## Запуск GUI

GUI запускается отдельно от сервера.

### Вариант 1. Запуск из PowerShell

```powershell
cd C:\DB\DB_video_ready_gui_fix3\src\gui
dotnet run
```

В GUI указать:

```text
Host: 127.0.0.1
Port: 5432
```

Затем нажать:

```text
Connect
```

### Вариант 2. Запуск GUI из WSL через Windows PowerShell

```bash
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "cd 'C:\DB\DB_video_ready_gui_fix3\src\gui'; dotnet run"
```

> Важно: WPF-приложение является Windows-приложением. Его не следует запускать как Linux GUI-приложение внутри WSL.

---

## SQL-примеры

### Полный демонстрационный сценарий

```sql
CREATE DATABASE video_demo;
USE video_demo;
CREATE TABLE users (id INT AUTO_INCREMENT, name TEXT UNIQUE, age INT, balance FLOAT, active BOOL);
INSERT INTO users (name, age, balance, active) VALUES ('Alice', 25, 1500.50, true);
INSERT INTO users (name, age, balance, active) VALUES ('Bob', 30, 2100.75, true);
INSERT INTO users (name, age, balance, active) VALUES ('Charlie', 22, 900.00, false);
SELECT * FROM users;
```

### Выбор конкретного пользователя

```sql
USE video_demo;
SELECT * FROM users WHERE name = 'Alice';
```

### Обновление записи

```sql
USE video_demo;
UPDATE users SET age = 26 WHERE name = 'Alice';
SELECT * FROM users;
```

### Удаление записи

```sql
USE video_demo;
DELETE FROM users WHERE name = 'Charlie';
SELECT * FROM users;
```

### Проверка UNIQUE

```sql
USE video_demo;
INSERT INTO users (name, age, balance, active) VALUES ('Alice', 28, 3000.00, true);
```

Ожидаемый результат — ошибка, потому что поле `name` объявлено как `UNIQUE`.

### Список баз и таблиц

```sql
SHOW DATABASES;
USE video_demo;
SHOW TABLES;
```

---

## Тестирование

Запуск всех тестов:

```bash
./run_all_tests.sh
```

На Windows:

```bat
run_all_tests.bat
```

В тесты входят:

| Тип тестов | Назначение |
|---|---|
| Unit tests | Проверка отдельных модулей |
| Integration tests | Проверка взаимодействия компонентов |
| Functional tests | Проверка полного сценария работы сервера |

---

## Docker

### Сборка Docker-образа

```bash
docker build --no-cache -t databaseoficial-customdb-server .
```

### Запуск контейнера

```bash
docker run --rm -p 5432:5432 databaseoficial-customdb-server
```

### Docker Compose

```bash
docker compose up --build
```

---

## Формат сетевого протокола

Клиент отправляет запрос в формате:

```text
[4 bytes: длина SQL-запроса] [SQL-запрос]
```

Сервер отвечает:

```text
[4 bytes: статус] [4 bytes: длина JSON-ответа] [JSON-ответ]
```

---

## Формат JSON-ответов

### SELECT

```json
{
  "type": "select",
  "columns": ["id", "name", "age"],
  "rows": [
    ["1", "Alice", "25"],
    ["2", "Bob", "30"]
  ],
  "affected_rows": 2
}
```

### DDL / DML

```json
{
  "type": "ok",
  "message": "Query executed successfully",
  "affected_rows": 1
}
```

### Ошибка

```json
{
  "type": "error",
  "message": "Invalid CREATE TABLE syntax"
}
```

---

## Возможные проблемы

| Проблема | Решение |
|---|---|
| GUI не подключается к серверу | Указать `127.0.0.1` вместо `localhost` |
| Порт 5432 занят | Запустить сервер на другом порте |
| `dotnet run` не открывает окно | Запускать GUI из Windows-папки, не из `\\wsl.localhost\...` |
| `Permission denied` при запуске скрипта | Выполнить `chmod +x scripts/build_full.sh run_all_tests.sh` |
| Ошибка `Invalid CREATE TABLE syntax` | Писать `CREATE TABLE` в одну строку |
| Данные остались от прошлой демонстрации | Выполнить `rm -rf data` перед запуском |
| Docker копирует старую сборку | Проверить наличие `.dockerignore` |

---

## Демонстрация сохранения данных

1. Создать базу и таблицу.
2. Добавить данные.
3. Выполнить `SELECT`.
4. Остановить сервер через `Ctrl + C`.
5. Запустить сервер заново.
6. Снова выполнить:

```sql
USE video_demo;
SELECT * FROM users;
```

Данные должны сохраниться.

---

## Статус проекта

Проект готов к демонстрации:

- сервер собирается через CMake;
- GUI подключается к TCP-серверу;
- SQL-команды выполняются;
- данные сохраняются между перезапусками;
- тесты запускаются через единый скрипт;
- доступна Docker-сборка.

---

## Авторы

Проект выполнен в рамках лабораторной работы / кейс-чемпионата по дисциплине:

> Программирование на основе классов и шаблонов

