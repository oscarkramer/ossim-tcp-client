FROM ubuntu:16.04

RUN apt-get update
RUN apt-get install cmake -y
RUN apt-get install gcc-4.9 -y
RUN apt-get install g++ -y
RUN apt-get install libpthread-stubs0-dev -y

COPY client_app /client_app/
RUN chmod 777 /client_app/bin/start-client.sh

WORKDIR /client_app/bin

RUN cmake -DCMAKE_BUILD_TYPE=Release ../src/client_app
RUN make

ENTRYPOINT ["bash", "/client_app/bin/start-client.sh"]
