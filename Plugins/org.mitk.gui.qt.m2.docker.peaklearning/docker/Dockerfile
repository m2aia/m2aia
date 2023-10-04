FROM ubuntu:22.10 as system

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

# Manage to use the virtual environment
RUN python3 -m venv /.venv
ENV VIRTUAL_ENV="/.venv"
RUN export VIRTUAL_ENV
ENV _OLD_VIRTUAL_PATH="$PATH"
ENV PATH="$VIRTUAL_ENV/bin:$PATH"
RUN export PATH

# Install python dependencies
RUN pip install tensorflow
RUN pip install SimpleITK numpy scipy
RUN pip install -i https://test.pypi.org/simple/ --extra-index-url https://pypi.org/simple pym2aia

# clean caches
RUN pip cache purge
RUN apt-get clean && rm -rf /var/cache/apt/archives /var/lib/apt/lists/*

# provide app data
FROM system as application
RUN git clone https://github.com/m2aia/pym2aia-examples-msiPL.git /msiPL
COPY app_msiPL.py app_msiPL.py
ENTRYPOINT [ "python", "app_msiPL.py" ]