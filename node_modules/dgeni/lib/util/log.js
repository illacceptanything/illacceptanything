/**
 * @dgService log
 * @kind object
 * @description
 * A service for logging what the dgeni is up to
 */
module.exports = function logFactory() {
  var winston = require('winston');
  winston.cli();
  winston.level = 'info';
  return winston;
};