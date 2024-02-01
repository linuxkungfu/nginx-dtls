"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Response = exports.ErrorCode = void 0;
var ErrorCode;
(function (ErrorCode) {
    ErrorCode[ErrorCode["SUCCESS"] = 0] = "SUCCESS";
    ErrorCode[ErrorCode["FAIL"] = 1] = "FAIL";
    ErrorCode[ErrorCode["UNKNOWN"] = 2] = "UNKNOWN";
    ErrorCode[ErrorCode["INNER_ERROR"] = 3] = "INNER_ERROR";
    ErrorCode[ErrorCode["IP_FILTER"] = 4] = "IP_FILTER";
    ErrorCode[ErrorCode["SERVER_BUSY"] = 5] = "SERVER_BUSY";
    ErrorCode[ErrorCode["MISS_PARAMETER"] = 6] = "MISS_PARAMETER";
    ErrorCode[ErrorCode["OUTERNETIP_ERROR"] = 7] = "OUTERNETIP_ERROR";
    ErrorCode[ErrorCode["HTTP_OK"] = 200] = "HTTP_OK";
    ErrorCode[ErrorCode["HTTP_BAD_REQUEST_ERROR"] = 400] = "HTTP_BAD_REQUEST_ERROR";
    ErrorCode[ErrorCode["HTTP_UNAUTHORIZED_ERROR"] = 401] = "HTTP_UNAUTHORIZED_ERROR";
    ErrorCode[ErrorCode["HTTP_PAYMENT_REQUIRED_ERROR"] = 402] = "HTTP_PAYMENT_REQUIRED_ERROR";
    ErrorCode[ErrorCode["HTTP_FORBIDDEN_ERROR"] = 403] = "HTTP_FORBIDDEN_ERROR";
    ErrorCode[ErrorCode["HTTP_NOT_FOUND_ERROR"] = 404] = "HTTP_NOT_FOUND_ERROR";
    ErrorCode[ErrorCode["HTTP_INNER_ERROR"] = 500] = "HTTP_INNER_ERROR";
})(ErrorCode = exports.ErrorCode || (exports.ErrorCode = {}));
;
class Response {
    /**
     * SUCCESS
     * @param data
     * @returns {{code: number, data: *}}
     */
    static success(data) {
        return {
            code: ErrorCode.SUCCESS,
            data: data
        };
    }
    /**
     *FAILED
     * @param code
     * @param message
     * @returns {{code: *, message: *}}
     */
    static fail(code, message) {
        return {
            code: code,
            message: message
        };
    }
}
exports.Response = Response;
//# sourceMappingURL=error_code.js.map