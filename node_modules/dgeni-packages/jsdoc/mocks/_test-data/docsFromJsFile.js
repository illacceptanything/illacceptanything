module.exports = [
{
  startingLine : 3,
  content:
    "@ngdoc object\n" +
    "@name ng/$controllerProvider\n" +
    "@description\n" +
    "The {@link ng/$controller $controller service} is used by Angular to create new\n" +
    "controllers.\n" +
    "\n" +
    "This provider allows controller registration via the\n" +
    "{@link ng/$controllerProvider#methods_register register} method."
},
{
  startingLine: 18,
  content:
    "@ngdoc function\n" +
    "@name ng/$controllerProvider#register\n" +
    "@methodOf ng/$controllerProvider\n" +
    "@param {string|Object} name Controller name, or an object map of controllers where the keys are\n" +
    "   the names and the values are the constructors.\n" +
    "@param {Function|Array} constructor Controller constructor fn (optionally decorated with DI\n" +
    "   annotations in the array notation).",
},
{
  startingLine: 39,
  content:
     "@ngdoc function\n" +
     "@name ng/$controller\n" +
     "@requires $injector\n" +
    "\n" +
     "@param {Function|string} constructor If called with a function then it's considered to be the\n" +
     "   controller constructor function. Otherwise it's considered to be a string which is used\n" +
     "   to retrieve the controller constructor using the following steps:\n" +
    "\n" +
     "   * check if a controller with given name is registered via `$controllerProvider`\n" +
     "   * check if evaluating the string on the current scope returns a constructor\n" +
     "   * check `window[constructor]` on the global `window` object\n" +
    "\n" +
     "@param {Object} locals Injection locals for Controller.\n" +
     "@return {Object} Instance of given controller.\n" +
    "\n" +
     "@description\n" +
     "`$controller` service is responsible for instantiating controllers.\n" +
    "\n" +
     "It's just a simple call to {@link AUTO/$injector $injector}, but extracted into\n" +
     "a service, so that one can override this service with {@link https://gist.github.com/1649788\n" +
     "BC version}."
}];