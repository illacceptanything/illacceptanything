require('./utilities.coffee').infect global

{EventEmitter} = require 'events'

Paws = require './data.coffee'
infect global, Paws

module.exports =
   reactor = new Object

# A Mask is a lens through which one can use a `Thing` as a delinator of a particular sub-graph of the
# running program's object-graph.
reactor.Mask = Mask = class Mask
   constructor: constructify(return:@) (@root)->
   
   # Given the `Thing`-y root, flatten out the nodes (more `Thing`s) of the sub-graph ‘owned’ by that
   # root (at the time this function is called) into a simple unique-set. Acheived by climbing the graph.
   flatten: ()->
      recursivelyMask = (set, root)->
         set.push root
         _(root.metadata)
            .filter().filter('owns')
            .pluck('to')
            .reduce(recursivelyMask, set)
      
      _.uniq recursivelyMask new Array, @root
   
   # Explores other `Mask`'s graphs, returning `true` if this `Mask` encapsulates any of the same nodes.
   conflictsWith: (others...)->
      others = others.reduce ((others, other)->
         others.concat other.flatten() ), new Array
      
      _(others).some (thing)=> _(@flatten()).contains thing
   
   # Explores other `Mask`'s graphs, returning `true` if they include *all* of this `Mask`'s nodes.
   containedBy: (others...)->
      others = others.reduce ((others, other)->
         others.concat other.flatten() ), new Array
      
      _(@flatten()).difference(others).isEmpty()

# This acts as a `Unit`'s store of access knowledge: `Executions` are matched to the `Mask`s they've
# been given responsibility for
#
# I'd *really* like to see a better data-structure; but my knowledge of such things is insufficient
# to apply a truly appropriate one. For now, just a simple mapping of `Mask`s to accessors
# (`Executions`).
reactor.Table = Table = class Table
   constructor: ->
      @content = new Array
   
   give: (accessor, masks...)->
      entry = _(@content).find accessor: accessor
      if not entry?
         @content.push(entry = { accessor: accessor, masks: new Array })
      
      entry.masks.push masks...
      return entry
   
   get: (accessor)-> _(@content).find(accessor: accessor)?.masks ? new Array
   
   # FIXME: Test the remove-conflicting-masks functionality
   remove: ({accessor, mask})->
      return unless accessor? or mask?
      _.remove @content, (entry)->
         return false if accessor? and entry.accessor != accessor
         return true unless mask
         _.remove entry.masks, (m)-> m.conflictsWith mask
         entry.masks.length == 0
   
   # Returns `true` if a given `Mask` is fully contained by the set of responsibility that a given
   # `accessor` has already been given.
   has: (accessor, mask)->
      mask.containedBy @get(accessor)...
   
   # Returns `true` if a given `Mask` conflicts with any of the responsibility given to *other*
   # accessors.
   canHave: (accessor, mask)->
      not _(@content).reject(accessor: accessor).some (entry)->
         mask.conflictsWith entry.masks...
   
   allowsStagingOf: ({stagee, _, requestedMask})-> not requestedMask? or
      @has(stagee, requestedMask) or
      @canHave(stagee, requestedMask)


reactor.Staging = Staging = class Staging
   constructor: constructify (@stagee, @result, @requestedMask)->


# The default receiver for `Thing`s preforms a ‘lookup’ (described in `data.coffee`).
Paws.Thing::receiver = new Native (rv, world)->
   [caller, subject, message] = rv.toArray()
   results = subject.find message
   # FIXME: Welp, this is horrible error-handling. "Print a warning and freeze forevah."
   Paws.notice "~~ No results on #{Paws.inspect subject} for #{Paws.inspect message}." unless results[0]
   world.stage caller, results[0].valueish() if results[0]
.rename 'thing✕'

# `Execution`'s default-receiver preforms a “call”-patterned staging; that is, cloning the subject
# `Execution`, staging that clone, and leaving the caller unstaged.
Paws.Execution::receiver = new Native (rv, world)->
   [caller, subject, message] = rv.toArray()
   world.stage subject.clone(), message
.rename 'execution✕'


