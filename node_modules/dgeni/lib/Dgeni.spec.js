var Dgeni = require('./Dgeni');

describe("Dgeni", function() {
  var dgeni, mockLogger;

  beforeEach(function() {
    mockLogger = jasmine.createSpyObj('log', ['error', 'warning', 'info', 'debug', 'silly']);
    dgeni = new Dgeni();
    var mockLoggerPackage = dgeni.package('mockLogger');
    mockLoggerPackage.factory(function log() { return mockLogger; });
  });

  describe("constructor", function() {
    it("should accept an array of packages to load", function() {
      var package1 = new Dgeni.Package('package1');
      var package2 = new Dgeni.Package('package2');
      dgeni = new Dgeni([package1, package2]);
      expect(dgeni.packages.package1).toBe(package1);
      expect(dgeni.packages.package2).toBe(package2);
    });

    it("should complain if the packages parameter is not an array", function() {
      expect(function() {
        new new Dgeni('bad-param');
      }).toThrow();
    });
  });

  describe("package()", function() {
    it("should add the package to the packages property", function() {
      var testPackage = new Dgeni.Package('test-package');
      dgeni.package(testPackage);
      expect(dgeni.packages['test-package']).toEqual(testPackage);
    });
    it("should create a new package if passed a string", function() {
      var newPackage = dgeni.package('test-package');
      expect(Dgeni.Package.isPackage(newPackage)).toBeTruthy();
    });
    it("should throw an error if the not passed an instance of Package or a string name", function() {
      expect(function() {
        dgeni.package({});
      }).toThrow();
    });
    it("should pass dependencies through to the new package", function() {
      var newPackage = dgeni.package('test-package', ['dep1', 'dep2']);
      expect(newPackage.dependencies).toEqual(['dep1', 'dep2']);
    });

    it("should load up package dependencies that are defined inline", function(done) {
      var log = [];
      var a = new Dgeni.Package('a').processor(function aProcessor() {
        return { $process: function() { log.push('a'); } };
      });
      var b = new Dgeni.Package('b', [a]);
      dgeni.package(b);
      expect(b.dependencies).toEqual([a]);
      expect(b.namedDependencies).toEqual(['a']);
      dgeni.generate().then(function() {
        expect(log).toEqual(['a']);
        done();
      });
    });

    it("should not load a dependency that is already loaded", function() {
      var log = [];

      // Load package a1, with name 'a'
      var a1 = new Dgeni.Package('a').processor({ name: 'a', $process: function() { log.push('a1');} });
      dgeni.package(a1);

      // Load package b with inline depencency on a2, which also has name 'a'
      // This second 'a' package (i.e. a2) should nt get loaded
      var a2 = new Dgeni.Package('a').processor({ name: 'a', $process: function() { log.push('a2');} });
      var b = new Dgeni.Package('b', [a2]);

      dgeni.package(b);

      expect(b.dependencies).toEqual([a2]);
      expect(b.namedDependencies).toEqual(['a']);
      dgeni.generate().then(function() {
        expect(log).toEqual(['a1']);
        done();
      });
    });

    it("should not modify the `dependencies` property of a package", function() {
      var a = new Dgeni.Package('a').processor({ name: 'a', $process: function() { } });
      var b = new Dgeni.Package('b', [a]).processor({ name: 'a', $process: function() { } });
      dgeni.package(b);
      expect(b.dependencies).toEqual([a]);
      expect(b.namedDependencies).toEqual(['a']);

    });
  });

  describe("configureInjector", function() {

    it("should return the configured injector", function() {
      var injector = dgeni.configureInjector();
      expect(injector.get).toEqual(jasmine.any(Function));
    });

    it("should add some basic shared services to the injector", function() {
      var injector = dgeni.configureInjector();

      expect(injector.get('dgeni')).toEqual(jasmine.any(Object));
      expect(injector.get('log')).toEqual(jasmine.any(Object));
      expect(injector.get('log').debug).toEqual(jasmine.any(Function));
      expect(injector.get('getInjectables')).toEqual(jasmine.any(Function));
    });

    it("should set stop on error defaults", function() {
      var stopOnProcessingError, stopOnValidationError;
      dgeni.package('testPackage').config(function(dgeni) {
        stopOnProcessingError = dgeni.stopOnProcessingError;
        stopOnValidationError = dgeni.stopOnValidationError;
      });
      var injector = dgeni.configureInjector();
      expect(stopOnProcessingError).toBe(true);
      expect(stopOnValidationError).toBe(true);
    });
  });

  describe("generate()", function() {

    describe("packages", function() {

      it("should add services from packages in the correct package dependency order", function(done) {
        var log = [];
        dgeni.package('test1', ['test2'])
          .factory(function testValue() { return 'test 1'; });
        dgeni.package('test2')
          .factory(function testValue() { return 'test 2'; });
        dgeni.package('test3', ['test1', 'test2'])
          .processor(function test3Processor(testValue) {
            return {
              $process: function(docs) {
                log.push(testValue); }
            };
          });
        dgeni.package('test4', ['test3'])
          .processor(function test3Processor(testValue) {
            return {
              $process: function(docs) {
                log.push(testValue + '(overridden)'); }
            };
          });
        dgeni.generate()
          .then(function() {
            expect(log).toEqual(['test 1(overridden)']);
          })
          .finally(done);
      });

      it("should complain if the two packages have the same name", function() {
        dgeni.package('test');
        expect(function() {
          dgeni.package('test');
        }).toThrow();
      });

    });


    describe("config blocks", function() {

      it("should run the config functions in the correct package dependency order", function(done) {
        var log = [];
        dgeni.package('test')
          .processor(function testProcessor() {
            return {
              $process: function() { log.push(this.testValue); }
            };
          });
        dgeni.package('test1', ['test2'])
          .config(function(testProcessor) { testProcessor.testValue = 1; });
        dgeni.package('test2', ['test'])
          .config(function(testProcessor) { testProcessor.testValue = 2; });
        dgeni.generate()
          .then(function() {
            expect(log).toEqual([1]);
            done();
          });
      });

      it("should provide access to the injector", function(done) {
        var localInjector;
        dgeni.package('test')
          .config(function(injector) {
            localInjector = injector;
          });
        dgeni.generate().finally(function() {
          expect(localInjector.get('injector')).toBe(localInjector);
          done();
        });
      });
    });


    describe("services", function() {


      it("should add services to the injector", function(done) {
        var log = [];

        dgeni.package('test-package')
          .processor(function testProcessor(service1, service2) {
            return {
              $process: function(docs) {
                log.push(service1);
                log.push(service2);
              }
            };
          })
          .factory(function service1() { return 'service1 value'; })
          .factory(function service2(service1) { return service1 + ' service2 value'; });

        dgeni.generate().finally(function() {
          expect(log).toEqual(['service1 value', 'service1 value service2 value']);
          done();
        });
      });

    });


    describe("processors", function() {

      describe("dependencies", function() {

        it("should order the processors by dependency", function(done) {
          var log = [];
          dgeni.package('test1')
            .processor(function a() { return { $runAfter: ['c'], $process: function() { log.push('a'); } }; })
            .processor(function b() { return { $runAfter: ['c','e','a'], $process: function() { log.push('b'); } }; })
            .processor(function c() { return { $runBefore: ['e'], $process: function() { log.push('c'); } }; })
            .processor(function d() { return { $runAfter: ['a'], $process: function() { log.push('d'); } }; })
            .processor(function e() { return { $runAfter: [], $process: function() { log.push('e'); } }; });
          dgeni.generate()
            .then(function() {
              expect(log).toEqual(['c', 'e', 'a', 'b', 'd']);
              done();
            });
        });

        it("should ignore processors that have $enabled set to false", function(done) {
          var log = [];
          dgeni.package('test1')
            .processor(function a() { return { $process: function() { log.push('a'); } }; })
            .processor(function b() { return { $enabled: false, $process: function() { log.push('b'); } }; })
            .processor(function c() { return { $process: function() { log.push('c'); } }; });
          dgeni.generate()
            .then(function() {
              expect(log).toEqual(['a', 'c']);
              done();
            });
        });

        it("should allow config blocks to change $enabled on a processor", function() {
          var log = [];
          dgeni.package('test1')
            .processor(function a() { return { $process: function() { log.push('a'); } }; })
            .processor(function b() { return { $enabled: false, $process: function() { log.push('b'); } }; })
            .processor(function c() { return { $process: function() { log.push('c'); } }; })
            .config(function(a,b,c) {
              a.$enabled = false;
              b.$enabled = true;
            });
          dgeni.generate()
            .then(function() {
              expect(log).toEqual(['b', 'c']);
              done();
            });
        });

        it("should throw an error if the $runAfter dependencies are invalid", function() {
          dgeni.package('test')
            .processor(function badRunAfterProcessor() { return { $runAfter: 'tags-processed' }; });
          expect(function() {
            dgeni.generate();
          }).toThrow();
        });

        it("should throw an error if the $runBefore dependencies are invalid", function() {
          dgeni.package('test')
            .processor(function badRunBeforeProcessor() { return { $runBefore: 'tags-processed' }; });
          expect(function() {
            dgeni.generate();
          }).toThrow();
        });

        it("should allow config blocks to change the order of the processors", function(done) {
          log = [];
          dgeni.package('test')
            .processor(function a() { return { $runBefore: ['b'], $process: function(docs) { log.push('a' ); } }; })
            .processor(function b() { return { $runBefore: ['c'], $process: function(docs) { log.push('b' ); } }; })
            .processor(function c() { return { $process: function(docs) { log.push('c' ); } }; })
            .config(function(a, b, c) {
              b.$runBefore = [];
              c.$runBefore = ['b'];
            });
          dgeni.generate([]).then(function() {
              expect(log).toEqual(['a', 'c', 'b']);
              done();
            });
        });
      });

      describe("validation", function() {

        it("should fail if processor has an invalid property", function(done) {
          dgeni.package('test')
            .processor(function testProcessor() {
              return {
                $validate: { x: { presence: true } }
              };
            });

          dgeni.generate().catch(function(errors) {
            expect(errors).toEqual([{ processor : "testProcessor", package : "test", errors : { x : [ "X can't be blank" ] } }]);
            done();
          });
        });


        it("should not fail if all the processors properties are valid", function(done) {
          var log = [];
          dgeni.package('test')
            .processor(function testProcessor() {
              return {
                $validate: { x: { presence: true } },
                $process: function() { log.push(this.x); }
              };
            })
            .config(function(testProcessor) {
              testProcessor.x = 'not blank';
            });

          dgeni.generate().then(function() {
            expect(log).toEqual(['not blank']);
            done();
          });
        });

        it("should not fail if stopOnValidationError is false", function(done) {

          dgeni.package('test')
            .config(function(dgeni) {
              dgeni.stopOnValidationError = false;
            })
            .processor(function testProcessor() {
              return {
                $validate: { x: { presence: true } }
              };
            });

          var error;
          dgeni.generate()
            .catch(function(e) {
              error = e;
            })
            .finally(function() {
              expect(error).toBeUndefined();
              expect(mockLogger.error).toHaveBeenCalled();
              done();
            });
        });

      });

      describe("bad-processor", function() {
        var testPackage, doc, badProcessor;

        beforeEach(function() {
          testPackage = dgeni.package('test')
            .processor(function badProcessor() {
              return {
                $process: function() { throw new Error('processor failed'); }
              };
            });
          doc = {};
        });

        describe('stopOnProcessingError', function(done) {

          it("should fail if stopOnProcessingError is true and a processor throws an Error", function(done) {
            dgeni.generate()
              .catch(function(e) {
                expect(e).toBeDefined();
                done();
              });
          });

          it("should not fail but log the error if stopOnProcessingError is false a processor throws an Error", function(done) {

            var error;
            testPackage
              .config(function(dgeni) {
                dgeni.stopOnProcessingError = false;
              });

            dgeni.generate()
              .catch(function(e) {
                error = e;
              })
              .finally(function() {
                expect(error).toBeUndefined();
                expect(mockLogger.error).toHaveBeenCalled();
                done();
              });
          });

          it("should continue to process the subsequent processors after a bad-processor if stopOnProcessingError is false", function(done) {
            var called = false;

            testPackage
              .config(function(dgeni) {
                dgeni.stopOnProcessingError = false;
              })
              .processor(function checkProcessor() {
                return {
                  $runAfter: ['badProcessor'],
                  $process: function() {
                    called = true;
                  }
                };
              });

            dgeni.generate().finally(function() {
              expect(called).toEqual(true);
              done();
            });
          });
        });
      });
    });
  });
});