var Package = require('./Package');

describe("Package", function() {
  describe("constructor()", function() {

    it("should complain if no name is given", function() {
      expect(function() {
        new Package();
      }).toThrow();
      expect(function() {
        new Package(['dep1', 'dep2']);
      }).toThrow();
    });

    it("should add dependencies, if provided", function() {
      var package = new Package('packageName', ['dep1', 'dep2']);
      expect(package.dependencies).toEqual(['dep1', 'dep2']);
    });

    it("should create an empty dependencies collection if no dependencies are provided", function() {
      var package = new Package('packageName');
      expect(package.dependencies).toEqual([]);
    });

    it("should complain if dependencies is not an array", function() {
      expect(function() {
        new Package('somePackage', {});
      }).toThrow();
    });
  });

  describe("isPackage", function() {
    it("should return true for instances of Package", function() {
      var realPackage = new Package('realPackage', ['dep1']);
      expect(Package.isPackage(realPackage)).toEqual(true);
    });
    it("should return true for package-like objects", function() {
      var duckPackage = {
        name: 'duckPackage',
        module: {},
        dependencies: ['dep1']
      };
      expect(Package.isPackage(duckPackage)).toEqual(true);
    });
    it("should return false for non-package-like objects", function() {
      var nonPackage = {
        name: 'nonPackage',
        //module: {},
        dependencies: ['dep1']
      };
      expect(Package.isPackage(nonPackage)).toEqual(false);
    });
    it("should return false if passed a non-object", function() {
      var nonPackage = 'packageName';
      expect(Package.isPackage(nonPackage)).toEqual(false);
    });
  });

  describe("processor()", function() {

    it("should add processors defined by an object to the processors property", function() {
      var package = new Package('packageName');
      package.processor({ name: 'testProcessor'});
      expect(package.processors[0]).toEqual('testProcessor');
    });

    it("should add processors defined by a factory function to the processors property", function() {
      var package = new Package('packageName');
      package.processor(function testProcessor() {});
      expect(package.processors[0]).toEqual('testProcessor');
    });

    it("should complain if the processorFactory does not have a name", function() {
      var package = new Package('packageName');
      expect(function() {
        package.processor(function() {});
      }).toThrow();

      expect(function() {
        package.processor({ missing: 'name'});
      }).toThrow();
    });

    it("should use the first param as the name if it is a string", function() {
      var package = new Package('packageName');
      package.processor('testProcessor', { $process: function(docs) { } });
      expect(package.processors[0]).toEqual('testProcessor');
    });

    it("should add the processor to the DI module", function() {
      var package = new Package('packageName');
      var testProcessor = function testProcessor() {};
      package.processor(testProcessor);
      expect(package.module.testProcessor).toEqual(['factory', testProcessor]);
    });
  });

  describe("factory()", function() {
    it("should complain if the factory is not a function", function() {
      var package = new Package('packageName');
      expect(function() {
        package.factory({ name: 'bad factory'});
      }).toThrow();
    });

    it("should complain if the factory does not have a name", function() {
      var package = new Package('packageName');
      expect(function() {
        package.factory(function() {});
      }).toThrow();
    });

    it("should use the first param as the name if it is a string", function() {
      var package = new Package('packageName');

      var testServiceFactory = function() {};
      package.factory('testService', testServiceFactory);
      expect(package.module.testService).toEqual(['factory', testServiceFactory]);
    });

    it("should add the service to the DI module", function() {
      var package = new Package('packageName');
      var testService = function testService() {};
      package.factory(testService);
      expect(package.module.testService).toEqual(['factory', testService]);
    });
  });


  describe("type()", function() {
    it("should complain if the constructor is not a function", function() {
      var package = new Package('packageName');
      expect(function() {
        package.type({ name: 'bad type'});
      }).toThrow();
    });

    it("should complain if the constructor does not have a name", function() {
      var package = new Package('packageName');
      expect(function() {
        package.type(function() {});
      }).toThrow();
    });

    it("should use the first param as the name if it is a string", function() {
      var package = new Package('packageName');
      var TestService = function() {};
      package.type('testService', TestService);
      expect(package.module.testService).toEqual(['type', TestService]);
    });

    it("should add the service to the DI module", function() {
      var package = new Package('packageName');

      var TestService = function TestService() {};
      package.type(TestService);
      expect(package.module.TestService).toEqual(['type', TestService]);
    });
  });


  describe("config()", function() {
    it("should add the function to the configFns property", function() {
      var package = new Package('packageName');
      var testFn = function() {};
      package.config(testFn);
      expect(package.configFns[0]).toEqual(testFn);
    });

    it("should complain if configFn is not a function", function() {
      var package = new Package('packageName');
      expect(function() {
        package.config({ some: 'non-function'});
      }).toThrow();
    });

  });

});