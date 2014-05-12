`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`
require('./utilities.coffee').infect global

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
         _(root.metadata).filter().filter('isResponsible').pluck('to').reduce recursivelyMask, set
      
      _.uniq recursivelyMask new Array, @root
   
   # Explores other `Mask`'s graphs, returning `true` if this `Mask` encapsulates any of the same nodes.
   conflictsWith: (others...)->
      others = others.reduce ((others, other)->
         others.concat other.flatten() ), new Array
      
      _(others).some (thing)=> _(this.flatten()).contains thing
   
   # Explores other `Mask`'s graphs, returning `true` if they include *all* of this `Mask`'s nodes.
   containedBy: (others...)->
      others = others.reduce ((others, other)->
         others.concat other.flatten() ), new Array
      
      _(this.flatten()).difference(others).isEmpty()

# This acts as a `Unit`'s store of access knowledge: `Executions` are matched to the `Mask`s they've
# successfully requested a form of access to.
#
# I'd *really* like to see a better data-structure; but my knowledge of such things is insufficient to
# apply a truly appropriate one. For now, just a simple mapping of `Mask`s to accessors (`Executions`).
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
      _(@content).remove (entry)->
         return false if accessor? and entry.accessor != accessor
         return true unless mask
         _(entry.masks).remove (m)-> m.conflictsWith mask
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

reactor.Combination = Combination = class Combination
   constructor: constructify (@subject, @message)->


# The default receiver for `Thing`s preforms a ‘lookup’ (described in `data.coffee`).
Paws.Thing::receiver = new Alien (rv, world)->
   [_, caller, subject, message] = rv.toArray()
   results = subject.find message
   Paws.notice "No results on #{subject.toString()} for #{message.toString()}." unless results[0]
   world.stage caller, results[0].valueish() if results[0]
.rename 'thing✕'

# `Execution`'s default-receiver preforms a “call”-patterned staging; that is, cloning the subject
# `Execution`, staging that clone, and leaving the caller unstaged.
Paws.Execution::receiver = new Alien (rv, world)->
   [_, caller, subject, message] = rv.toArray()
   world.stage subject.clone(), message
.rename 'execution✕'


# Given an `Execution`, this will preform the functions of the `reactor` necessary to advance that
# `Execution` one ‘step’, or combination. This requires a `response` (usually the ‘result’ of the
# previous combination; more specifically, whatever the thing that queued this `Execution` passed to
# it.)
#
# This will mutate the `position` counter of the `Execution` (hence the name of `advance()`), as
# well as managing the `stack` thereof.
#---
# XXX: At some point, I want to refactor this function (originally, a method of Execution, that I
#      decided was more in-kind with the rest of `reactor` instead of with anything implemented
#      within the rest of the data-types, and so moved here) to be A) simpler, and B) integrated
#      tighter with the rest of the reactor. For now, however, it's a direct port from `µpaws.js`.
# TODO: REPEAT. REFACTOR THIS SHIT.
# XXX: The original implementation .bind()ed aliens' bits to the Alien object (`this` at call-time.)
#      For the moment, I've nixed this, depending on the reactor loop to handle that with `apply`.
reactor.advance = (exec, result)-> advance.call exec, result

