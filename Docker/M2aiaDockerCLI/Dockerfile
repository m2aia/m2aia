# install ubuntu version that is compatible with M2aia
ARG version=20.04
FROM ubuntu:${version}
# refrain from interactive questions
ARG DEBIAN_FRONTEND=noninteractive
# install additional packages
RUN apt update && apt upgrade -y && apt install -y \
python3 \
r-base \
vim
COPY ./bin /bin
# put vimrc file in root dir, to enable vim customization
COPY .vimrc /root

