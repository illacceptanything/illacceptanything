module.exports = function createMockLog(logToConsole) {
  var mockLog = jasmine.createSpyObj('mockLog', ['silly', 'debug', 'info', 'warn', 'error']);
  if ( logToConsole ) {
    mockLog.silly.and.callFake(console.log);
    mockLog.debug.and.callFake(console.log);
    mockLog.info.and.callFake(console.log);
    mockLog.warn.and.callFake(console.log);
    mockLog.error.and.callFake(console.log);
  }
  return mockLog;
};
