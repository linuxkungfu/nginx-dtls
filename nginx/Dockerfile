FROM alpine:3.13 as build-env
# FROM frolvlad/alpine-gxx as build-env
# FROM buildpack-deps:buster as build-bin
USER 0
WORKDIR /app/cst-nginx
RUN apk update \
    && apk add --no-cache build-base pcre-dev zlib-dev make perl linux-headers

FROM build-env as build-bin
COPY conf conf
COPY auto auto
COPY configure configure
COPY dep dep
COPY contrib contrib
COPY html html
COPY man man
COPY src src
RUN cd dep/ && tar -axvf openssl-1.1.1l.tar.gz
# RUN ./configure --with-stream --with-stream_ssl_module --with-openssl=./dep/openssl-1.1.1l --prefix=/app/cst-nginx --with-debug
RUN ./configure --with-stream --with-stream_ssl_module --with-openssl=./dep/openssl-1.1.1l --prefix=/app/cst-nginx
RUN make
# from buildpack-deps:buster

FROM alpine:3.13.2
WORKDIR /app/cst-nginx
COPY --from=build-bin --chown=1000:1000 /app/cst-nginx/objs/nginx /app/cst-nginx/nginx
COPY --from=build-bin --chown=1000:1000 /app/cst-nginx/conf /app/cst-nginx/conf
COPY --from=build-bin --chown=1000:1000 /app/cst-nginx/html /app/cst-nginx/html
RUN apk add pcre \
    && addgroup -g 101 -S nginx \
    && adduser -S -D -H -u 101 -h /var/cache/nginx -s /sbin/nologin -G nginx -g nginx nginx \
    && mkdir -p /app/cst-nginx/logs && chown 1000:1000 /app/cst-nginx/logs \
    && ln -sf /dev/stdout //app/cst-nginx/logs/access.log \
    && ln -sf /dev/stderr /app/cst-nginx/logs/error.log

EXPOSE 80 443
STOPSIGNAL SIGQUIT
CMD ["./nginx", "-g", "daemon off;",  "-c", "/app/cst-nginx/conf/nginx.conf"]
