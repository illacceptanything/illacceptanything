expect = require 'expect.js'

describe "Paws' utilities:", ->
   
   utilities = require '../Source/utilities'
   it 'should exist', ->
      expect(utilities).to.be.ok()
   
   
   run = utilities.runInNewContext
   describe '#runInNewContext', ->
      it 'should not error out', ->
         expect(-> run 'true')    .to.not.throwException()
         expect(-> run 'Function').to.not.throwException()
      
      it 'should return values', ->
         expect(run '42').to.be 42
      it 'should return functions', ->
         expect(run 'Function').to.be.a 'function'
      
      it.skip 'should expose the passed sandbox', ->
         sandbox = {foo: new Object}
         expect(run '(function(){ return foo })()', sandbox).to.be sandbox.foo
      
      it.skip 'should mirror updates to values in the passed sandbox', ->
         sandbox = new Object
         run '(function(){ foo = 456 })()', sandbox
         expect(sandbox.foo).to.be 456
      
      
      it 'should use a new JavaScript execution-context', ->
         # FIXME: The following test currently fails on Testling, for reasons unknown.
         expect(run 'Object').to.not.be Object
         expect(run 'new Object').to.not.be.an Object
         
         expect(run 'Function').to.not.be Function
         expect(run 'new Object').to.not.be.an Object
   
   if process.browser then describe '#runInNewContext (client)', ->
      it 'should not leave trash in the DOM', ->
         iframes = window.document.getElementsByTagName 'iframe'
         expect(iframes).to.be.empty()
   
   
   sub = utilities.subclass
   subclassTests = (canHaveAccessors) -> ->
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
         expect( (new Fan)('foo') ).to.be 'foobar'
      
      it 'should maintain the prototype-chain as expected', ->
         Fan = sub Function
         Fan.prototype.method = (foo) -> this.foo = foo
         
         fan = new Fan
         expect(-> fan.method 'bar').to.not.throwError()
         expect(fan.foo).to.be 'bar'
      
   describe '#subclass (via __proto__)', subclassTests true if utilities.hasPrototypeAccessors()
   describe '#subclass (via a foreign context)', subclassTests false
