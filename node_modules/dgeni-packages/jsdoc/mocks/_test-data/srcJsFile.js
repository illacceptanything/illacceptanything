module.exports = 
"'use strict';\n" +
"\n" +
"/**\n" +
" * @ngdoc object\n" +
" * @name ng/$controllerProvider\n" +
" * @description\n" +
" * The {@link ng/$controller $controller service} is used by Angular to create new\n" +
" * controllers.\n" +
" *\n" +
" * This provider allows controller registration via the\n" +
" * {@link ng/$controllerProvider#methods_register register} method.\n" +
" */\n" +
"function $ControllerProvider() {\n" +
"  var controllers = {},\n" +
"      CNTRL_REG = /^(\\S+)(\\s+as\\s+(\\w+))?$/;\n" +
"\n" +
"\n" +
"  /**\n" +
"   * @ngdoc function\n" +
"   * @name ng/$controllerProvider#register\n" +
"   * @methodOf ng/$controllerProvider\n" +
"   * @param {string|Object} name Controller name, or an object map of controllers where the keys are\n" +
"   *    the names and the values are the constructors.\n" +
"   * @param {Function|Array} constructor Controller constructor fn (optionally decorated with DI\n" +
"   *    annotations in the array notation).\n" +
"   */\n" +
"  this.register = function(name, constructor) {\n" +
"    assertNotHasOwnProperty(name, 'controller');\n" +
"    if (isObject(name)) {\n" +
"      extend(controllers, name);\n" +
"    } else {\n" +
"      controllers[name] = constructor;\n" +
"    }\n" +
"  };\n" +
"\n" +
"\n" +
"  this.$get = ['$injector', '$window', function($injector, $window) {\n" +
"\n" +
"    /**\n" +
"     * @ngdoc function\n" +
"     * @name ng/$controller\n" +
"     * @requires $injector\n" +
"     *\n" +
"     * @param {Function|string} constructor If called with a function then it's considered to be the\n" +
"     *    controller constructor function. Otherwise it's considered to be a string which is used\n" +
"     *    to retrieve the controller constructor using the following steps:\n" +
"     *\n" +
"     *    * check if a controller with given name is registered via `$controllerProvider`\n" +
"     *    * check if evaluating the string on the current scope returns a constructor\n" +
"     *    * check `window[constructor]` on the global `window` object\n" +
"     *\n" +
"     * @param {Object} locals Injection locals for Controller.\n" +
"     * @return {Object} Instance of given controller.\n" +
"     *\n" +
"     * @description\n" +
"     * `$controller` service is responsible for instantiating controllers.\n" +
"     *\n" +
"     * It's just a simple call to {@link AUTO/$injector $injector}, but extracted into\n" +
"     * a service, so that one can override this service with {@link https://gist.github.com/1649788\n" +
"     * BC version}.\n" +
"     */\n" +
"    return function(expression, locals) {\n" +
"      var instance, match, constructor, identifier;\n" +
"\n" +
"      if(isString(expression)) {\n" +
"        match = expression.match(CNTRL_REG),\n" +
"        constructor = match[1],\n" +
"        identifier = match[3];\n" +
"        expression = controllers.hasOwnProperty(constructor)\n" +
"            ? controllers[constructor]\n" +
"            : getter(locals.$scope, constructor, true) || getter($window, constructor, true);\n" +
"\n" +
"        assertArgFn(expression, constructor, true);\n" +
"      }\n" +
"\n" +
"      instance = $injector.instantiate(expression, locals);\n" +
"\n" +
"      if (identifier) {\n" +
"        if (!(locals && typeof locals.$scope == 'object')) {\n" +
"          throw minErr('$controller')('noscp',\n" +
"              \"Cannot export controller '{0}' as '{1}'! No $scope object provided via `locals`.\",\n" +
"              constructor || expression.name, identifier);\n" +
"        }\n" +
"\n" +
"        locals.$scope[identifier] = instance;\n" +
"      }\n" +
"\n" +
"      return instance;\n" +
"    };\n" +
"  }];\n" +
"}";