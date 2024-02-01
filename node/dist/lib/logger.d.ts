import * as log4js from 'log4js';
declare const getLogger: () => log4js.Logger;
declare const getLoggerByCategory: (category: string) => log4js.Logger;
declare const initLogger: (logConf: any, serverId: string, categoryName: [string]) => void;
export { initLogger, getLogger, getLoggerByCategory };
//# sourceMappingURL=logger.d.ts.map