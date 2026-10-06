FROM python:3.11-slim

LABEL authors="lubyant1994"

# OpenDSAS provides the `dsas` CLI used for transect casting, shoreline
# intersection, and change-rate computation.
RUN pip install --no-cache-dir opendsas

WORKDIR /app
COPY . /app
RUN pip install --no-cache-dir .

CMD ["python", "-m", "pytest"]
