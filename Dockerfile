FROM python:3.11-slim

LABEL authors="lubyant1994"

RUN apt-get update && apt-get install -y --no-install-recommends \
    libgdal-dev \
    gdal-bin \
    && rm -rf /var/lib/apt/lists/*

# OpenDSAS provides the `dsas` CLI used for transect casting, shoreline
# intersection, and change-rate computation.
RUN pip install --no-cache-dir opendsas

WORKDIR /app
COPY . /app
RUN pip install --no-cache-dir .

CMD ["python", "-m", "pytest"]
