import * as os from 'os';
const murmurhash = require('murmurhash3');
export default class Util {
  private static _localIp: string;
  private static _serverId: string;
  private static _cpus: number;
  private static _port: number;
  public static getHostName(): string {
    return os.hostname();
  }
  public static getHostIp(): string {
    if (Util._localIp == null) {
      var interfaces = os.networkInterfaces();
      for (var devName in interfaces) {
        var iface = interfaces[devName];
        for (var i = 0; i < iface!.length; i++) {
          var alias = iface![i];
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

  public static getServerId(): string {
    if (Util._serverId == null) {
      // const mur = murmurhash.murmur128Sync(`cst_nginx_${Util.getHostName()}_${Util.getHostIp()}:${Util._port}`);
      Util._serverId = murmurhash.murmur32HexSync(`cst_nginx_${Util.getHostName()}_${Util.getHostIp()}:${Util._port}`);
      // Util._serverId = `${mur[0]}${mur[1]}${mur[2]}${mur[3]}`;
    }
    return Util._serverId;
  }
  public static getCpus(): number {
    if (Util._cpus == null) {
      Util._cpus = os.cpus().length;
    }
    return Util._cpus;
  }
  public static getCpuLoad(): number[] {
    return os.loadavg();
  }
  public static getCpuUsage(): number {
    const cpu = process.cpuUsage();
    return (cpu.system + cpu.user) / 1000000 ;
  }
  public static getMemSum(): number {
    return os.totalmem();
  }
  public static getMemUseRate(): number {
    return os.freemem()/ os.totalmem();
  }
  public static setLocalIp(ip: string) {
    Util._localIp = ip;
  }
  public static formatAddrEnv(addrStr: string): string[] {
    if (addrStr == null || addrStr.length == 0) {
      return [];
    }
    addrStr = addrStr.replace(/,/g, ";");
    addrStr = addrStr.replace(/ /g, "");
    addrStr = addrStr.replace(/\t/g, "");
    return addrStr.split(";");
  }
  public static setListenPort(port: number) {
    Util._port = port;
  }
}