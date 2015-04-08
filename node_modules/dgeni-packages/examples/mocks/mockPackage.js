var Package = require('dgeni').Package;

module.exports = function mockPackage() {

  return new Package('mockPackage', [require('../'), require('../../jsdoc')])

  // provide a mock log service
  .factory('log', function() { return require('dgeni/lib/mocks/log')(false); })

  // provide a mock template engine for the tests
  .factory('templateEngine', function dummyTemplateEngine() {});
};
