describe("log", function() {
  it("should wrap the winston library", function() {

    var winston = require('winston');
    spyOn(winston, 'cli');
    var log = require('./log')();
    expect(winston.cli).toHaveBeenCalled();
    expect(winston.level).toEqual('info');
  });
});