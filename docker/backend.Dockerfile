FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1

RUN apt-get update && \
    apt-get install -y \
        clang \
        llvm \
        lld \
        cmake \
        ninja-build \
        git \
        python3 \
        python3-pip \
        python3-venv \
        build-essential \
        curl && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY backend/requirements.txt ./requirements.txt

RUN python3 -m venv /opt/venv && \
    /opt/venv/bin/pip install --no-cache-dir \
        -r requirements.txt

ENV PATH="/opt/venv/bin:$PATH"

COPY engine/ ./engine/

RUN cmake -S engine -B /tmp/engine-build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release && \
    cmake --build /tmp/engine-build && \
    install -m 755 /tmp/engine-build/llvm-engine /usr/local/bin/llvm-engine && \
    rm -rf /tmp/engine-build

COPY backend/ ./backend/

EXPOSE 8000

CMD ["uvicorn","backend.main:app","--host","0.0.0.0","--port","8000"]
