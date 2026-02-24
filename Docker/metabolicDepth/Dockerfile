FROM nvidia/cuda:11.8.0-runtime-ubuntu22.04 AS system

# Build arguments for version control
ARG M2AIA_VERSION=0.6.5
ARG BUILD_DATE
ARG BUILD_VERSION

# Labels for container metadata
LABEL org.label-schema.schema-version="1.0"
LABEL org.label-schema.build-date=$BUILD_DATE
LABEL org.label-schema.version=$BUILD_VERSION
LABEL org.label-schema.name="M2aia-MetabolicDepth"
LABEL org.label-schema.description="Multi-GASTON metabolic depth analysis for mass spectrometry imaging (MSI) data"
LABEL org.label-schema.url="https://m2aia.de/"
LABEL org.label-schema.vendor="m2aia.de"

# Install system dependencies in a single layer
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get install -y tzdata && \
    apt-get install -q -y --no-install-recommends \
        libglu1-mesa-dev \
        libgomp1 \
        libopenslide-dev \
        python3 \
        python3-pip \
        python3-venv \
        git \
        build-essential \
        unzip && \
    apt-get clean && \
    rm -rf /var/cache/apt/archives /var/lib/apt/lists/*

# Create and activate virtual environment
RUN python3 -m venv /.venv
ENV VIRTUAL_ENV="/.venv"
ENV PATH="$VIRTUAL_ENV/bin:$PATH"

# Set working directory
WORKDIR /app

# Copy requirements file
COPY requirements.txt .

# Install Python dependencies with version pinning
RUN pip install --no-cache-dir --upgrade pip setuptools wheel && \
    pip install --no-cache-dir -r requirements.txt

# Install Multi-GASTON from GitHub
RUN pip install --no-cache-dir git+https://github.com/raphael-group/Multi-GASTON.git

# Application stage
FROM system AS application

# Set working directory
WORKDIR /app

# Copy application file
COPY app_metabolic_depth.py /app/app_metabolic_depth.py
RUN chmod +x /app/app_metabolic_depth.py

# Override numba cache to use home directory
ENV NUMBA_CACHE_DIR=/home/m2aia/.cache/numba

# Set entrypoint
ENTRYPOINT ["python", "/app/app_metabolic_depth.py"]
