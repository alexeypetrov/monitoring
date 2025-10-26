# URL Monitoring Server

HTTP-сервер для мониторинга доступности URL-адресов с поддержкой многопоточной обработки и хранением результатов в SQLite базе данных.

## Функционал

Приложение предоставляет API для проверки доступности веб-ресурсов:

- **Асинхронная проверка URL**: Принимает список URL-адресов и проверяет их доступность
- **Многопоточная обработка**: Поддерживает настраиваемое количество потоков для проверки url
- **Мониторинг производительности**: Измеряет время ответа и HTTP статус каждого URL
- **Хранение данных**: Сохраняет все результаты в SQLite базу данных
- **API**: Интерфейс для отправки запросов и получения результатов проверки доступности URL

### Архитектура

- **HTTP Server**: Boost.Asio для обработки HTTP запросов
- **URL Parser**: Многопоточный обработчик URL-ов с использованием libcurl
- **Database**: SQLite для хранения запросов и результатов проверки доступности URL

## Сборка

### Зависимости

- CMake 3.12+
- C++20 compiler
- Boost (program_options, asio)
- libcurl
- SQLite3
- nlohmann/json

### Ubuntu/Debian

```bash
sudo apt update
sudo apt install cmake build-essential libboost-all-dev libcurl4-openssl-dev sqlite3 libsqlite3-dev nlohmann-json3-dev libgtest-dev
```

### Сборка проекта

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Параметры запуска

```bash
./monitoring [OPTIONS]
```

### Доступные параметры:

| Параметр | Короткий | Значение по умолчанию | Описание |
|----------|----------|-----------------------|----------|
| `--port` | `-p` | 8080 | Порт HTTP сервера |
| `--max-threads` | `-m` | 1 | Максимальное количество потоков для обработки URL-ов |
| `--database-path` | `-d` | monitoring.db | Путь к файлу SQLite базы данных |
| `--timeout` | `-t` | 10 | Таймаут для HTTP запросов (в секундах) |
| `--help` | `-h` | - | Показать справку по параметрам |

### Примеры запуска:

```bash
# Запуск с параметрами по умолчанию
./monitoring

# Запуск на порту 8080 с 4 потоками и таймаутом 5 секунд
./monitoring --port 8080 --max-threads 4 --timeout 5

# Запуск с базой данных
./monitoring --database-path data.db --port 8080
```

## API Endpoints

### POST /check_urls

Отправляет список URL-ов на проверку.

**Request Body:**
```json
{
    "urls": [
        {"url": "http://localhost/1"},
        {"url": "http://localhost/2"},
        {"url": "http://localhost/3"}
    ]
}
```

**Response:**
```json
{
    "status": "OK",
    "request_id": 123,
    "count_urls": 3
}
```

### GET /get_results/{request_id}

Получает результаты проверки URL-ов по ID запроса.

**Response:**
```json
{
    "request_id": "123",
    "urls": [
        {
            "url": "http://localhost/1",
            "http_status": 200,
            "response_time": 245,
            "created_at": "2025-10-15 10:30:45"
        },
        {
            "url": "http://localhost/2",
            "http_status": 200,
            "response_time": 312,
            "created_at": "2025-10-15 10:30:46"
        },
        {
            "url": "http://localhost/3",
            "http_status": 200,
            "response_time": 189,
            "created_at": "2025-10-15 10:30:47"
        }
    ]
}
```

## Примеры использования с curl

### 1. Отправка URL-ов на проверку

```bash
curl -X POST http://localhost:8080/check_urls \
  -H "Content-Type: application/json" \
  -d '{
    "urls": [
      {"url": "http://localhost/1"},
      {"url": "http://localhost/2"},
      {"url": "http://localhost/3"}
    ]
  }'
```

**Ответ:**
```json
{"status": "OK", "request_id": 10, "count_urls": 3}
```

### 2. Получение результатов

```bash
curl http://localhost:8080/get_results/10
```

**Ответ:**
```json
{
  "request_id": "10",
  "urls": [
    {
      "url": "http://localhost/1",
      "http_status": 200,
      "response_time": 245,
      "created_at": "2025-09-01 10:30:45"
    },
    {
      "url": "http://localhost/2",
      "http_status": 200,
      "response_time": 312,
      "created_at": "2025-09-01 10:30:46"
    },
    {
      "url": "http://localhost/3",
      "http_status": 200,
      "response_time": 189,
      "created_at": "2025-09-01 10:30:45"
    }
  ]
}
```

### 2. Полный пример

```bash
# 1. Запуск сервера
./monitoring --port 8080 --max-threads 4 --timeout 5

# 2. В другом терминале - отправка URL-ов
curl -X POST http://localhost:8080/check_urls \
  -H "Content-Type: application/json" \
  -d '{
    "urls": [
      {"url": "http://localhost/1"},
      {"url": "http://localhost/2"},
      {"url": "http://localhost/3"}
    ]
  }'
# {"status": "OK", "request_id": 10, "count_urls": 3}

# 3. Получение результатов (подождать несколько секунд)
curl http://localhost:8080/get_results/10
```

## База данных

Приложение автоматически создаёт SQLite базу данных со следующей структурой:

### Таблица `requests`
- `id` - PRIMARY KEY
- `content` - JSON содержимое запроса
- `created_at` - время создания запроса

### Таблица `urls`
- `id` - PRIMARY KEY
- `request_id` - ID связанного запроса
- `url` - проверяемый URL
- `http_status` - HTTP статус код (0 для timeout)
- `response_time` - время ответа в миллисекундах
- `created_at` - время проверки URL

## HTTP коды ответов для API

- **200 OK** - Успешный запрос
- **400 Bad Request** - Неверный JSON в запросе
- **404 Not Found** - Неизвестный endpoint или request_id
- **405 Method Not Allowed** - Неподдерживаемый HTTP метод
- **415 Unsupported Media Type** - Неверный Content-Type
