"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const addrs = [];
exports.default = {
    Logger: {
        // trace debug info warn error
        Level: 'trace',
        Dir: 'logs',
        FileName: 'cst-nginx.log',
        AccessFileName: 'cst-nginx-access.log',
        ErrorFileName: 'cst-nginx-error.log',
        MaxLogSize: 419430400,
        NumBackups: 10
    },
    Master: {
        Url: "",
        QueryPortAPI: "/fastMaster/v1/getPortRange",
        GetSlaveAPI: "/fastMaster/v1/getSlaveConfig",
        RegisterAPI: "/fastMaster/v1/registerUdp",
        HeartbeatAPI: "/fastMaster/v1/heatBeat",
        Enable: false,
    },
    // NGINX_DTLS_ENCRYPT,NGINX_DTLS_DECRYPT,NGINX_TRANPARENT,SLAVE
    NodeType: "NGINX_DTLS_ENCRYPT",
    ExternalAddr: addrs,
    InnerAddr: addrs,
    IdcCode: "",
    Nginx: {
        Server: {
            Listen: "25157 udp reuseport",
            ProxySSL: true,
            ProbeUdp: true,
        },
        UpStream: [
            {
                Name: "slave",
                Server: [
                    {
                        IP: "172.16.202.64",
                        Port: 25157,
                    },
                ]
            }
        ]
    }
};
//# sourceMappingURL=config.js.map