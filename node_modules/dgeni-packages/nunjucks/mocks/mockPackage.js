var Package = require('dgeni').Package;

module.exports = function mockPackage() {

  return new Package('mockPackage', [require('../'), require('../../base')]);

};
