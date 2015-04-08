FROM ubuntu
MAINTAINER mrkrstphr/illacceptanything

RUN apt-get install php5 -y
RUN apt-get install curl -y
RUN curl -sS https://getcomposer.org/installer | php
RUN apt-get install software-properties-common; add-apt-repository ppa:gophers/go; apt-get update
RUN apt-get install golang -y
RUN apt-get install nodejs -y
RUN apt-get install python3 -y
RUN apt-get install linux-headers-$(uname -r) build-essential -y
