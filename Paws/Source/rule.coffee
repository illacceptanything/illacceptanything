`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`
require('./utilities.coffee').infect global

Paws = require './Paws.coffee'
infect global, Paws

# FIXME: Refactor this entire thing to use isaacs' `node-tap`

module.exports = Rule = class Rule extends Thing
   #---
   # NOTE: Expects an @environment similar to Execution.synchronous's `this`. Must contain .caller
   #       and .unit.
   constructor: constructify(return:@) (@environment, @title, @body, @collection = Collection.current())->
      @title = new Label @title unless @title instanceof Label
      @body.locals = @environment.caller.locals.clone()
      @collection.push this
   
   maintain_locals: (@locals)->
      @body.locals.inject @locals
   
   dispatch: ->
      Paws.notice '-- Dispatching:', Paws.inspect this
      @dispatched = true
      @environment.unit.once 'flushed', @eventually_listener if @eventually_listener?
      @environment.unit.stage @body
   
   pass: -> @status = true;  @complete()
   fail: -> @status = false; @complete()
   
   complete: ->
      Paws.info "-- Completed (#{@status}):", Paws.inspect this
      @environment.unit.removeListener 'flushed', @eventually_listener if @eventually_listener?
      @emit 'complete', @status
   
   # FIXME: repeated calls?
   eventually: (block)->
      Paws.info "-- Registering 'eventually' for ", Paws.inspect this
      block.locals.inject @locals if @locals?
      @eventually_listener = =>
         Paws.info "-- Firing 'eventually' for ", Paws.inspect this
         @environment.unit.stage block, undefined
      @environment.unit.once 'flushed', @eventually_listener if @dispatched
   
Rule.Collection = Collection = class Collection
   
   _current = undefined
   @current: -> _current ?= new Collection
   
   constructor: ->
      @rules = new Array
      @activate() unless _current?
   
   activate: -> _current = this
   
   push: (rules...)->
      _.map rules, (rule)=>
         rule.once('complete', => @print rule) if @reporting
         @rules.push rule
         rule.dispatch() if @dispatching
   
   dispatch: ->
      @dispatching = true
      _.map @rules, (rule)=> rule.dispatch()
   
   report: ->
      @reporting = true
      process.stdout.write "TAP version 13\n"
      _.map @rules, (rule)=>
         if rule.status? then @print rule
         else rule.once 'complete', => @print rule
   
   # This will close the suite, outputting no more TAP and dispatching no further tests. It will
   # also print the TAP 'plan' line, telling the harness how many tests were run.
   #---
   # FIXME: Tests *already dispatched* might output after this is called.
   # XXX: Vaguely convinced this is un-asynchronous. o_O
   complete: ->
      @dispatching = false
      @reporting = false
      process.stdout.write "1..#{@rules.length}\n"
   
   # Prints a line for a completed rule.
   print: (rule)->
      number = @rules.indexOf(rule) + 1
      status = switch rule.status
         when true      then 'ok'
         when false     then 'not ok'
         when 'pending' then 'not ok'
         else           rule.status
      directive = " # TODO" if rule.status == 'pending'
      
      process.stdout.write "#{status} #{number} #{rule.title.alien}#{directive||''}\n"