# The Unitary design (i.e. distribution) isn't complete, at all. At the moment, a `Unit` is just a
# place to store the action-queue and access-table.
#
# Theoretically, this should be enough to, at least, run two Units *at once*, even if there's
# currently no design for the ways I want to allow them to interact.
# More on that later.
reactor.Unit = Unit = parameterizable class Unit extends EventEmitter
   constructor: constructify(return:@) ->
      @queue = new Array
      @table = new Table
   
   # `stage`ing is the core operation of a `Unit` as a whole, the only one that requires
   # simultaneous access to the `queue` and `table`.
   stage: (execution, result, requestedMask)->
      @queue.push new Staging execution, result, requestedMask
      @schedule() if @_?.immediate != no
   
   
   # This method looks for the foremost staging of the queue that either:
   # 
   #  1. doesn’t have an associated `requestedMask`,
   #  2. is already responsible for a mask equivalent to the one requested,
   #  3. or whose requested mask doesn’t conflict with any existing ones, excluding its own.
   # 
   # If no request is currently valid, it returns undefined.
   #---
   # FIXME: Ugly.
   next: ->
      idx = _(@queue).findIndex (staging)=> @table.allowsStagingOf staging
      @queue.splice(idx, 1)[0] if idx != -1
   upcoming: ->
      results = _.filter @queue, (staging)=> @table.allowsStagingOf staging
      return if results.length then results else undefined
   
   #---
   # XXX: Exists soely for debugging purposes. Could just emit *inside* `realize`.
   flushed: ->
      if process.env['TRACE_REACTOR']
         Paws.verbose "~~ Queue flushed#{if @queue.length then ' @ '+@queue.length else ''}."
      @emit 'flushed', @queue.length
   
   # Generate the form of object passed to receivers.
   @receiver_parameters: (stagee, subject, message)->
      new Thing(stagee, subject, message).rename '<receiver params>'
   
   # The core reactor of this implementation, `#realize` will ‘process’ a single `Staging` from this
   # `Unit`'s queue. Returns `true` if a `Staging` was acquired and in some way processed, and
   # `false` if no processing was possible.
   #
   # Emits a 'flushed' event if there's no executions in the queue (at least, none that are valid
   # for staging.) Listeners will be passed the number of unrealizable executions still in the
   # queue, if any are present.
   realize: ->
      unless staging = @next()
         @awaitingTicks = 0
         return no
      {stagee, result, requestedMask} = staging
      
      if process.env['TRACE_REACTOR']
         exec_printout = if not stagee.position? then '' else
            ("\n" + stagee.position.with(context: yes, tag: no).toString())
            .replace /\n/g, "\n   │  "
         Paws.warning ">> #{stagee} ← #{result}" + exec_printout
      
      # Remove completed stagees from the queue, with no further action.
      if stagee.complete()
         Paws.warning "   ╰┄ complete!" if process.env['TRACE_REACTOR']
         @flushed() unless @upcoming()
         return yes
      
      combo = stagee.advance result
      @current = stagee
      
      # If the staging has passed #next, then it's safe to grant it the ownership it's requesting
      @table.give stagee, requestedMask if requestedMask
      
      # If we're looking at a native, then we received a bit-function from #advance
      if typeof combo == 'function'
         combo.apply stagee, [result, this]
      
      else
         subject = combo.subject ? stagee.locals
         message = combo.message ? stagee.locals
         
         if process.env['TRACE_REACTOR']
            Paws.warning "   ╰┄ combo: #{combo.subject} × #{combo.message}"
         
         @stage subject.receiver.clone(),
            Unit.receiver_parameters stagee, subject, message
      
      @table.remove accessor: stagee if stagee.complete()
      
      @flushed() unless @upcoming()
      delete @current
      return yes

   # Every time `schedule()` is called on a `Unit`, the implementation is informed that there's at
   # least one more combination that needs to be processed. As long as the implementation *knows*
   # there's combinations waiting to be processed, it will attempt to process them actively, on the
   # stack (that is, non-asynchronously.) This is often quite a bit faster than waiting for the next
   # iteration of the underlying event-loop.
   #
   # Immediately after incrementing the count of awaiting-combinations, this will start the reactor
   # attempting to process those combinations
   #---
   # FIXME: I'm pretty sure `@awaitingTicks` can be replaced with `@queue.length`...
   schedule: ->
      ++@awaitingTicks
      return if @current?
      
      while @awaitingTicks
         if @realize() then --@awaitingTicks else return
   
   awaitingTicks: 0
   interval: 50         # in milliseconds; default of 1/20th of a second.
   interval = 0
   
   start: ->
      interval ||= setInterval @schedule.bind(this), @interval
      @schedule()
   stop:  ->
      if interval
         clearInterval(interval)
         interval = undefined
