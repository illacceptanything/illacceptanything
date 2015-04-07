assert = require 'assert'
sinon  = require 'sinon'
expect = require('sinon-expect').enhance require('expect.js'), sinon, 'was'

Paws = require "../Source/Paws.coffee"
Paws.utilities.infect global, Paws

describe 'The Paws reactor:', ->
   reactor = Paws.reactor
   parse   = Paws.parse
   advance = reactor.advance
   
   Table   = reactor.Table
   Mask    = reactor.Mask
   Staging = reactor.Staging
   
   Unit    = reactor.Unit
   
   it 'should exist', ->
      expect(reactor).to.be.ok()
   
   describe 'Mask', ->
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
      it 'should exist', ->
         expect(Table).to.be.ok()
      
      it 'should always return an array', ->
         table = new Table
         
         an_xec = new Execution
         
         expect(-> table.get an_xec).not.to.throwError()
         expect(table.get an_xec).to.be.an 'array'
      
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
      
      # FIXME: Not well-executed, either.
      it 'removes masks as requested', ->
         table = new Table
         
         an_xec = new Execution
         [mask_A, mask_B] = [new Mask(new Thing), new Mask(new Thing)]
         table.give an_xec, mask_A, mask_B
         another_xec = new Execution
         [mask_X, mask_Y] = [new Mask(new Thing), new Mask(new Thing)]
         table.give another_xec, mask_X, mask_Y
         
         expect(-> table.remove accessor: an_xec).not.to.throwError()
         expect(table.get an_xec).to.not.contain mask_A
         expect(table.get an_xec).to.not.contain mask_B
         expect(table.get another_xec).to.contain mask_X
         expect(table.get another_xec).to.contain mask_Y
         
      
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
      
      describe '#allowsStagingOf', ->
         it 'passes stagings with no requestedMask', ->
            table = new Table
            
            staging_without_mask = new Staging new Execution, new Thing
            expect(table.allowsStagingOf staging_without_mask).to.be yes
         
         it 'passes stagings already responsible for their requested Mask', ->
            table = new Table
            an_xec = new Execution; a_mask = new Mask new Thing
            table.give an_xec, a_mask
            
            responsible_staging = new Staging an_xec, new Thing, a_mask
            expect(table.allowsStagingOf responsible_staging).to.be yes
            
         it 'passes stagings already responsible for the same Thing', ->
            table = new Table
            an_xec = new Execution; a_thing = new Thing
            table.give an_xec, new Mask a_thing
            
            responsible_staging = new Staging an_xec, new Thing, new Mask a_thing
            expect(table.allowsStagingOf responsible_staging).to.be yes
            
         it 'passes stagings responsible for equivalent Masks', ->
            table = new Table
            an_xec = new Execution; a_thing = new Thing
            table.give an_xec, new Mask a_thing
            
            responsible_staging = new Staging an_xec, new Thing, new Mask a_thing
            expect(table.allowsStagingOf responsible_staging).to.be yes
         
         it 'passes stagings that have no conflicts, when there is other ownership', ->
            table = new Table
            [a_thing, another_thing] = [new Thing, new Thing]
            [an_xec, another_xec] = [new Execution, new Execution]
            table.give another_xec, new Mask another_thing
            
            available_staging = new Staging an_xec, new Thing, new Mask a_thing
            expect(table.allowsStagingOf available_staging).to.be yes
         
         it 'fails stagings responsible for Masks equivalent to existing ownership', ->
            table = new Table
            [a_thing, another_thing] = [new Thing, new Thing]
            parent_thing = Thing.construct {something: a_thing, something_else: another_thing}
            [an_xec, another_xec] = [new Execution, new Execution]
            table.give another_xec, new Mask Thing.construct {child: parent_thing}
            
            conflicting_staging = new Staging an_xec, new Thing, new Mask a_thing
            expect(table.allowsStagingOf conflicting_staging).to.be no
   
   
   describe "Thing's default receiver", ->
      here = undefined; caller = undefined; receiver = undefined
      beforeEach ->
         here     = new Unit; here.stage = sinon.spy()
         caller   = new Execution
         receiver = Thing::receiver.clone()
      
      it 'stages the caller if there is a result', ->
         a_thing = Thing.construct foo: another_thing = new Thing
         params = Unit.receiver_parameters caller, a_thing, new Label 'foo'
         
         bit = receiver.advance params
         bit.apply receiver, [params, here]
         
         expect(here.stage).was.calledOnce()
         
      it 'does not stage the caller if there is no result', ->
         a_thing = Thing.construct foo: another_thing = new Thing
         params = Unit.receiver_parameters caller, a_thing, new Label 'bar'
         
         bit = receiver.advance params
         bit.apply receiver, [params, here]
         
         expect(here.stage).was.notCalled()
         
      it 'finds a matching pair-ish Thing in the subject', ->
         a_thing = Thing.construct foo: another_thing = new Thing
         params = Unit.receiver_parameters caller, a_thing, new Label 'foo'
         
         bit = receiver.advance params
         bit.apply receiver, [params, here]
         
         result = here.stage.getCall(0).args[1]
         expect(result).to.be another_thing
   
   describe "Execution's default receiver", ->
      here = undefined; caller = undefined; receiver = undefined
      beforeEach ->
         here     = new Unit; here.stage = sinon.spy()
         caller   = new Execution
         receiver = Execution::receiver.clone()
      
      it 'clones the subject', ->
         an_exec = new Execution; something = new Thing
         params = Unit.receiver_parameters caller, an_exec, something
         
         sinon.spy an_exec, 'clone'
         
         bit = receiver.advance params
         bit.apply receiver, [params, here]
         
         expect(an_exec.clone).was.called()
         
      it "stages the subject's clone", ->
         an_exec = new Execution; something = new Thing
         params = Unit.receiver_parameters caller, an_exec, something
         
         bit = receiver.advance params
         bit.apply receiver, [params, here]
         
         expect(here.stage).was.calledWith sinon.match.any, something
         
      it 'does not re-stage the caller', ->
         an_exec = new Execution; something = new Thing
         params = Unit.receiver_parameters caller, an_exec, something
         
         bit = receiver.advance params
         bit.apply receiver, [params, here]
         
         expect(here.stage).was.neverCalledWith caller, sinon.match.any
   
   
   describe 'a Unit', ->
      here = undefined
      beforeEach ->
         here = new Unit; here.realize = sinon.spy()
      
      describe '#stage', ->
         it 'adds the passed staging-data to the queue', ->
            an_exec = new Execution; something = new Thing
            
            here.stage an_exec, something
            expect(here.queue).to.have.length 1
            expect(here.queue[0].stagee).to.be an_exec
            expect(here.queue[0].result).to.be something
            
         it 'will pass execution-flow onwards to the realization system', ->
            an_exec = new Execution; something = new Thing
            
            here.stage an_exec, something
            expect(here.realize).was.calledOnce()
            
         it "can be told not to increment realization-count", ->
            an_exec = new Execution; something = new Thing
            
            here.with(immediate: no).stage an_exec, something
            expect(here.realize).was.notCalled()
      
      describe '#next', ->
         here = undefined
         beforeEach ->
            here = new Unit
         
         # FIXME: These tests are too tightly-coupled to #stage. /=
         it 'returns undefined if no staging is available', ->
            expect(here.next()).to.be undefined
            
            a_thing = Thing.construct foo: another_thing = new Thing
            here.table.give new Execution, new Mask another_thing
            here.with(immediate: no).stage new Execution, null, new Mask a_thing
            expect(here.next()).to.be undefined
            
         it 'returns an available staging', ->
            an_exec = new Execution; a_thing = new Thing; another_thing = new Thing
            here.table.give new Execution, new Mask another_thing
            here.with(immediate: no).stage an_exec, null, new Mask a_thing
            
            staging = here.next()
            expect(staging).to.be.ok()
            expect(staging.stagee).to.be an_exec
            
         it 'removes the staging from the queue', ->
            here.with(immediate: no).stage new Execution, new Thing
            
            expect(here.queue).to.have.length 1
            staging = here.next()
            expect(here.queue).to.be.empty()
   
   
   describe 'Realization', ->
      here = null
      beforeEach ->
         here = new Unit
      
      it 'fails a tick if there are no queued stagings', ->
         expect(here.realize()).to.not.be.ok()
      
      # This may be redundant, but I like that it's a little less dependant on other parts of the
      # system. Correspondantly, though, it's *very* tightly coupled to the current implementation
      # of `realize()`.
      it 'fails a tick if no staging is eligible', ->
         sinon.stub(here, 'next').returns undefined
         expect(here.realize()).to.not.be.ok()
      
      it 'fails a tick if available stagings request conflicting responsibility', ->
         mutex = new Thing
         owner = new Native ->
         here.table.give owner, new Mask mutex
         
         requestor = new Native ->
         here.with(immediate: no).stage requestor, undefined, new Mask mutex
         expect(here.realize()).to.not.be.ok()
      
      it 'succeeds a tick if a complete stagee is removed from the queue', ->
         stagee = new Native
         here.with(immediate: no).stage stagee
         expect(here.realize()).to.be.ok()
      
      it 'succeeds a tick if advance fails?'
         # FIXME: WAT. I have no idea how advance() could possibly fail.
      
      it "doesn't flush when the queue is populated", ->
         here.with(immediate: no).stage new Native ->
         here.with(immediate: no).stage new Native ->
         
         listener = sinon.spy()
         here.once 'flushed', listener
         
         expect(here.realize()).to.be.ok()
         expect(listener).was.notCalled()
      
      it 'emits flush when the queue is emptied', ->
         here.with(immediate: no).stage new Native ->
         
         listener = sinon.spy()
         here.once 'flushed', listener
         
         expect(here.realize()).to.be.ok()
      
      it 'emits flush (with deferred-count) when no more stagings are realizable', ->
         mutex = new Thing
         owner = new Execution
         here.table.give owner, new Mask mutex
         
         requestor = new Native ->
         
         here.with(immediate: no).stage new Native ->
         here.with(immediate: no).stage requestor, undefined, new Mask mutex
         
         listener = sinon.spy()
         here.once 'flushed', listener
         
         expect(here.realize()).to.be.ok()
         expect(listener).was.calledWith 1
      
      it 'gives a realizable stagee any requested ownership', ->
         mutex = new Thing
         requestor = new Native ->
         here.with(immediate: no).stage requestor, undefined, new Mask mutex
         expect(here.realize()).to.be.ok()
         
         expect(here.table.has requestor, new Mask mutex)
         
      it "calls the next bit for realizable Natives", ->
         body = sinon.spy()
         stagee = new Native body
         here.with(immediate: no).stage stagee
         expect(here.realize()).to.be.ok()
         
         expect(body).was.calledOnce()
      
      it "stages a *clone* of the current combination-subject's receiver", ->
         receiver = new Native ->
         clone = receiver.clone()
         sinon.stub(receiver, 'clone').returns clone
         
         foo = (new Thing).rename 'foo'
         foo.receiver = receiver
         
         stagee = new Execution parse "foo bar"
         combo = stagee.advance()
         
         here.with(immediate: no).stage stagee, foo
         
         expect(receiver.complete()).to.be no
         expect(here.realize()).to.be.ok()
         expect(receiver.clone).was.calledOnce()
         
         expect(here.realize()).to.be.ok()
         expect(receiver.complete()).to.be no
         expect(clone.complete()).to.be yes
      
      it 'invalidates all ownership for the stagee, if it has been completed', ->
         thing = new Thing
         owner = new Native ->
         here.table.give owner, new Mask thing
         
         expect(here.table.has owner, new Mask thing).to.be yes
         
         here.with(immediate: no).stage owner
         expect(here.realize()).to.be.ok()
         
         expect(here.table.has owner, new Mask thing).to.be no
         
   
      describe 'scheduling', ->
         it 'causes realization', ->
            sinon.stub(here, 'realize').returns yes
            
            here.schedule()
            expect(here.realize).was.calledOnce()
         
         it 'causes an extra, *deferred*, realization; if called during a realization tick', ->
            here.with(immediate: no).stage new Native ->
               expect(here.queue).to.have.length 1
               here.schedule()
               expect(here.queue).to.have.length 1
            here.with(immediate: no).stage new Native ->
            
            expect(here.queue).to.have.length 2
            here.schedule()
            expect(here.queue).to.have.length 0
         
         afterEach -> here.stop()
         
         it 'immediately realizes when started', ->
            body = sinon.spy()
            here.with(immediate: no).stage new Native body
            
            expect(body).was.notCalled()
            here.start()
            expect(body).was.calledOnce()
         
         it 'causes further realization after the start', (done)->
            @timeout 150
            here.interval = 10
            here.start()
            
            body = sinon.spy()
            here.with(immediate: no).stage new Native body
            
            setTimeout ->
               expect(body).was.calledOnce()
               done()
            , 17
         
         it 'can be stopped', (done)->
            @timeout 150
            here.start()
            
            here.with(immediate: no).stage new Native ->
               expect().to.fail() # Should never be realized.
            
            process.nextTick ->
               here.stop()
            
            setTimeout done, 17
