FROM buildpack-deps:jessie

RUN groupadd -r mysql && useradd -r -g mysql mysql

RUN apt-get update && apt-get install -y cmake unzip

COPY . /build
RUN cd /build \
	&& ./build.sh \
	&& mv /build/install_package /phxsql \
	&& rm -rf /build

WORKDIR /phxsql/tools

ENV PATH="/phxsql/sbin:/phxsql/percona.src/bin:$PATH"

COPY docker-entrypoint.sh /usr/local/bin/
ENTRYPOINT ["docker-entrypoint.sh"]

EXPOSE 54321 6000 11111 17000 8001

VOLUME /data
