FROM ubuntu:22.04 AS system

# Build arguments for version control
ARG M2AIA_VERSION=0.6.5
ARG BUILD_DATE
ARG BUILD_VERSION

# Labels for container metadata
LABEL org.label-schema.schema-version="1.0"
LABEL org.label-schema.build-date=$BUILD_DATE
LABEL org.label-schema.version=$BUILD_VERSION
LABEL org.label-schema.name="M2aia-UMAP"
LABEL org.label-schema.description="UMAP dimensionality reduction for mass spectrometry imaging (MSI) data"
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
        unzip && \
    apt-get clean && \
    rm -rf /var/cache/apt/archives /var/lib/apt/lists/*

# Create virtual environment for better isolation
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

# Install Python dependencies with version pinning
RUN pip install --no-cache-dir --upgrade pip setuptools wheel && \
    pip install --no-cache-dir \
        SimpleITK>=2.2.0 \
        numpy>=1.21.0 \
        scipy>=1.7.0 \
        matplotlib>=3.4.0 \
        scikit-learn>=1.0.0 \
        umap-learn>=0.5.0 \
        m2aia>=${M2AIA_VERSION}

# Application stage
FROM system AS application

# Set working directory
WORKDIR /app

# Copy application file
COPY app_umap.py /app/app_umap.py
RUN chmod +x /app/app_umap.py

# Set entrypoint
ENTRYPOINT ["python3", "/app/app_umap.py"]