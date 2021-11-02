FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get install -y build-essential
RUN apt-get install -y texlive-latex-extra texlive-fonts-extra texlive-bibtex-extra latexmk
RUN apt-get install -y python3-pygments

COPY . /src
WORKDIR /src

CMD make clean && make
