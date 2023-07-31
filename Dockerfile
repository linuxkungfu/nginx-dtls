# FROM  harbor-g42c.corp.matrx.team/baselibrary/node:16-slim as build-env
# FROM  node:16-slim as build-env
# FROM node:14-alpine3.13 as build-env
FROM node:14-alpine3.15 as build-env
# USER 0
# RUN echo "${TIME_ZONE}" > /etc/timezone \
#     && apt-get update  && apt-get install -y libpcre3-dev zlib1g-dev perl python3-pip gcc make g++ patch unzip perl git tcl cmake pkg-config \
#     && ln -sf /usr/share/zoneinfo/${TIME_ZONE} /etc/localtime
RUN apk update && apk add --no-cache python3 tzdata build-base pcre-dev zlib-dev make perl linux-headers

FROM build-env as build-bin
WORKDIR /app/cst-nginx
# RUN apk update && apk add --no-cache build-base pcre-dev zlib-dev make perl linux-headers
COPY nginx nginx
COPY node node
COPY npm-scripts.js npm-scripts.js
COPY package-lock.json package-lock.json
COPY package.json package.json

RUN npm cache clean --force \
    && npm cache verify \
    && npm install --unsafe-perm \
    && npm run typescript:build
# from buildpack-deps:buster

FROM build-env
WORKDIR /app/cst-nginx
RUN apk add pcre \
    && addgroup -g 101 -S nginx \
    && adduser -S -D -H -u 101 -h /var/cache/nginx -s /sbin/nologin -G nginx -g nginx nginx \
    && mkdir -p /app/cst-nginx/logs 
    # && ln -sfT /dev/console /app/cst-nginx/logs/access.log \
    # && ln -sfT /dev/console /app/cst-nginx/logs/error.log 
    # && ln -sfT /dev/stdout /app/cst-nginx/logs/access.log \
    # && ln -sfT /dev/stderr /app/cst-nginx/logs/error.log 
    # && chown -R nginx:nginx /app/cst-nginx
# USER 0
# COPY --from=build-bin --chown=nginx:nginx /app/cst-nginx/nginx/objs/nginx /app/cst-nginx/nginx/objs/nginx
# COPY --from=build-bin --chown=nginx:nginx /app/cst-nginx/nginx/conf /app/cst-nginx/nginx/conf
# COPY --from=build-bin --chown=nginx:nginx /app/cst-nginx/nginx/html /app/cst-nginx/nginx/html
# COPY --from=build-bin --chown=nginx:nginx /app/cst-nginx/node/dist /app/cst-nginx/node/dist
# COPY --from=build-bin --chown=nginx:nginx /app/cst-nginx/package.json /app/cst-nginx/package.json
# COPY --from=build-bin --chown=nginx:nginx /app/cst-nginx/npm-scripts.js /app/cst-nginx/npm-scripts.js
# COPY --from=build-bin --chown=nginx:nginx /app/cst-nginx/node_modules /app/cst-nginx/node_modules
COPY --from=build-bin /app/cst-nginx/nginx/objs/nginx /app/cst-nginx/nginx/objs/nginx
COPY --from=build-bin /app/cst-nginx/nginx/conf /app/cst-nginx/nginx/conf
COPY --from=build-bin /app/cst-nginx/nginx/html /app/cst-nginx/nginx/html
COPY --from=build-bin /app/cst-nginx/node/dist /app/cst-nginx/node/dist
COPY --from=build-bin /app/cst-nginx/package.json /app/cst-nginx/package.json
COPY --from=build-bin /app/cst-nginx/npm-scripts.js /app/cst-nginx/npm-scripts.js
COPY --from=build-bin /app/cst-nginx/node_modules /app/cst-nginx/node_modules



EXPOSE 25156 25157
STOPSIGNAL SIGQUIT
CMD ["npm", "start"]
