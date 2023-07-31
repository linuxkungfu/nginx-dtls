export enum ErrorCode {
  SUCCESS = 0,
  FAIL = 1,
  UNKNOWN = 2,
  INNER_ERROR = 3,
  IP_FILTER = 4,
  SERVER_BUSY = 5,
  MISS_PARAMETER = 6,
  OUTERNETIP_ERROR = 7,

  HTTP_OK = 200,

  HTTP_BAD_REQUEST_ERROR = 400,
  HTTP_UNAUTHORIZED_ERROR = 401,
  HTTP_PAYMENT_REQUIRED_ERROR = 402,
  HTTP_FORBIDDEN_ERROR = 403,
  HTTP_NOT_FOUND_ERROR = 404,

  HTTP_INNER_ERROR = 500,
};

export class Response {
  /**
   * SUCCESS
   * @param data
   * @returns {{code: number, data: *}}
   */
  static success(data: any) {
      return {
          code: ErrorCode.SUCCESS,
          data: data
      }
  }

  /**
   *FAILED
   * @param code
   * @param message
   * @returns {{code: *, message: *}}
   */
  static fail(code: ErrorCode, message: string) {
      return {
          code: code,
          message: message
      }
  }
}