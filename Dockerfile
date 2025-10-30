#  Этап 1: сборка Qt-приложения
# ==============================
FROM ubuntu:22.04 AS builder

# Устанавливаем нужные инструменты и Qt
RUN apt-get update && apt-get install -y \
    build-essential \
    qtbase5-dev \
    qt5-qmake \
    qtbase5-dev-tools \
    libqt5widgets5 \
    libqt5gui5 \
    libqt5core5a \
    && rm -rf /var/lib/apt/lists/*

# Создаём рабочую директорию
WORKDIR /app

# Копируем проект в контейнер
COPY . /app

# Собираем проект через qmake и make
RUN qmake && make -j$(nproc)

# ==============================
#  Этап 2: запуск контейнера
# ==============================
FROM ubuntu:22.04

# Устанавливаем только библиотеки, нужные для запуска Qt GUI
RUN apt-get update && apt-get install -y \
    libqt5widgets5 \
    libqt5gui5 \
    libqt5core5a \
    libx11-6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Копируем собранное приложение из builder-а
COPY --from=builder /app /app

# По умолчанию запускаем приложение
CMD ["./Pixel"]
