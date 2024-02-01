"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const axios_1 = require("axios");
const https = require("https");
const error_code_1 = require("../lib/error_code");
const logger_1 = require("../lib/logger");
const util_1 = require("../lib/util");
const logger = (0, logger_1.getLogger)();
class Controller {
    #masterUrl;
    #masterQueryPortAPI;
    #masterRegisterAPI;
    #masterHeartbeatAPI;
    #masterGetSlaveAPI;
    #idcCode;
    #minPort;
    #maxPort;
    #usernameLength;
    #externalAddr;
    #innerAddr;
    #nodeType;
    #agent;
    #upstreamServer; // 下一跳配置
    #upstreamServerStr;
    #heartbeatTimeout;
    constructor(master, externalAddr, innerAddr, nodeType, upstreamStream, idcCode) {
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
        this.#agent = axios_1.default.create({
            httpsAgent: new https.Agent({
                rejectUnauthorized: false
            })
        });
        this.#upstreamServer = upstreamStream;
        this.#heartbeatTimeout = 0;
        const serverArray = [];
        this.#upstreamServer.forEach((value, index) => {
            serverArray.push(`${value.IP}:${value.Port}`);
        });
        this.#upstreamServerStr = serverArray.toString();
    }
    get heartbeatTimeout() {
        return this.#heartbeatTimeout;
    }
    get masterQueryPortAPI() {
        return this.#masterQueryPortAPI;
    }
    get maxPort() {
        return this.#maxPort;
    }
    get usernameLength() {
        return this.#usernameLength;
    }
    async init() {
        const url = `${this.#masterUrl}${this.#masterGetSlaveAPI}`;
        const reqData = {
            idc: this.#idcCode,
            nodeEncryptType: this.#nodeType,
        };
        const res = await this.#agent.post(url, reqData);
        if (res.status != error_code_1.ErrorCode.HTTP_OK || res?.data?.code != error_code_1.ErrorCode.HTTP_OK) {
            logger.error(`[controller][init]query udp port failed, http status:${res.status}, code:${res?.data?.code}, message:${res?.data?.msg}`);
            return false;
        }
        const data = res.data.data;
        if (data == null) {
            logger.error(`[controller][init]query udp port lack data field:${JSON.stringify(res.data)}`);
            return false;
        }
        if (this.#idcCode.length === 0) {
            this.#idcCode = data.idcCode;
        }
        this.#minPort = data.minPort;
        this.#maxPort = data.maxPort;
        this.#usernameLength = data.usernameLength;
        if (data.heatBeatTimeOut != null) {
            this.#heartbeatTimeout = +data.heatBeatTimeOut;
        }
        this.#upstreamServer.splice(0, this.#upstreamServer.length);
        if (data.nextSkipUrl != null) {
            data.nextSkipUrl.forEach((value, _index) => {
                const addr = value.split(":");
                if (addr.length == 2) {
                    this.#upstreamServer.push({ IP: addr[0], Port: +addr[1] });
                }
                else {
                    logger.error(`[controller][init]get upstream server from master miss port:${value}`);
                }
            });
            this.#upstreamServerStr = data.nextSkipUrl.toString();
        }
        else {
            this.#upstreamServerStr = "";
        }
        logger.info(`[controller][init]upstream server:${JSON.stringify(this.#upstreamServer)}`);
        return true;
    }
    async register() {
        const serverId = util_1.default.getServerId();
        const reqData = {
            serverId,
            idc: this.#idcCode,
            localIp: util_1.default.getHostIp(),
            innerAddrArray: this.#innerAddr,
            externalAddrArray: this.#externalAddr,
            nodeEncryptType: this.#nodeType,
        };
        const url = `${this.#masterUrl}${this.#masterRegisterAPI}`;
        const res = await this.#agent.post(url, reqData);
        if (res.status != error_code_1.ErrorCode.HTTP_OK || res?.data?.code != error_code_1.ErrorCode.HTTP_OK) {
            logger.error(`[controller][register]failed, http status:${res.status}, code:${res?.data?.code}, message:${res?.data?.msg}`);
            return false;
        }
        logger.info(`[controller][register]idc code:${this.#idcCode} local ip:${util_1.default.getHostIp()} inner addr:${JSON.stringify(this.#innerAddr)}, external addr:${JSON.stringify(this.#externalAddr)} register success`);
        return true;
    }
    async heartbeat() {
        const serverId = util_1.default.getServerId();
        const portPeerNumber = new Map();
        portPeerNumber.set(this.#minPort, 0);
        const reqData = {
            serverId,
            idc: this.#idcCode,
            hostIp: util_1.default.getHostIp(),
            cpus: util_1.default.getCpus(),
            cpuLoad: util_1.default.getCpuLoad()[0],
            cpuUsage: util_1.default.getCpuUsage(),
            portPeerNumber: Object.fromEntries(portPeerNumber),
            rx: 0,
            tx: 0,
        };
        const url = `${this.#masterUrl}${this.#masterHeartbeatAPI}`;
        const res = await this.#agent.post(url, reqData);
        if (res.status != error_code_1.ErrorCode.HTTP_OK || res?.data?.code != error_code_1.ErrorCode.HTTP_OK) {
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
                    this.#upstreamServer.push({ IP: addr[0], Port: +addr[1] });
                }
                else {
                    logger.error(`[controller][heartbeat]next skip url error:${value}`);
                }
            }
            this.#upstreamServerStr = currentServerStr;
            process.kill(process.pid, 'SIGHUP');
        }
        return true;
    }
}
exports.default = Controller;
//# sourceMappingURL=controller.js.map