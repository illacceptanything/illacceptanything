var getInjectablesFactory = require('./getInjectables');

describe("getInjectables", function() {

  var mockInjector;
  var getInjectables;

  beforeEach(function() {
    mockInjector = jasmine.createSpyObj('injector', ['invoke']);
    mockInjector.invoke.and.callFake(function(fn) { return fn(); });
    getInjectables = getInjectablesFactory(mockInjector);
  });

  it("should call invoke on the injector for each factory", function() {

    function a() { return {}; }
    function b() { return {}; }
    function c() { return {}; }

    getInjectables([a, b, c]);
    expect(mockInjector.invoke.calls.count()).toEqual(3);
  });

  it("should get the name from the instance, then the factory", function() {
    function a() { return {}; }
    function b() { return function b2() {}; }
    function c() { return { name: 'c2' }; }

    var instances = getInjectables([a, b, c]);
    expect(instances[0].name).toEqual('a');
    expect(instances[1].name).toEqual('b2');
    expect(instances[2].name).toEqual('c2');
  });
});