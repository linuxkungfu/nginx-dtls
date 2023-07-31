import { getLogger, getLoggerByCategory} from "../lib/logger";
import * as path from 'path';
import { spawn, ChildProcess } from 'child_process';
import {NginxConfFile} from 'nginx-conf';
import { EnhancedEventEmitter } from "../lib/EnhancedEventEmitter";
import { time } from "console";
import { stringify } from "qs";
import { Server } from "http";
export type NginxEvents =
{
  died: [Error];
}
const rootDir = path.join(__dirname, '..', '..', '..');
const workdir = path.join(rootDir, 'nginx/objs');
const workerBin = process.env.NGINX_WORKER_BIN
	? process.env.SRS_WORKER_BIN
	: path.join(workdir, 'nginx');
const templateConfigPath = path.join(rootDir, 'nginx','conf','nginx-template.conf');
const configPath = path.join(rootDir, 'nginx','conf','nginx.conf');
const logger = getLogger();
const loggerChild = getLoggerByCategory('child');
export default class NginxServer extends EnhancedEventEmitter<NginxEvents> 
{
  #config: any;
  #child?: ChildProcess;
  #pid: number;
  #spawnDone?: boolean;
  #running: boolean;
  constructor(config: any) {
    super();
    this.#config = config;
    this.#pid = 0;
    this.#running = false;
  }
  public async run(): Promise<boolean> {
    this.#running = true;
    if (this.#config == null) {
      logger.warn(`NginxServer config is null`);
      return false;
    }
    try {
      const spawnBin = workerBin;
		  const spawnArgs: string[] = [];
      if (await this.updateConfig() == false) {
        return false;
      }
      spawnArgs.push('-c');
      spawnArgs.push(`${configPath}`);
      spawnArgs.push('-p');
      spawnArgs.push(`${rootDir}`);
      this.#child = spawn(
        // command
        spawnBin!,
        // args
        spawnArgs,
        // options
        {
          env :
          {
            ...process.env,
            NGINX_VERSION : 'v1.22'
            // Let the worker process inherit all environment variables, useful
            // if a custom and not in the path GCC is used so the user can set
            // LD_LIBRARY_PATH environment variable for runtime.
          },
          cwd: workdir,
  
          detached : false,
  
          // fd 0 (stdin)   : Just ignore it.
          // fd 1 (stdout)  : Pipe it for 3rd libraries that log their own stuff.
          // fd 2 (stderr)  : Same as stdout.
          // stdio       : [ 'ignore', 'pipe', 'pipe', ],
          shell: true,
          windowsHide : false
        });
        this.#pid = this.#child.pid ? this.#child.pid : 0;
        if (this.#pid === 0) {
          return false;
        } else {
          this.#spawnDone = true;
        }
        this.setup();
      return true;
    } catch (err: any){
      logger.error(`create nginx failed:${err?.message}, stack:${err?.stack}`);
      return false;
    }
  }
  public shutdown() {
    if (this.#pid > 0) {
      this.#running = false;
      logger.info(`nginx(${this.#pid}) shutdown`);
		  process.kill(this.#pid, "SIGQUIT");
    }
  }

  private setup() {
    this.#child!.on('exit', (code, signal) =>
		{
			this.#child = undefined;

      if (this.#running) {
        if (!this.#spawnDone) {
          logger.error('nginx start failed');
        } else {
          logger.error(
            'nginx process died unexpectedly [pid:%s, code:%s, signal:%s]',
            this.#pid, code, signal);
        }
        this.#pid = 0;
        this.emit('@failure', new TypeError('nginx exit'));
      } else {
        this.#pid = 0;
        this.emit('@exit', new TypeError('nginx exit'));
      }
      
		});
    this.#child!.stdout!.on('data', (buffer: Buffer) =>
		{
      // console.log(buffer.toString('utf8'));
      loggerChild.trace(buffer.toString('utf8'));
    });
    // In case of a worker bug, mediasoup will log to stderr.
		this.#child!.stderr!.on('data', (buffer) =>
		{
			for (const line of buffer.toString('utf8').split('\n')) {
				if (line)
        loggerChild.error(`[nginx] ${line}`);
			}
		});
  }

  async updateConfig():Promise<boolean> {
    logger.info(`update nginx config`);
    let finish: boolean = false;
    let success: boolean = false;
    NginxConfFile.create(templateConfigPath, (err, conf) => {
      try {
        finish = true;
        if (err || !conf) {
          logger.error(`load nginx config:${configPath} failed, error:${err?.message}`);
          return;
        }
        const stream = conf.nginx.stream?.[0];
        if (!stream) {
          logger.error(`nginx config lack stream block`);
          return;
        }
        conf?.die(templateConfigPath);
        conf.live(configPath);
        finish = false;
        if (this.#config.UpStream == null || this.#config.UpStream.length == 0 || this.#config.UpStream[0].Server == null || this.#config.UpStream[0].Server.length == 0) {
          logger.error(`upstream server is empty`);
          stream._remove("server");
          stream._remove("upstream");
          finish = true;
          success = true;
          conf.flush();
          return;
        }
        if (this.#config?.Server?.Listen && this.#config?.Server?.Listen!.length > 0) {
          stream!.server![0]!._remove("listen");
          stream!.server![0]!._add("listen", this.#config.Server.Listen);
        }
        stream?.server?.[0]._remove("proxy_ssl");
        if (this.#config?.Server?.ProxySSL) {
          stream?.server?.[0]._add("proxy_ssl", "on");
        } else {
          stream?.server?.[0]._add("proxy_ssl", "off");
        }
        stream?.server?.[0]._remove("probe_enable");
        if (this.#config.Server.ProbeUdp && (this.#config?.Server?.Listen.indexOf("udp") != -1 || this.#config?.Server?.Listen.indexOf("UDP") != -1)) {
          stream?.server?.[0]._add("probe_enable", "on");
        } else {
          stream?.server?.[0]._add("probe_enable", "off");
        }
        if (this.#config?.UpStream && this.#config?.UpStream.length > 0) {
          stream._remove("upstream");
          stream?.server?.[0]._remove("proxy_pass");
          this.#config?.UpStream.forEach((upstream:{Name:string, Server:[]}) => {
            if (upstream?.Server && upstream?.Server.length > 0) {
              stream?._add("upstream", upstream.Name, []);
              stream?.server?.[0]._add("proxy_pass", upstream.Name);
              upstream.Server.forEach((server:{IP:string, Port:string}) => {
                stream?.upstream![0]._add("server", `${server.IP}:${server.Port}`);  
                // stream?.upstream![0]._add("server", `172.16.204.12:30000`);  
              });
            }
          });
          

        } else {
          stream?.server?.[0]._remove("proxy_pass");
          stream._remove("upstream");
          
        }
        conf.on('flushed', () => {
          finish = true;
          logger.debug(`write nginx config success`);
        })
        conf.flush();
        success = true;
        return;
      } catch(err) {
        finish = true;
        logger.error(`create nginx conf failed:${err}`);
        return;
      }
      
    });
    while(!finish) {
      await new Promise(resolve =>setTimeout(() => resolve(1), 100));
    }
    return success;
  }

  async reloadConfig(): Promise<boolean> {
    logger.info("reload nginx config");
    const ret = await this.updateConfig();
    if (ret) {
      if (this.#pid > 0) {
        process.kill(this.#pid, 'SIGHUP');
      }
      logger.info("reload nginx config successful");
    }
    return true;
  }
}