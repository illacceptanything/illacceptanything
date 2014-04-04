`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`
assert = require 'assert'
expect = require 'expect.js'

Paws = require "../Source/Paws.coffee"
Paws.utilities.infect global, Paws

describe 'The Paws reactor:', ->
   reactor = Paws.reactor
   it 'should exist', ->
      expect(reactor).to.be.ok()
   
   describe 'Mask', ->
      Mask = reactor.Mask
      
      describe '#flatten', ->
         it 'should always return at least the root', ->
            a_mask = new Mask(a_thing = new Thing)
            expect(a_mask.flatten()).to.contain a_thing
         
         it 'should include anything owned by that root', ->
            [a_thing, another_thing] = [new Thing, new Thing]
            a_mask = new Mask Thing.construct {something: a_thing, something_else: another_thing}
            expect(a_mask.flatten()).to.contain a_thing
            expect(a_mask.flatten()).to.contain another_thing
         
         it 'should include anything owned, recursively, by that roots', ->
            [a_thing, another_thing] = [new Thing, new Thing]
            parent_thing = Thing.construct {something: a_thing, something_else: another_thing}
            a_mask = new Mask Thing.construct {child: parent_thing}
            expect(a_mask.flatten()).to.contain parent_thing
            expect(a_mask.flatten()).to.contain a_thing
            expect(a_mask.flatten()).to.contain another_thing
      
      # FIXME: This is insufficiently exercised.
      describe '#conflictsWith', ->
         it 'should indicate whether passed a mask currently contains some of the same items', ->
            a_mask = new Mask Thing.construct(things = {a: new Thing, b: new Thing})
            expect(a_mask.conflictsWith new Mask things.a).to.be true
            expect(a_mask.conflictsWith new Mask new Thing).to.be false
      
      describe '#containedBy', ->
         it 'should return true if the only item is contained by a passed Mask', ->
            a_mask       = new Mask Thing.construct(things = {a: new Thing, b: new Thing})
            another_mask = new Mask things.a
            expect(another_mask.containedBy a_mask).to.be true
         
         it 'should return true if all items are contained by a passed Mask', ->
            [a_thing, another_thing] = [new Thing, new Thing]
            parent_thing = Thing.construct {something: new Thing, something_else: new Thing}
            a_mask = new Mask Thing.construct {child: parent_thing}
            another_mask = new Mask parent_thing
            expect(another_mask.containedBy a_mask).to.be true
         
         it 'should return true if all items are contained by one or another of the passed Masks'
            # NYI: Is this even possible? If it contains the root node of a given mask, then it must
            #      contain all nodes. In fact, I should take advantage of that in my climbing algorithm ...
         
         it 'should return false if any item is *not* contained by one of the passed Masks', ->
            # XXX: Ditto above. It's possible that *only* roots can possibly be not-contained. Hm ...
            [a_thing, another_thing] = [new Thing, new Thing]
            parent_thing = Thing.construct {something: new Thing, something_else: new Thing}
            a_mask = new Mask Thing.construct {child: parent_thing}
            another_mask = new Mask parent_thing
            expect(a_mask.containedBy another_mask).to.be false
   
   describe 'a responsibility Table', ->
      Table = reactor.Table
      Mask = reactor.Mask
      
      it 'should exist', ->
         expect(Table).to.be.ok()
      
      it 'should store a Mask for a given Execution', ->
         table = new Table
         
         an_xec = new Execution
         a_mask = new Mask new Thing
         
         expect(-> table.give an_xec, a_mask).not.to.throwError()
         expect(table.get an_xec).to.contain a_mask
      
      it 'should store multiple Masks for a given Execution', ->
         table = new Table
         
         an_xec = new Execution
         a_mask = new Mask new Thing
         expect(-> table.give an_xec, a_mask).not.to.throwError()
         expect(table.get an_xec).to.contain a_mask
         
         another_mask = new Mask new Thing
         expect(-> table.give an_xec, another_mask).not.to.throwError()
         expect(table.get an_xec).to.contain another_mask
         expect(table.get an_xec).to.contain a_mask
      
      it 'should separately store Masks for multiple Executions', ->
         table = new Table
         
         an_xec = new Execution
         [mask_A, mask_B] = [new Mask(new Thing), new Mask(new Thing)]
         table.give an_xec, mask_A, mask_B
         expect(table.get an_xec).to.contain mask_A
         expect(table.get an_xec).to.contain mask_B
         
         another_xec = new Execution
         [mask_X, mask_Y] = [new Mask(new Thing), new Mask(new Thing)]
         table.give another_xec, mask_X, mask_Y
         expect(table.get another_xec).to.contain mask_X
         expect(table.get another_xec).to.contain mask_Y
         expect(table.get another_xec).to.not.contain mask_A
         expect(table.get another_xec).to.not.contain mask_B
      
      # FIXME: Not well-exercised.
      describe '#has', ->
         it 'should be able to tell if a given Mask already belongs to a given Execution', ->
            table = new Table
            an_xec = new Execution
            
            parent_thing = Thing.construct {something: new Thing, something_else: new Thing}
            a_mask = new Mask Thing.construct {child: parent_thing}
            
            table.give an_xec, a_mask
            expect(table.has an_xec, new Mask parent_thing).to.be true
      
      describe '#canHave', ->
         it 'should reject a Mask responsible for a Thing that belongs to another Execution', ->
            table = new Table
            an_xec = new Execution
            another_xec = new Execution
            
            parent_thing = Thing.construct {something: new Thing, something_else: new Thing}
            a_mask = new Mask Thing.construct {child: parent_thing}
            
            table.give another_xec, a_mask
            expect(table.canHave an_xec, new Mask parent_thing).to.be false
         
         it 'should not be affected by unrelated responsibility held by the Table', ->
            table = new Table
            an_xec = new Execution
            another_xec = new Execution
            
            table.give another_xec, new Mask new Thing
            expect(table.canHave an_xec, new Mask new Thing).to.be true
