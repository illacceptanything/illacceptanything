`require = require('../Source/cov_require.js')(require)`
paws = require "../Source/paws.coffee"
expect = require 'expect.js'

describe "Paws' utilities:", ->
   utilities = require "../Source/utilities.coffee"
   it 'should exist', ->
      expect(utilities).to.be.ok()
   
   describe 'chain()', ->
      composed = utilities.chain -> 'whee'
      it 'should always return the `this` value', ->
         object = new Object
         expect(composed.call object).to.be object
   
   describe 'modifier()', ->
      composed = utilities.modifier (foo) -> return 'yep' if foo == 'foo'
      it 'should return the return-value of the body ...', ->
         expect(composed 'foo').to.be 'yep'
      it '... unless the body returns nothing', ->
         object = new Object
         expect(composed object).to.be object
   
   
   describe 'constructify()', ->
      it 'basically works', ->
         expect(constructify).to.be.ok()
         expect(-> constructify ->).to.not.throwException()
         expect(constructify ->).to.be.a 'function'
         Ctor = constructify ->
         expect(-> new Ctor).to.not.throwException()
         class Klass
            constructor: constructify ->
         expect(-> new Klass).to.not.throwException()
      it 'can take options', ->
         expect(-> constructify(foo: 'bar') ->).to.not.throwException()
      
      it 'returns a *new* function, not the constructor-body passed to it', ->
         body = ->
         Ctor = constructify body
         expect(constructify).to.not.be body
         class Klass
            constructor: constructify body
         expect(Klass).to.not.be body
      it 'can pass the `arguments` object intact', ->
         Ctor = constructify(arguments: 'intact') (args) ->
            @caller = args.callee.caller
         it = null; func = null
         expect(-> (func = -> it = new Ctor)() ).to.not.throwException()
         expect(it).to.have.property 'caller'
         expect(it.caller).to.be func
      it "causes constructors it's called on to always return instances", ->
         Ctor = constructify ->
         expect(new Ctor)  .to.be.a Ctor
         expect(    Ctor()).to.be.a Ctor
         expect(new Ctor().constructor).to.be Ctor
         expect(    Ctor().constructor).to.be Ctor
         class Klass
            constructor: constructify ->
         expect(new Klass)  .to.be.a Klass
         expect(    Klass()).to.be.a Klass
      
      it 'uses a really hacky system that requires you not to call the wrapper before CoffeeScript does', ->
         paws.info "Silencing output."; paws.SILENT()
         Ctor = null
         class Klass
            constructor: Ctor = constructify ->
         Ctor()
         expect(-> new Klass).to.throwException()
      it 'can be called multiple times /re', ->
         Ctor1 = constructify ->
         expect(-> new Ctor1).to.not.throwException()
         expect(-> new Ctor1).to.not.throwException()
         Ctor2 = constructify ->
         expect(-> Ctor2()).to.not.throwException()
         expect(-> Ctor2()).to.not.throwException()
         class Klass1
            constructor: constructify ->
         expect(-> new Klass1).to.not.throwException()
         expect(-> new Klass1).to.not.throwException()
         class Klass2
            constructor: constructify ->
         expect(-> Klass2()).to.not.throwException()
         expect(-> Klass2()).to.not.throwException()
      
      it 'executes the function-body passed to it, on new instances', ->
         Ctor = constructify -> @called = yes
         expect(new Ctor().called).to.be.ok()
      
      it "returns the return-value of the body, if it isn't nullish", ->
         Ctor = constructify (rv) -> return rv
         obj = new Object
         expect(new Ctor(obj)).to.be obj
         expect(    Ctor(obj)).to.be obj
      it 'returns the new instance, otherwise', ->
         Ctor = constructify (rv) -> return 123
         expect(new Ctor)  .not.to.be 123
         expect(    Ctor()).not.to.be 123
         expect(new Ctor)  .to.be.a Ctor
         expect(    Ctor()).to.be.a Ctor
      it 'can be configured to *always* return the instance', ->
         Ctor = constructify(return: this) -> return new Array
         expect(new Ctor()).not.to.be.an 'array'
         expect(    Ctor()).not.to.be.an 'array'
      
      it 'should call any ancestor that exists', ->
         Ancestor = constructify -> @ancestor_called = true
         class Parent extends Ancestor
            constructor: constructify -> @parent_called = true
         class Child extends Parent
            constructor: constructify -> @child_called = true
         
         expect(new Parent)  .to.be.an Ancestor
         expect(    Parent()).to.be.an Ancestor
         expect(new Child)  .to.be.an Ancestor
         expect(    Child()).to.be.an Ancestor
         expect(new Child)  .to.be.a Parent
         expect(    Child()).to.be.a Parent
         
         expect(new Parent)  .to.have.property 'ancestor_called'
         expect(    Parent()).to.have.property 'ancestor_called'
         expect(new Child)  .to.have.property 'ancestor_called'
         expect(    Child()).to.have.property 'ancestor_called'
         expect(new Child)  .to.have.property 'parent_called'
         expect(    Child()).to.have.property 'parent_called'
   
   
   describe 'parameterizable()', ->
      utilities.parameterizable class Twat
         constructor: -> return this
      
      it 'should create a parameterizable constructor', ->
         constructor = new Twat.with(foo: 'bar')
         expect(constructor).to.be.a 'function'
         expect(constructor()).to.be.a Twat
         expect(constructor()._.foo).to.be 'bar'
      
      it 'should provide parameterizable methods', ->
         twat = new Twat
         expect(twat.with(foo: 'bar')).to.be twat
         expect(twat._.foo).to.be 'bar'
      
      it 'should not leave cruft around on the object', (complete) ->
         twat = new Twat.with({})()
         setTimeout => # *Intentionally* using setTimeout instead of nextTick
            expect(twat._).to.be undefined
            complete()
         , 0
   
   describe 'delegated()', ->
      class Delegatee
         shadowed: ->
         operate: (arg) -> return [this, arg]
      
      correct_shadowed = ->
      utilities.delegated('foo', Delegatee) class Something
         shadowed: correct_shadowed
         constructor: (@foo) ->
      
      it 'should delegate calls to missing methods, if possible', ->
         something = new Something(new Delegatee)
         expect(Something::operate).to.be.ok()
         expect(-> something.operate()).to.not.throwException()
         expect(something.operate 123).to.eql [something.foo, 123]
      
      it 'should not shadow re-implemented methods', ->
         expect(Something::shadowed).to.be correct_shadowed
   
   
   describe.skip 'runInNewContext()', ->
      run = utilities.runInNewContext
      it 'should not error out', ->
         expect(-> run 'true')    .to.not.throwException()
         expect(-> run 'Function').to.not.throwException()
      
      it 'should return values', ->
         expect(run '42').to.be 42
      it 'should return functions', ->
         expect(run 'Function').to.be.a 'function'
      
      it 'should expose the passed sandbox', ->
         sandbox = {foo: new Object}
         expect(run '(function(){ return foo })()', sandbox).to.be sandbox.foo
      
      it 'should mirror updates to values in the passed sandbox', ->
         sandbox = new Object
         run '(function(){ foo = 456 })()', sandbox
         expect(sandbox.foo).to.be 456
      
      
      it 'should use a new JavaScript execution-context', ->
         # FIXME: The following test currently fails on Testling, for reasons unknown.
         expect(run 'Object').to.not.be Object
         expect(run 'new Object').to.not.be.an Object
         
         expect(run 'Function').to.not.be Function
         expect(run 'new Object').to.not.be.an Object
      
      describe '(regressions)', ->
         it 'should expose expected globals to eval-bodies /re #4', ->
            $Function = run 'Function'
            $func = new $Function "return Object"
            expect(-> $func()).to.not.throwException()
            expect($func()).to.be.a 'function'
   
   if process.browser then describe.skip '#runInNewContext (client)', ->
      it 'should not leave trash in the DOM', ->
         iframes = window.document.getElementsByTagName 'iframe'
         expect(iframes).to.be.empty()
   
   
   subclassTests = (canHaveAccessors) -> ->
      sub = utilities.subclass
      beforeEach -> utilities.hasPrototypeAccessors(canHaveAccessors)
      
      it 'should return functions', ->
         expect(sub Function).to.be.a 'function'
         expect(sub Function).to.not.be Function
      
      it 'should return operable constructors', ->
         Fan = sub Function
         expect(new Fan).to.be.a Fan
      
      if (canHaveAccessors)
         it 'should instantiate descendants into the local context\'s inheritance-tree', ->
            Fan = sub Function
            expect(new Fan).to.be.an Object
            expect(new Fan).to.be.a  Function
      
      it 'should support a function-body for the constructor', ->
         Fan = sub Function, (stuff) -> this.stuff = stuff; this
         expect(new Fan('foo').stuff).to.be 'foo'
      
      it 'should support a function-body for the descendant', ->
         Fan = sub Function,
            ->
            (arg) -> arg + 'bar'
         
         fan = new Fan
         expect( fan('foo') ).to.be 'foobar'
      
      it 'should maintain the prototype-chain as expected', ->
         Fan = sub Function
         Fan.prototype.method = (foo) -> this.foo = foo
         
         fan = new Fan
         expect(-> fan.method 'bar').to.not.throwError()
         expect(fan.foo).to.be 'bar'
      
   describe.skip 'subclass() (via __proto__)', subclassTests true if utilities.hasPrototypeAccessors()
   describe.skip 'subclass() (via a foreign context)', subclassTests false
