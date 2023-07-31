// import * as createError from "http-errors"
// import * as express from "express";
// import rtmp from "./web/routes/rtmp";
// import HttpServer from "./web/http_server";
import config from "./config";
import {initLogger,getLogger} from "./lib/logger";
import NginxServer from "./nginx/nginx_server";
import Controller from "./controller/controller";
import { exit } from "process";
import { nginxContext } from "./nginx/nginx_context";
import Util from "./lib/util";
const logger = getLogger();
initLogger(config.Logger, Util.getServerId(), ['child']);

async function runNginxServer(shutdown:(code: string)=>void): Promise<NginxServer>{
  const nginxServer = new NginxServer(config.Nginx);
  nginxServer.on("@failure", ()=>{
    if (shutdown) {
      shutdown("-1");
    }
  });
  nginxServer.on("@exit", ()=>{
  });
  const ret = await nginxServer.run();
  if (ret == false) {
    logger.error(`start nginx process failed, server exit`);
    exit(-1);
  }
  return nginxServer;
}

function initEnv() {
  if (process.env.MASTER_URL != null && process.env.MASTER_URL.length > 0) {
    config.Master.Url = process.env.MASTER_URL || "";
  }
  if (process.env.LOCAL_IDCX != null && process.env.LOCAL_IDCX.length > 0) {
    config.IdcCode = process.env.LOCAL_IDCX;
  }
  

  if (process.env.ENABLE_REGISTER != null && process.env.ENABLE_REGISTER.length > 0) {
    if (process.env.ENABLE_REGISTER.toLowerCase() == "true") {
      config.Master.Enable = true;
    } else {
      config.Master.Enable = false;
    }
  }

  if (process.env.NGINX_LISTEN != null && process.env.NGINX_LISTEN.length > 0) {
    config.Nginx.Server.Listen = process.env.NGINX_LISTEN;
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
    config.Nginx.UpStream = [{
      Name: "slave",
      Server: [],
    }]
    upstream.forEach((value,index) => {
      if (value.length > 0) {
        const server = value.split(":");
        if (server.length !== 2) {
          logger.error(`NGINX_UPSTREAM env format error:${value}`);
        } else {
          config.Nginx.UpStream[0].Server.push({IP: server[0], Port: +server[1]});
        }
      }
      
    });
  }
  // 外部地址（允许加上端口，有端口表示代理模式）
  if (process.env.EXTERNAL_ADDR != null && process.env.EXTERNAL_ADDR.length > 0) {
    config.ExternalAddr = Util.formatAddrEnv(process.env.EXTERNAL_ADDR);
  }
  // 内网地址（允许加上端口，有端口表示代理模式）
  if (process.env.INNER_ADDR != null && process.env.INNER_ADDR.length > 0) {
    config.InnerAddr = Util.formatAddrEnv(process.env.INNER_ADDR);
  }
  if (process.env.NODE_TYPE != null && process.env.NODE_TYPE.length > 0) {
    config.NodeType = process.env.NODE_TYPE;
  }
  
  if (config.NodeType === "NGINX_DTLS_DECRYPT") {
    config.Nginx.Server.ProxySSL = false;
    config.Nginx.Server.Listen = config.Nginx.Server.Listen.replace(/ ssl /g, " ");
    // console.log(config.Nginx.Server.Listen);
    config.Nginx.Server.Listen = config.Nginx.Server.Listen.replace(/udp /g, "udp ssl ");
  } else if (config.NodeType === "NGINX_DTLS_ENCRYPT") {
    config.Nginx.Server.ProxySSL = true;
    config.Nginx.Server.Listen = config.Nginx.Server.Listen.replace(/ ssl /g, " ");
  }
  if (process.env.PROBE_UDP != null && process.env.PROBE_UDP.length > 0 ) {
    if (process.env.PROBE_UDP.toLowerCase() == "true") {
      config.Nginx.Server.ProbeUdp = true
    } else {
      config.Nginx.Server.ProbeUdp = false
    }
  }
}
function initConfig(): boolean {
  const addr = config.Nginx.Server.Listen.split(" ");
  if (addr.length == 0) {
    logger.error(`miss nginx listen port:${config.Nginx.Server.Listen}`);
    return false;
  }
  if (config.InnerAddr == null || config.InnerAddr.length == 0) {
    
    config.InnerAddr = [`${Util.getHostIp()}:${addr[0]}`];
  }
  Util.setListenPort(+addr[0]);
  return true;
}
function checkConfig() {
  if (config.Master.Enable && (config.Master.Url == null || config.Master.Url.length === 0)) {
    logger.error(`master url required`);
    exit(-1);
  }
}

(async () => {
  try {
    process.on('uncaughtException', (err: any, origin: any) => {
      logger.error(`Exception origin: ${origin}\nCaught exception: ${err.message}\n${err.stack}`);
    });
    process.on('unhandledRejection', (err: any, _p) => {
      logger.error(`Caught unhandledRejection: ${err.message}\n${err.stack}`);
    });
    const shutdown = function(code: string){
      nginxContext.nginxServer.shutdown();
      if (code !== "-1") {
        logger.info(`cst-nginx will shutdown signal:${code}`);
        exit(0);
      } else {
        logger.warn(`cst-nginx runtime error, will exit`);
        exit(+code);

      }
    }
    initEnv();
    if (initConfig() == false) {
      logger.error(`cst-nginx config error, will exit`);
      exit(-2);
    }
    checkConfig();
    let upstreamServer = [{IP:"", Port: 0}];
    if (config.Nginx.UpStream == null || config.Nginx.UpStream.length == 0) {
      config.Nginx.UpStream = [
        {
          Name: "slave",
          Server: upstreamServer,
        }
      ];
    } else {
      upstreamServer = config.Nginx.UpStream[0].Server;
    }
    const controller = new Controller(config.Master, config.ExternalAddr, config.InnerAddr, config.NodeType, upstreamServer, config.IdcCode);
    if (config.Master.Enable && await controller.init() == false) {
      logger.error("init server failed");
      exit(-1);
    }
    
    if (config.Master.Enable) {
      controller.register();
    }
    nginxContext.nginxServer = await runNginxServer(shutdown);
    // srsContext.httpServer = await runHttpServer();
    // srsContext.controller = new Controller(config.Controller, config.Http.Port);
    
    process.on('SIGINT', shutdown);
    process.on('SIGTERM', shutdown);
    process.on('SIGQUIT', shutdown);
    process.on('SIGHUP', function(code: string) {
      nginxContext.nginxServer.reloadConfig();
    });
    // const interval = await srsContext.controller.register();
    // if (interval == 0) {
    //   logger.error(`register server failed, server exit`);
    //   exit(-2);
    // }
    setInterval(async () => {
      // srsContext.controller.heartbeat();
      if (config.Master.Enable) {
        const ret = await controller.heartbeat();
        if ( ret === false) {
          logger.warn(`register is true, will register again`);
          controller.register();
        }
      }
    }, controller.heartbeatTimeout * 1000);
  } catch (e: any) {
    logger.error('error:%s stack:%s', e.message, e.stack);
  }
})();