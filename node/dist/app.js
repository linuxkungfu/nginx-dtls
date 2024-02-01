"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
// import * as createError from "http-errors"
// import * as express from "express";
// import rtmp from "./web/routes/rtmp";
// import HttpServer from "./web/http_server";
const config_1 = require("./config");
const logger_1 = require("./lib/logger");
const nginx_server_1 = require("./nginx/nginx_server");
const controller_1 = require("./controller/controller");
const process_1 = require("process");
const nginx_context_1 = require("./nginx/nginx_context");
const util_1 = require("./lib/util");
const logger = (0, logger_1.getLogger)();
(0, logger_1.initLogger)(config_1.default.Logger, util_1.default.getServerId(), ['child']);
async function runNginxServer(shutdown) {
    const nginxServer = new nginx_server_1.default(config_1.default.Nginx);
    nginxServer.on("@failure", () => {
        if (shutdown) {
            shutdown("-1");
        }
    });
    nginxServer.on("@exit", () => {
    });
    const ret = await nginxServer.run();
    if (ret == false) {
        logger.error(`start nginx process failed, server exit`);
        (0, process_1.exit)(-1);
    }
    return nginxServer;
}
function initEnv() {
    if (process.env.MASTER_URL != null && process.env.MASTER_URL.length > 0) {
        config_1.default.Master.Url = process.env.MASTER_URL || "";
    }
    if (process.env.LOCAL_IDCX != null && process.env.LOCAL_IDCX.length > 0) {
        config_1.default.IdcCode = process.env.LOCAL_IDCX;
    }
    if (process.env.ENABLE_REGISTER != null && process.env.ENABLE_REGISTER.length > 0) {
        if (process.env.ENABLE_REGISTER.toLowerCase() == "true") {
            config_1.default.Master.Enable = true;
        }
        else {
            config_1.default.Master.Enable = false;
        }
    }
    if (process.env.NGINX_LISTEN != null && process.env.NGINX_LISTEN.length > 0) {
        config_1.default.Nginx.Server.Listen = process.env.NGINX_LISTEN;
    }
    // if (process.env.NGINX_PROXY_SSL != null && process.env.NGINX_PROXY_SSL.length > 0) {
    //   if(process.env.NGINX_PROXY_SSL.toLowerCase() == "on") {
    //     config.Nginx.Server.ProxySSL = true;
    //   } else {
    //     config.Nginx.Server.ProxySSL = false;
    //   }
    // }
    if (process.env.NGINX_UPSTREAM != null && process.env.NGINX_UPSTREAM.length > 0) {
        const upstream = process.env.NGINX_UPSTREAM.split(";");
        config_1.default.Nginx.UpStream = [{
                Name: "slave",
                Server: [],
            }];
        upstream.forEach((value, index) => {
            if (value.length > 0) {
                const server = value.split(":");
                if (server.length !== 2) {
                    logger.error(`NGINX_UPSTREAM env format error:${value}`);
                }
                else {
                    config_1.default.Nginx.UpStream[0].Server.push({ IP: server[0], Port: +server[1] });
                }
            }
        });
    }
    // 外部地址（允许加上端口，有端口表示代理模式）
    if (process.env.EXTERNAL_ADDR != null && process.env.EXTERNAL_ADDR.length > 0) {
        config_1.default.ExternalAddr = util_1.default.formatAddrEnv(process.env.EXTERNAL_ADDR);
    }
    // 内网地址（允许加上端口，有端口表示代理模式）
    if (process.env.INNER_ADDR != null && process.env.INNER_ADDR.length > 0) {
        config_1.default.InnerAddr = util_1.default.formatAddrEnv(process.env.INNER_ADDR);
    }
    if (process.env.NODE_TYPE != null && process.env.NODE_TYPE.length > 0) {
        config_1.default.NodeType = process.env.NODE_TYPE;
    }
    if (config_1.default.NodeType === "NGINX_DTLS_DECRYPT") {
        config_1.default.Nginx.Server.ProxySSL = false;
        config_1.default.Nginx.Server.Listen = config_1.default.Nginx.Server.Listen.replace(/ ssl /g, " ");
        // console.log(config.Nginx.Server.Listen);
        config_1.default.Nginx.Server.Listen = config_1.default.Nginx.Server.Listen.replace(/udp /g, "udp ssl ");
    }
    else if (config_1.default.NodeType === "NGINX_DTLS_ENCRYPT") {
        config_1.default.Nginx.Server.ProxySSL = true;
        config_1.default.Nginx.Server.Listen = config_1.default.Nginx.Server.Listen.replace(/ ssl /g, " ");
    }
    if (process.env.PROBE_UDP != null && process.env.PROBE_UDP.length > 0) {
        if (process.env.PROBE_UDP.toLowerCase() == "true") {
            config_1.default.Nginx.Server.ProbeUdp = true;
        }
        else {
            config_1.default.Nginx.Server.ProbeUdp = false;
        }
    }
}
function initConfig() {
    const addr = config_1.default.Nginx.Server.Listen.split(" ");
    if (addr.length == 0) {
        logger.error(`miss nginx listen port:${config_1.default.Nginx.Server.Listen}`);
        return false;
    }
    if (config_1.default.InnerAddr == null || config_1.default.InnerAddr.length == 0) {
        config_1.default.InnerAddr = [`${util_1.default.getHostIp()}:${addr[0]}`];
    }
    util_1.default.setListenPort(+addr[0]);
    return true;
}
function checkConfig() {
    if (config_1.default.Master.Enable && (config_1.default.Master.Url == null || config_1.default.Master.Url.length === 0)) {
        logger.error(`master url required`);
        (0, process_1.exit)(-1);
    }
}
(async () => {
    try {
        process.on('uncaughtException', (err, origin) => {
            logger.error(`Exception origin: ${origin}\nCaught exception: ${err.message}\n${err.stack}`);
        });
        process.on('unhandledRejection', (err, _p) => {
            logger.error(`Caught unhandledRejection: ${err.message}\n${err.stack}`);
        });
        const shutdown = function (code) {
            nginx_context_1.nginxContext.nginxServer.shutdown();
            if (code !== "-1") {
                logger.info(`cst-nginx will shutdown signal:${code}`);
                (0, process_1.exit)(0);
            }
            else {
                logger.warn(`cst-nginx runtime error, will exit`);
                (0, process_1.exit)(+code);
            }
        };
        initEnv();
        if (initConfig() == false) {
            logger.error(`cst-nginx config error, will exit`);
            (0, process_1.exit)(-2);
        }
        checkConfig();
        let upstreamServer = [{ IP: "", Port: 0 }];
        if (config_1.default.Nginx.UpStream == null || config_1.default.Nginx.UpStream.length == 0) {
            config_1.default.Nginx.UpStream = [
                {
                    Name: "slave",
                    Server: upstreamServer,
                }
            ];
        }
        else {
            upstreamServer = config_1.default.Nginx.UpStream[0].Server;
        }
        const controller = new controller_1.default(config_1.default.Master, config_1.default.ExternalAddr, config_1.default.InnerAddr, config_1.default.NodeType, upstreamServer, config_1.default.IdcCode);
        if (config_1.default.Master.Enable && await controller.init() == false) {
            logger.error("init server failed");
            (0, process_1.exit)(-1);
        }
        if (config_1.default.Master.Enable) {
            controller.register();
        }
        nginx_context_1.nginxContext.nginxServer = await runNginxServer(shutdown);
        // srsContext.httpServer = await runHttpServer();
        // srsContext.controller = new Controller(config.Controller, config.Http.Port);
        process.on('SIGINT', shutdown);
        process.on('SIGTERM', shutdown);
        process.on('SIGQUIT', shutdown);
        process.on('SIGHUP', function (code) {
            nginx_context_1.nginxContext.nginxServer.reloadConfig();
        });
        // const interval = await srsContext.controller.register();
        // if (interval == 0) {
        //   logger.error(`register server failed, server exit`);
        //   exit(-2);
        // }
        setInterval(async () => {
            // srsContext.controller.heartbeat();
            if (config_1.default.Master.Enable) {
                const ret = await controller.heartbeat();
                if (ret === false) {
                    logger.warn(`register is true, will register again`);
                    controller.register();
                }
            }
        }, controller.heartbeatTimeout * 1000);
    }
    catch (e) {
        logger.error('error:%s stack:%s', e.message, e.stack);
    }
})();
//# sourceMappingURL=app.js.map