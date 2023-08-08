FROM rocker/r-ver:4.2 as rbase

LABEL maintainer="jtfc@posteo.de"

ARG BUILD_DATE
ARG BUILD_VERSION

# Labels.
LABEL org.label-schema.schema-version="1.0"

LABEL org.label-schema.build-date=$BUILD_DATE
LABEL org.label-schema.version=$BUILD_VERSION

LABEL org.label-schema.name="R-base"
LABEL org.label-schema.description="R base image for processing mass spectrometry imaging (MSI) data."

LABEL org.label-schema.url="https://m2aia.de/"
LABEL org.label-schema.vendor="m2aia.de"


RUN apt-get update
RUN apt-get install -q -y --no-install-recommends \  
    libglu1-mesa-dev \
    libglpk-dev \
    python3
    

# Commandline parser
# Requires python3
RUN R -e 'install.packages("argparse")'

# For NRRD access
RUN R -e 'install.packages("nat")' 

# Required?
RUN R -e 'install.packages("tidyverse")'
RUN R -e 'install.packages("devtools")'
RUN R -e 'install.packages("rgl")'

# MSI dependencies
RUN R -e 'install.packages("MALDIquant")'
RUN R -e 'install.packages("MALDIquantForeign")'


FROM rbase

RUN apt-get update
RUN apt-get install -q -y --no-install-recommends \
    libproj-dev \
    libgdal-dev

RUN R -e 'install.packages("parallel")'
RUN R -e 'install.packages("import")'
RUN R -e 'install.packages("spatstat")'
RUN R -e 'install.packages("raster")'
RUN R -e 'install.packages("Matrix")'

RUN apt-get install -q -y --no-install-recommends \
    git
RUN R -e 'install.packages("miniUI")'
RUN R -e 'install.packages("pkgbuild")'
RUN R -e 'install.packages("pkgload")'
RUN R -e 'install.packages("profvis")'
RUN R -e 'install.packages("shiny")'
RUN R -e 'install.packages("remotes")'
RUN R -e 'install.packages("usethis")'
RUN R -e 'devtools::install_github("CeMOS-Mannheim/moleculaR")'

RUN mkdir /app/
COPY . /app/
WORKDIR /app/

ENTRYPOINT [ "Rscript","doCppm.R"]