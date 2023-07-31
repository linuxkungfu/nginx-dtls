import NginxServer from "./nginx_server";
export class NginxContext {
  #nginxServer: NginxServer | any;
  constructor() {
    this.#nginxServer = null;
  }
  get nginxServer(): NginxServer | any {
    return this.#nginxServer;
  }
  set nginxServer(nginxServer: NginxServer) {
    this.#nginxServer = nginxServer;
  }
}

export const nginxContext = new NginxContext();