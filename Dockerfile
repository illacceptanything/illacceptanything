FROM ubuntu
MAINTAINER mrkrstphr/illacceptanything

RUN apt-get install php
RUN curl -sS https://getcomposer.org/installer | php
RUN apt-get install go
RUN apt-get install nodejs
RUN apt-get install python3