advance = (response)->
   return if @complete()
   
   if this instanceof Alien
      @pristine = no
      return @bits.shift()
   
   unless @pristine
      # ... we're continuing an existing execution
      
      unless @position?
         # ... that's previously reached the *end* of an expression, so we step ‘up’ the stack.
         {value, next} = @stack.pop()
         @position = next
         return new Combination value, response
      
      {contents, next} = @position
      unless @position.contents instanceof parser.Expression
         # The upcoming node being neither the end of an expression, nor the beginning of a new one,
         # then it must be a Thing. We combine that against the response passed-in.
         @position = next
         return new Combination response, contents
      else
         # Else, we're going to dive into the new expression, and then go through all of the below.
         @position = contents
         @stack.push value: response, next: next
      
   # We've exhausted the easy situations. Either we're looking at the beginning of a new expression,
   # in a non-pristine Execution, or we're at the beginning of a pristine Execution.
   @pristine = no
   
   # Even if we've already dug into a new expression above, there's still possibly *more*
   # expressions nested immediately (that is, as the first node in the previous one). We drill down
   # through all of those, if so, until we get to a ‘real’ node (that is, a node whose `contents`
   # isn't another Expression, but rather a `Thing`.)
   while @position.next?.contents instanceof parser.Expression
      {contents, next} = @position
      @position = next.contents
      # DOCME: wat is this:
      @stack.push value: @locals, next: next.next
   
   upcoming = @position
   unless upcoming.next?
      # DOCME: wat.
      {value, next} = @stack.pop()
      @position = next
      return new Combination value, upcoming.contents ? this # Special-cased self-reference, `()`
   
   # DOCME: wat.
   {contents, next} = @position
   @position = next.next
   return new Combination contents ? @locals, next.contents


# The Unitary design (i.e. distribution) isn't complete, at all. At the moment, a `Unit` is just a
# place to store the action-queue and access-table.
#
# Theoretically, this should be enough to, at least, run two Units *at once*, even if there's
# currently no design for the ways I want to allow them to interact.
# More on that later.
reactor.Unit = Unit = parameterizable class Unit
   constructor: constructify(return:@) ->
      @queue = new Array
      @table = new Table
   
   # `stage`ing is the core operation of a `Unit` as a whole, the only one that requires
   # simultaneous access to the `queue` and `table`.
   stage: (execution, result, requestedMask)->
      @queue.push new Staging execution, result, requestedMask
      @schedule() if @_?.incrementAwaiting != no
   
   
   # This method looks for the foremost request of the queue that either:
   # 
   #  1. doesn’t have an associated `requestedMask`,
   #  2. is already responsible for a mask equivalent to the one requested,
   #  3. or whose requested mask doesn’t conflict with any existing ones, excluding its own.
   # 
   # If no request is currently valid, it returns undefined.
   next: ->
      _(@queue).findWhere (staging, idx)=>
         @queue.splice(idx, 1)[0] if @table.allowsStagingOf staging
   
   # Generate the form of object passed to receivers.
   @receiver_parameters: (stagee, subject, message)->
      new Thing(stagee, subject, message).rename '<receiver params>'
   
   # The core reactor of this implementation, `#realize` will ‘process’ a single `Staging` from this
   # `Unit`'s queue. Returns `true` if a `Staging` was acquired and in some way processed, and
   # `false` if no processing was possible.
   realize: ->
      return no unless staging = @next()
      {stagee, result, requestedMask} = staging
      prior_position = stagee.position
      
      return yes if stagee.complete()
      return yes unless combo = reactor.advance stagee, result
      
      @current = stagee
      
      if process.env['TRACE_REACTOR']
         Paws.alert ">> #{stagee} ← #{result}"
         Paws.alert "   #{prior_position.with(context: yes, tag: no).toString()}" if prior_position?
         Paws.alert "   ┗> #{combo.subject} × #{combo.message}" if combo.subject?
      
      # If the staging has passed #next, then it's safe to grant it the ownership it's requesting
      @table.give stagee, requestedMask if requestedMask
      
      # If we're looking at an alien, then we received a bit-function from #advance
      if typeof combo == 'function'
         combo.apply stagee, [result, this]
      
      else
         @stage combo.subject.receiver.clone(),
            Unit.receiver_parameters stagee, combo.subject, combo.message
      
      @table.remove accessor: stagee if stagee.complete()
      
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
   schedule: ->
      ++@awaitingTicks
      return if @current?
      
      while @awaitingTicks
         if @realize() then --@awaitingTicks else return
   
   awaitingTicks: 0
   interval: 50         # in milliseconds; default of 1/20th of a second.
   interval = 0
   
   start: -> interval ||= setInterval @realize.bind(this), @interval
   stop:  -> if interval then clearInterval(interval) and interval = undefined
