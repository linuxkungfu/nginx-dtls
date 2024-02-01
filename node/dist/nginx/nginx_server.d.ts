import { EnhancedEventEmitter } from "../lib/EnhancedEventEmitter";
export declare type NginxEvents = {
    died: [Error];
};
export default class NginxServer extends EnhancedEventEmitter<NginxEvents> {
    #private;
    constructor(config: any);
    run(): Promise<boolean>;
    shutdown(): void;
    private setup;
    updateConfig(): Promise<boolean>;
    reloadConfig(): Promise<boolean>;
}
//# sourceMappingURL=nginx_server.d.ts.map