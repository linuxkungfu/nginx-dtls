import axios from 'axios';
import * as https from 'https';
import { ErrorCode } from '../lib/error_code';
import {getLogger} from "../lib/logger";
import Util from '../lib/util';
const logger = getLogger();

export default class Controller {
  #masterUrl:string;
  #masterQueryPortAPI: string;
  #masterRegisterAPI: string;
  #masterHeartbeatAPI: string;
  #masterGetSlaveAPI: string;
  #idcCode: string;
  #minPort: number;
  #maxPort: number;
  #usernameLength: number;
  #externalAddr: string[];
  #innerAddr: string[];
  #nodeType: string;
  #agent: any;
  #upstreamServer: {IP: string, Port: number}[]; // 下一跳配置
  #upstreamServerStr: string;
  #heartbeatTimeout: number;
  constructor(master: {Url:string, QueryPortAPI: string, GetSlaveAPI: string, RegisterAPI: string, HeartbeatAPI: string}, externalAddr: string[], innerAddr: string[], nodeType: string, upstreamStream: {IP: string, Port: number}[], idcCode: string) {
    this.#masterUrl = master.Url;
    this.#masterQueryPortAPI = master.QueryPortAPI;
    this.#masterGetSlaveAPI = master.GetSlaveAPI;
    this.#masterRegisterAPI = master.RegisterAPI;
    this.#masterHeartbeatAPI = master.HeartbeatAPI;
    this.#idcCode = idcCode;
    this.#minPort = 0;
    this.#maxPort = 0;
    this.#usernameLength = 0;
    this.#externalAddr = externalAddr;
    this.#innerAddr = innerAddr;
    this.#nodeType = nodeType;
    this.#agent = axios.create({
      httpsAgent: new https.Agent({  
        rejectUnauthorized: false
      })
    });
    this.#upstreamServer = upstreamStream;
    this.#heartbeatTimeout = 0;
    const serverArray: string[] = []; 
    this.#upstreamServer.forEach((value:{IP: string, Port: number}, index: number) => {
      serverArray.push(`${value.IP}:${value.Port}`);
    });
    this.#upstreamServerStr = serverArray.toString();
  }
  get heartbeatTimeout(): number {
    return this.#heartbeatTimeout;
  }
  get masterQueryPortAPI(): string {
    return this.#masterQueryPortAPI;
  }
  get maxPort(): number {
    return this.#maxPort;
  }
  get usernameLength(): number {
    return this.#usernameLength;
  }
  async init():Promise<boolean> {
    const url = `${this.#masterUrl}${this.#masterGetSlaveAPI}`;
    const reqData = {
      idc: this.#idcCode,
      nodeEncryptType: this.#nodeType,
    };
    const res: {status: any, data: any} = await this.#agent.post(url, reqData);
    if (res.status != ErrorCode.HTTP_OK || res?.data?.code != ErrorCode.HTTP_OK) {
      logger.error(`[controller][init]query udp port failed, http status:${res.status}, code:${res?.data?.code}, message:${res?.data?.msg}`);
      return false;
    }
    const data = res.data.data;
    if (data == null) {
      logger.error(`[controller][init]query udp port lack data field:${JSON.stringify(res.data)}`);
      return false;
    }
    if (this.#idcCode.length === 0) {
      this.#idcCode = data!.idcCode;
    }
    this.#minPort = data!.minPort;
    this.#maxPort = data!.maxPort;
    this.#usernameLength = data!.usernameLength;
    if (data.heatBeatTimeOut != null) {
      this.#heartbeatTimeout = +data!.heatBeatTimeOut
    }
    this.#upstreamServer.splice(0, this.#upstreamServer.length);
    if (data.nextSkipUrl != null) {
      data!.nextSkipUrl.forEach((value: string, _index: number) => {
        const addr = value.split(":");
        if (addr.length == 2) {
          this.#upstreamServer.push({IP: addr[0], Port: +addr[1]});
        } else {
          logger.error(`[controller][init]get upstream server from master miss port:${value}`);
        }
      });
      this.#upstreamServerStr = data.nextSkipUrl.toString();
    } else {
      this.#upstreamServerStr = "";
    }
    logger.info(`[controller][init]upstream server:${JSON.stringify(this.#upstreamServer)}`);
    return true;
  }
  async register(): Promise<boolean> {
    const serverId = Util.getServerId();
    const reqData = {
      serverId,
      idc: this.#idcCode,
      localIp: Util.getHostIp(),
      innerAddrArray: this.#innerAddr,
      externalAddrArray: this.#externalAddr,
      nodeEncryptType: this.#nodeType,
    };
    const url = `${this.#masterUrl}${this.#masterRegisterAPI}`;
    const res: {status: any, data: any} = await this.#agent.post(url, reqData);
    if (res.status != ErrorCode.HTTP_OK || res?.data?.code != ErrorCode.HTTP_OK) {
      logger.error(`[controller][register]failed, http status:${res.status}, code:${res?.data?.code}, message:${res?.data?.msg}`);
      return false;
    }
    logger.info(`[controller][register]idc code:${this.#idcCode} local ip:${Util.getHostIp()} inner addr:${JSON.stringify(this.#innerAddr)}, external addr:${JSON.stringify(this.#externalAddr)} register success`);
    return true;
  }

  async heartbeat(): Promise<boolean> {
    const serverId = Util.getServerId();
    const portPeerNumber = new Map<number, number>();
    portPeerNumber.set(this.#minPort, 0);
    const reqData = {
      serverId,
      idc: this.#idcCode,
      hostIp: Util.getHostIp(),
      cpus: Util.getCpus(),
      cpuLoad: Util.getCpuLoad()[0],
      cpuUsage: Util.getCpuUsage(),
      portPeerNumber: Object.fromEntries(portPeerNumber),
      rx: 0,
      tx: 0,
    };
    const url = `${this.#masterUrl}${this.#masterHeartbeatAPI}`;
    const res: {status: any, data: any} = await this.#agent.post(url, reqData);
    if (res.status != ErrorCode.HTTP_OK || res?.data?.code != ErrorCode.HTTP_OK) {
      logger.error(`[controller][heartbeat]http status:${res.status}, code:${res?.data?.code}, message:${res?.data?.msg}`);
      return false;
    }
    logger.debug(`[controller][heartbeat]response data:${JSON.stringify(res.data.data)}`);
    if (res.data.data.register === true) {
      return false;
    }
    const currentServerStr = res?.data?.data?.nextSkipUrl?.join(":");
    if (this.#upstreamServerStr !== currentServerStr) {
      logger.info(`[controller][heartbeat]upstream configuration changed from ${this.#upstreamServerStr} to ${currentServerStr}`);
      this.#upstreamServer.splice(0, this.#upstreamServer.length);
      for (let index = 0; index < res?.data?.data?.nextSkipUrl?.length; index++) {
        const value = res?.data?.data?.nextSkipUrl[index];
        const addr = value.split(":");
        if (addr.length === 2) {
          this.#upstreamServer.push({IP: addr[0], Port: +addr[1]});
        } else {
          logger.error(`[controller][heartbeat]next skip url error:${value}`);
        }
      }
      this.#upstreamServerStr = currentServerStr;
      process.kill(process.pid, 'SIGHUP');
    }
    return true;
  }
}