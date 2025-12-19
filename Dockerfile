FROM ghcr.io/harlequix/ns:3.45

RUN apt update && apt install -y \
    python3-full \
    python3-venv \
    python3-dev

RUN python3 -m venv /opt/venv

RUN /opt/venv/bin/pip install --no-cache-dir numpy matplotlib

ENV PATH="/opt/venv/bin:$PATH"
