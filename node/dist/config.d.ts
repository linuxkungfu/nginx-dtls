declare const _default: {
    Logger: {
        Level: string;
        Dir: string;
        FileName: string;
        AccessFileName: string;
        ErrorFileName: string;
        MaxLogSize: number;
        NumBackups: number;
    };
    Master: {
        Url: string;
        QueryPortAPI: string;
        GetSlaveAPI: string;
        RegisterAPI: string;
        HeartbeatAPI: string;
        Enable: boolean;
    };
    NodeType: string;
    ExternalAddr: string[];
    InnerAddr: string[];
    IdcCode: string;
    Nginx: {
        Server: {
            Listen: string;
            ProxySSL: boolean;
            ProbeUdp: boolean;
        };
        UpStream: {
            Name: string;
            Server: {
                IP: string;
                Port: number;
            }[];
        }[];
    };
};
export default _default;
//# sourceMappingURL=config.d.ts.map