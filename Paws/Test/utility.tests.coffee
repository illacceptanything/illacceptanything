expect = require 'expect.js'

Paws = require "../Source/Paws.coffee"

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
      composed = utilities.modifier (foo)-> return 'yep' if foo == 'foo'
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
         Ctor = constructify(arguments: 'intact') (args)->
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
         Paws.info "Silencing output."; Paws.SILENT()
         Ctor = null
         class Klass
            constructor: Ctor = constructify ->
         Ctor()
         expect(-> new Klass).to.throwException()
      it 'can be called multiple times /reg', ->
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
         Ctor = constructify (rv)-> return rv
         obj = new Object
         expect(new Ctor(obj)).to.be obj
         expect(    Ctor(obj)).to.be obj
      it 'returns the new instance, otherwise', ->
         Ctor = constructify (rv)-> return 123
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
      
      it 'should provide the name of the original constructor', ->
         class Klass
            constructor: constructify ->
         instance = new Klass
        #expect(instance.constructor.__name__).to.be 'Klass'
         
         instance = Klass()
         expect(instance.constructor.__name__).to.be 'Klass'
           
   
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
      
      it 'should not leave cruft around on the object', (complete)->
         twat = new Twat.with({})()
         setTimeout => # *Intentionally* using setTimeout instead of nextTick
            expect(twat._).to.be undefined
            complete()
         , 0
   
   describe 'delegated()', ->
      it 'should create definitions for super methods', ->
         class Delegatee
            operate: (arg)-> return this: this, argument: arg
         
         Something = utilities.delegated('a_member', Delegatee) class Something
            constructor: (@a_member)->
         
         expect(Something::operate).to.be.ok()
      
      it 'should delegate calls to missing methods', ->
         class Delegatee
            operate: (arg)-> return this: this, argument: arg
         
         Something = utilities.delegated('a_member', Delegatee) class Something
            constructor: (@a_member)->
         
         expect(Something::operate).to.be.ok()
         
         foo = new Delegatee
         instance = new Something(foo)
         expect(-> instance.operate()).to.not.throwException()
         expect(instance.operate('bar').this).to.be foo
         expect(instance.operate('bar').argument).to.be 'bar'
      
      it 'should not shadow re-implemented methods', ->
         correct_shadowed = ->
         class Delegatee
            shadowed: ->
         
         Something = utilities.delegated('foo', Delegatee) class Something
            shadowed: correct_shadowed
            constructor: (@foo)->
         
         expect(Something::shadowed).to.be correct_shadowed
      
      it 'should not delegate non-function properties', ->
         class Delegatee
            somebody: 'Micah'
         correct_shadowed = ->
         
         Something = utilities.delegated('foo', Delegatee) class Something
            constructor: (@foo)->
         
         expect(Object.getOwnPropertyNames Something::).to.not.contain 'somebody'
      
      it 'should delegate to ancestors', ->
         class Ancestor
            operate: (arg)-> return this: this, argument: arg
         
         class Delegatee extends Ancestor
         
         Something = utilities.delegated('a_member', Delegatee) class Something
            constructor: (@a_member)->
         
         expect(Something::operate).to.be.ok()
         
         foo = new Delegatee
         instance = new Something(foo)
         expect(instance.operate('bar').this).to.be foo
         expect(instance.operate('bar').argument).to.be 'bar'
      
      it 'should handle built-ins well /reg', ->
         Something = utilities.delegated('stuff', Array) class Something
            constructor: (@stuff)->
         
         expect(Something::shift).to.be.ok()
         
         instance = new Something([1, 2, 3])
         expect(instance.shift()).to.be 1
         expect(instance.stuff).to.have.length 2
