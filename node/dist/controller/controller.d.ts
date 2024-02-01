export default class Controller {
    #private;
    constructor(master: {
        Url: string;
        QueryPortAPI: string;
        GetSlaveAPI: string;
        RegisterAPI: string;
        HeartbeatAPI: string;
    }, externalAddr: string[], innerAddr: string[], nodeType: string, upstreamStream: {
        IP: string;
        Port: number;
    }[], idcCode: string);
    get heartbeatTimeout(): number;
    get masterQueryPortAPI(): string;
    get maxPort(): number;
    get usernameLength(): number;
    init(): Promise<boolean>;
    register(): Promise<boolean>;
    heartbeat(): Promise<boolean>;
}
//# sourceMappingURL=controller.d.ts.map