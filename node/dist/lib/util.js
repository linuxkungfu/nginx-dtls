"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const os = require("os");
const murmurhash = require('murmurhash3');
class Util {
    static _localIp;
    static _serverId;
    static _cpus;
    static _port;
    static getHostName() {
        return os.hostname();
    }
    static getHostIp() {
        if (Util._localIp == null) {
            var interfaces = os.networkInterfaces();
            for (var devName in interfaces) {
                var iface = interfaces[devName];
                for (var i = 0; i < iface.length; i++) {
                    var alias = iface[i];
                    if (alias.family === 'IPv4' && alias.address !== '127.0.0.1' && !alias.internal) {
                        Util._localIp = alias.address;
                        break;
                    }
                }
                if (Util._localIp != null) {
                    break;
                }
            }
            if (Util._localIp == null) {
                Util._localIp = "unknown";
            }
        }
        return Util._localIp;
    }
    static getServerId() {
        if (Util._serverId == null) {
            // const mur = murmurhash.murmur128Sync(`cst_nginx_${Util.getHostName()}_${Util.getHostIp()}:${Util._port}`);
            Util._serverId = murmurhash.murmur32HexSync(`cst_nginx_${Util.getHostName()}_${Util.getHostIp()}:${Util._port}`);
            // Util._serverId = `${mur[0]}${mur[1]}${mur[2]}${mur[3]}`;
        }
        return Util._serverId;
    }
    static getCpus() {
        if (Util._cpus == null) {
            Util._cpus = os.cpus().length;
        }
        return Util._cpus;
    }
    static getCpuLoad() {
        return os.loadavg();
    }
    static getCpuUsage() {
        const cpu = process.cpuUsage();
        return (cpu.system + cpu.user) / 1000000;
    }
    static getMemSum() {
        return os.totalmem();
    }
    static getMemUseRate() {
        return os.freemem() / os.totalmem();
    }
    static setLocalIp(ip) {
        Util._localIp = ip;
    }
    static formatAddrEnv(addrStr) {
        if (addrStr == null || addrStr.length == 0) {
            return [];
        }
        addrStr = addrStr.replace(/,/g, ";");
        addrStr = addrStr.replace(/ /g, "");
        addrStr = addrStr.replace(/\t/g, "");
        return addrStr.split(";");
    }
    static setListenPort(port) {
        Util._port = port;
    }
}
exports.default = Util;
//# sourceMappingURL=util.js.map