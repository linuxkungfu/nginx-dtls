"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.nginxContext = exports.NginxContext = void 0;
class NginxContext {
    #nginxServer;
    constructor() {
        this.#nginxServer = null;
    }
    get nginxServer() {
        return this.#nginxServer;
    }
    set nginxServer(nginxServer) {
        this.#nginxServer = nginxServer;
    }
}
exports.NginxContext = NginxContext;
exports.nginxContext = new NginxContext();
//# sourceMappingURL=nginx_context.js.map