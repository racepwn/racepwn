FROM debian:latest

RUN apt-get update
RUN apt-get install -y build-essential golang clang libevent-dev openssl libssl-dev cmake wget unzip

COPY stub_config /home/config

RUN cd /home; wget https://github.com/racepwn/racepwn/archive/master.zip

RUN unzip /home/master.zip -d /opt/racepwn

RUN cd /opt/racepwn/racepwn-master/; ./build.sh

ENV PATH=${PATH}:/opt/racepwn/racepwn-master/build/racepwn