"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getLoggerByCategory = exports.getLogger = exports.initLogger = void 0;
const log4js = require("log4js");
const timezoneOffset = -new Date().getTimezoneOffset() / 60;
const timezoneOffsetStr = timezoneOffset >= 0 ? `T+${timezoneOffset}` : `T${timezoneOffset}`;
const fileLayout = {
    "type": "pattern",
    "pattern": "[%p][%d{yyyy-MM-dd hh:mm:ss.SSS} %x{Zone}][cst-nginx][%X{X-SERVER-ID}]%m",
    tokens: {
        Zone: function (logEvent) {
            return timezoneOffsetStr;
        }
    }
};
const consoleLayout = {
    "type": "pattern",
    "pattern": "%[[%p][%d{yyyy-MM-dd hh:mm:ss.SSS} %x{Zone}][cst-nginx][%X{X-SERVER-ID}]%m%]",
    tokens: {
        Zone: function (logEvent) {
            return timezoneOffsetStr;
        }
    }
};
let logger = log4js.getLogger();
const getLogger = function () {
    return logger;
};
exports.getLogger = getLogger;
const getLoggerByCategory = function (category) {
    return log4js.getLogger(category);
};
exports.getLoggerByCategory = getLoggerByCategory;
const initLogger = function (logConf, serverId, categoryName) {
    log4js.configure({
        appenders: {
            console: { type: 'console', layout: consoleLayout },
            access: { type: 'dateFile', filename: `${logConf.Dir}/${logConf.AccessFileName}`, layout: fileLayout },
            app: { type: 'file', filename: `${logConf.Dir}/${logConf.FileName}`, maxLogSize: logConf.MaxLogSize, numBackups: logConf.NumBackups, layout: fileLayout },
            errorFile: { type: 'file', filename: `${logConf.Dir}/${logConf.ErrorFileName}`, maxLogSize: logConf.MaxLogSize, numBackups: logConf.NumBackups, layout: fileLayout },
            errors: { type: 'logLevelFilter', level: 'error', appender: 'errorFile', layout: fileLayout }
        },
        categories: {
            default: { appenders: ['app', 'errors', 'console'], level: logConf.Level, enableCallStack: true },
            child: { appenders: ['app', 'errors', 'console'], level: logConf.Level, enableCallStack: false },
            http: { appenders: ['access'], level: 'info', enableCallStack: true }
        }
    });
    logger.addContext('X-SERVER-ID', serverId);
    for (const name in categoryName) {
        getLoggerByCategory(name).addContext('X-SERVER-ID', serverId);
    }
    logger = log4js.getLogger();
    const infoLevel = log4js.levels.getLevel("debug");
    console.info = function (...args) {
        logger._log(infoLevel, args);
    };
    const warnLevel = log4js.levels.getLevel("warn");
    console.warn = function (...args) {
        logger._log(warnLevel, args);
    };
    const errorLevel = log4js.levels.getLevel("warn");
    console.error = function (...args) {
        logger._log(errorLevel, args);
    };
};
exports.initLogger = initLogger;
//# sourceMappingURL=logger.js.map