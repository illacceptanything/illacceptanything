`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`
assert = require 'assert'
expect = require 'expect.js'
sinon  = require 'sinon'

# TODO: Replace all the 'should' language with more direct 'will' language
#       (i.e. “it should return ...” becomes “it returns ...”
describe 'The Paws API:', ->
   Paws = require "../Source/Paws.coffee"
   it 'should be defined', ->
      expect(Paws).to.be.ok()
   
   Thing     = Paws.Thing
   Label     = Paws.Label
   Execution = Paws.Execution
   
   describe 'Thing', ->
      
      Relation = Paws.Relation
      describe 'Relation', ->
         describe '##from', ->
            it 'should return the passed value if it is already a Relation', ->
               rel = new Relation(new Thing)
               expect(Relation.from rel).to.be rel
            it 'should return a relation *to* the passed value if not', ->
               thing = new Thing
               expect(Relation.from thing).to.be.a Relation
               expect(Relation.from(thing).to).to.be thing
            
            it 'should ensure all elements of a passed array are Relations', ->
               thing1 = new Thing; thing2 = new Thing; thing3 = new Thing
               array = [thing1, new Relation(thing2), thing3]
               expect(Relation.from array).to.be.an 'array'
               expect(Relation.from(array).every (el) -> el instanceof Relation).to.be.ok()
            
            it 'should be able to change the responsibility of relations', ->
               thing1 = new Thing; thing2 = new Thing; thing3 = new Thing
               array = [thing1, new Relation(thing2, false), new Relation(thing3, true)]
               expect( # TODO: This could be cleaner.
                  Relation.with(responsible: yes).from(array).every (el) -> el.isResponsible
               ).to.be.ok()
               
         it 'should default to being irresponsible', ->
            expect((new Relation).isResponsible).to.be false
         
         describe '#clone', ->
            it 'should return a new Relation', ->
               rel = new Relation(new Thing, true)
               expect(rel.clone()).to.not.be rel
               expect(rel.clone().to).to.be rel.to
               expect(rel.clone().isResponsible).to.be rel.isResponsible
         
      describe '##pair', ->
      
      uuid_regex = /[a-f0-9]{8}-[a-f0-9]{4}-4[a-f0-9]{3}-[89aAbB][a-f0-9]{3}-[a-f0-9]{12}/
      it 'should have a UUID', ->
         expect((new Thing).id).to.match uuid_regex
      
      it 'should noughtify the metadata by default', ->
         thing = new Thing
         expect(thing.metadata).to.have.length 1
         expect(thing.metadata[0]).to.be undefined
         
         bare_thing = new Thing.with(noughtify: no)()
         expect(bare_thing.metadata).to.have.length 0
      
      it 'should store metadata relations', ->
         child1 = new Thing; child2 = new Thing
         thing = new Thing child1, child2
         expect(thing).to.have.property 'metadata'
         expect(thing.metadata).to.be.an 'array'
         expect(thing.metadata[1]).to.be.a Relation
         expect(thing.metadata[1].to).to.be child1
         expect(thing.metadata[2]).to.be.a Relation
         expect(thing.metadata[2].to).to.be child2
      
      describe '#clone', ->
         it 'should return a new Thing', ->
            thing = new Thing
            expect(thing.clone()).to.not.be thing
         it 'should have identical metadata', ->
            thing = new Thing new Thing, new Thing, new Thing
            clone = thing.clone()
            expect(clone.metadata).to.have.length 4
            thing.metadata.forEach (rel, i) -> if rel
               expect(clone.at i).to.be.ok()
               expect(rel).not.to.be clone.metadata[i]
               expect(rel.to).to.be clone.metadata[i].to
         it 'should apply new metadata to any passed Thing', ->
            thing1 = new Thing new Thing, new Thing, new Thing
            thing2 = new Thing new Thing
            old_metadata = thing2.metadata
            
            result = thing1.clone(thing2)
            expect(result).to.be thing2
            expect(thing2.metadata).to.not.be old_metadata
      
      # FIXME: Seperate JavaScript-side convenience API tests from algorithmic tests
      describe '#find', ->
         first = new Thing; second = new Thing; third = new Thing
         foo_bar_foo = new Thing Thing.pair('foo', first),
                                 Thing.pair('bar', second),
                                 Thing.pair('foo', third)
         
         it 'should return an array ...', ->
            expect(foo_bar_foo.find Label 'foo').to.be.an 'array'
         it '... of result-Things ...', ->
            expect(foo_bar_foo.find(Label 'foo').length).to.be.greaterThan 0
            foo_bar_foo.find(Label 'foo').forEach (result) ->
               expect(result).to.be.a Thing
         it '... that only contains matching pairs ...', ->
            expect(foo_bar_foo.find Label 'foo').to.have.length 2
         it '... in reverse order', ->
            expect(foo_bar_foo.find(Label 'foo')[0].valueish()).to.be third
            expect(foo_bar_foo.find(Label 'foo')[1].valueish()).to.be first
         
         it 'should handle non-pair Things gracefully', ->
            thing = new Thing Thing.pair('foo', first),
                              new Thing,
                              Thing.pair('bar', second),
                              Thing.pair('foo', third)
            expect(thing.find Label 'foo').to.have.length 2
         
         it 'should accept JavaScript primitives', ->
            expect(foo_bar_foo.find 'foo').to.have.length 2
   
   
   describe 'Label', ->
      it 'should contain a String', ->
         foo = new Label 'foo'
         expect(foo).to.be.a Thing
         expect(foo.alien).to.be.a String
         expect(foo.alien.valueOf()).to.be 'foo' # temporary hack. see: http://git.io/expect.js-57
      
      it 'should compare as equal, when containing the same String', ->
         foo1 = new Label 'foo'
         foo2 = new Label 'foo'
         expect(foo1.compare foo2).to.be true
   
   
   describe 'Execution', ->
      Alien  = Paws.Alien
      Native = Paws.Native
      
      it 'should construct as either a Native or an Alien or a native', ->
         expect(new Execution ->).to.be.an Alien
         expect(new Execution {}).to.be.a Native
         expect(new Execution)   .to.be.a Native
      
      it 'should begin life as pristine', ->
         expect((new Execution).pristine).to.be yes
      it 'should have locals', ->
         exe = new Execution
         expect(exe.find 'locals').to.not.be.empty()
         expect(exe.locals()).to.be.a Thing
         expect(exe.locals().metadata).to.have.length 2
         
         expect(exe         .at(1).valueish()).to.be exe.locals()
         expect(exe         .at(1).metadata[2].isResponsible).to.be true
         expect(exe.locals().at(1).valueish()   ).to.be exe.locals()
         expect(exe.locals().at(1).metadata[2].isResponsible).to.be false
      
      describe '(Alien / nukespace code)', ->
         it 'should take a series of procedure-bits', ->
            a = (->); b = (->); c = (->)

            expect(-> new Execution a, b, c).to.not.throwException()
            expect(   new Execution a, b, c).to.be.an Alien
            
            expect(  (new Execution a, b, c).bits).to.have.length 3
            expect(  (new Execution a, b, c).bits).to.eql [a, b, c]
         
         it 'should know whether it is complete', ->
            ex = new Execution ->
            expect(ex.complete()).to.be false
            
            ex.bits.length = 0
            expect(ex.complete()).to.be true
         
         it 'can be cloned', ->
            ex = new Execution ->
            expect(-> ex.clone()).to.not.throwException()
            expect(   ex.clone()).to.be.an Alien
            
         it 'has the same bits after cloning', ->
            funcs =
               one: ->
               two: ->
               three: ->
            ex = new Execution funcs.one, funcs.two, funcs.three
            
            clone = ex.clone()
            expect(clone.bits).to.not.be ex.bits
            expect(clone.bits).to.eql [funcs.one, funcs.two, funcs.three]
         
         it 'shares locals with clones', ->
            ex = new Execution ->
            clone = ex.clone()
            
            expect(clone.locals()).to.equal ex.locals()
         
         describe '##synchronous', ->
            synchronous = Alien.synchronous
            it 'accepts a function', ->
               expect(   synchronous).to.be.ok()
               expect(-> synchronous ->).to.not.throwException()
            
            it 'results in a new Alien', ->
               expect(synchronous ->).to.be.an Alien
            
            it 'adds bits corresponding to the arity of the function', ->
               expect( (synchronous (a, b)->)       .bits).to.have.length 3
               expect( (synchronous (a, b, c, d)->) .bits).to.have.length 5
            
            describe 'produces bits that ...', ->
               a = null
               beforeEach -> a =
                  caller: new Execution
                  thing:  new Label 'foo'
                  world: { stage: sinon.spy() }
               call = (exe, rv)->
                  exe.bits.shift().call exe, rv, a.world
               
               it 'are Functions', ->
                  exe = synchronous (a, b, c)->
                  expect(exe.bits[0]).to.be.a Function
                  expect(exe.bits[1]).to.be.a Function
                  expect(exe.bits[2]).to.be.a Function
                  expect(exe.bits[3]).to.be.a Function
               
               it 'expect a caller, RV, and world', ->
                  exe = synchronous (a, b, c)->
                  expect(exe.bits[0]).to.have.length 2 # `caller`-curry
                  expect(exe.bits[1]).to.have.length 3
                  expect(exe.bits[2]).to.have.length 3
               
               it 'can be successfully called', ->
                  exe = synchronous (a, b, c)->
                  expect(-> call exe, a.caller).to.not.throwException()
                  expect(-> call exe, a.thing).to.not.throwException()
               
               it 'are provided a `caller` by the first bit', ->
                  some_function = sinon.spy (a, b, c)->
                  exe = synchronous some_function
                  exe.bits = exe.bits.map (bit)-> sinon.spy bit
                  bits = exe.bits.slice()
                  
                  call exe, a.caller
                  call exe, new Label 123
                  call exe, new Label 456
                  call exe, new Label 789
                  
                  assert bits[1].calledWith a.caller
                  assert bits[2].calledWith a.caller
                  assert bits[3].calledWith a.caller
               
               it 're-stage the `caller` after each coproductive consumption', ->
                  stage = a.world.stage
                  exe = synchronous (a, b, c)->
                  
                  call exe, a.caller
                  expect(stage.callCount).to.be 1
                  assert stage.getCall(0).calledOn a.world
                  assert stage.getCall(0).calledWith a.caller, exe
                  
                  call exe, new Label 123
                  expect(stage.callCount).to.be 2
                  assert stage.getCall(1).calledOn a.world
                  assert stage.getCall(1).calledWith a.caller, exe
                  
                  call exe, new Label 456
                  expect(stage.callCount).to.be 3
                  assert stage.getCall(2).calledOn a.world
                  assert stage.getCall(2).calledWith a.caller, exe
                  
                  call exe, new Label 789
                  expect(stage.callCount).to.not.be 4
               
               it 're-stage the `caller` after all coproduction if a result is returned', ->
                  stage = a.world.stage
                  
                  result = new Label "A result!"
                  exe = synchronous (a)-> return result
                  
                  call exe, a.caller
                  expect(stage.callCount).to.be 1
                  assert stage.getCall(0).calledOn a.world
                  assert stage.getCall(0).calledWith a.caller, exe
                  
                  call exe, new Label 123
                  expect(stage.callCount).to.be 2
                  assert stage.getCall(1).calledOn a.world
                  assert stage.getCall(1).calledWith a.caller, result
               
               it 'call the passed function exactly once, when exhausted', ->
                  some_function = sinon.spy (a, b, c)->
                  exe = synchronous some_function
                  
                  call exe, a.caller
                  call exe, new Label 123
                  call exe, new Label 456
                  call exe, new Label 789
                  
                  assert some_function.calledOnce
                  
               it 'collect individually passed arguments into arguments to the passed function', ->
                  some_function = sinon.spy (a, b, c)->
                  exe = synchronous some_function
                  
                  things =
                     first:  new Label 123
                     second: new Label 456
                     third:  new Label 789
                  
                  call exe, a.caller
                  call exe, things.first
                  call exe, things.second
                  call exe, things.third
                  
                  assert some_function.calledWithExactly(
                     things.first, things.second, things.third, a.world)
               
               it 'inject context into the passed function', ->
                  some_function = sinon.spy (arg)->
                  exe = synchronous some_function
                  
                  call exe, a.caller
                  call exe, a.thing
                  
                  expect(some_function.firstCall.thisValue).to.have.property 'caller'
                  expect(some_function.firstCall.thisValue.caller).to.be.an Execution
                  expect(some_function.firstCall.thisValue.caller).to.be a.caller
                  
                  expect(some_function.firstCall.thisValue).to.have.property 'this'
                  expect(some_function.firstCall.thisValue.this).to.be.an Execution
                  expect(some_function.firstCall.thisValue.this).to.be exe
                  
                  expect(some_function.firstCall.thisValue).to.have.property 'world'
                 #expect(some_function.firstCall.thisValue.world).to.be.a World # FIXME
                  expect(some_function.firstCall.thisValue.world).to.be a.world
      
      describe '(Native / libspace code)', ->
         Expression = Paws.parser.Expression
         
         it 'should take a position', ->
            expr = new Expression
            
            expect(-> new Execution expr).to.not.throwException()
            expect(   new Execution expr).to.be.an Native
            
            expect(  (new Execution expr).position).to.be.ok()
         
         it 'should know whether it is complete', ->
            ex = new Execution (new Expression)
            expect(ex.complete()).to.be false
            
            ex.position = null
            ex.stack.push 42
            expect(ex.complete()).to.be false
            
            ex.stack.length = 0
            expect(ex.complete()).to.be true
         
         it 'can be cloned', ->
            ex = new Execution Expression()
            expect(-> ex.clone()).to.not.throwException()
            expect(   ex.clone()).to.be.an Native
            
         it 'preserves the position and stack when cloning', ->
            pos1 = new Expression
            pos2 = new Expression
            ex = new Execution pos1
            
            clone1 = ex.clone()
            expect(clone1.position).to.be pos1
            expect(clone1.stack).to.not.be ex.stack
            expect(clone1.stack).to.eql ex.stack
            
            ex.position = pos2
            ex.stack.push new Label 'intermediate value'
            clone2 = ex.clone()
            expect(clone2.position).to.be pos2
            expect(clone2.stack).to.have.length 1
            expect(clone2.stack).to.not.be ex.stack
            expect(clone2.stack).to.eql ex.stack
         
         it 'shares locals with clones', ->
            ex = new Execution Expression()
            clone = ex.clone()
            
            expect(clone.locals()).to.equal ex.locals()
