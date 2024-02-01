export default class Util {
    private static _localIp;
    private static _serverId;
    private static _cpus;
    private static _port;
    static getHostName(): string;
    static getHostIp(): string;
    static getServerId(): string;
    static getCpus(): number;
    static getCpuLoad(): number[];
    static getCpuUsage(): number;
    static getMemSum(): number;
    static getMemUseRate(): number;
    static setLocalIp(ip: string): void;
    static formatAddrEnv(addrStr: string): string[];
    static setListenPort(port: number): void;
}
//# sourceMappingURL=util.d.ts.map