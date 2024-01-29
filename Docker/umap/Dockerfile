FROM ubuntu:22.04 as system

# system container dependencies
RUN apt-get update 
RUN DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get -y install tzdata
RUN apt-get install -q -y --no-install-recommends \
    libglu1-mesa-dev \
    libgomp1 \
    libopenslide-dev \
    python3 \
    python3-pip \
    python3-venv \
    git \
    unzip

# Install python dependencies
RUN pip install SimpleITK numpy scipy matplotlib scikit-learn umap-learn
RUN pip install m2aia==0.5.10

# clean caches
RUN pip cache purge
RUN apt-get clean && rm -rf /var/cache/apt/archives /var/lib/apt/lists/*

# provide app data
FROM system as application

COPY app_umap.py app_umap.py
ENTRYPOINT [ "python3", "app_umap.py"]