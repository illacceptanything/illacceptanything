`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`
(Paws = require './Paws.coffee').utilities.infect global
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
   
   get: (accessor)-> _(@content).find(accessor: accessor)?.masks
   
   remove: (accessor)->
      delete @content[ _(@content).findIndex accessor: accessor ]
   
   # Returns `true` if a given `Mask` is fully contained by the set of responsibility that a given
   # `accessor` has already been given.
   has: (accessor, mask)->
      mask.containedBy @get(accessor)...
   
   # Returns `true` if a given `Mask` conflicts with any of the responsibility given to *other*
   # accessors.
   canHave: (accessor, mask)->
      not _(@content).reject(accessor: accessor).some (entry)->
         mask.conflictsWith entry.masks...

# The Unitary design (i.e. distribution) isn't complete, at all. At the moment, a `Unit` is just a
# place to store the action-queue and access-table.
#
# Theoretically, this should be enough to, at least, run two Units *at once*, even if there's
# currently no design for the ways I want to allow them to interact.
# More on that later.
reactor.Unit = Unit = class Unit
   constructor: constructify(return:@) ->
      @queue = new Array
      @table = new Table

reactor.Staging = Staging = class Staging
   # NYI

reactor.Combination = Combination = class Combination
   constructor: constructify (@subject, @message)->

# Given an `Execution`, this will preform the functions of the `reactor` necessary to advance that
# `Execution` one ‘step’, or combination. This requires a `response` (usually the ‘result’ of the
# previous combination; more specifically, whatever the thing that queued this `Execution` passed to
# it.)
#
# This will mutate the `position` counter of the `Execution` (hence the name of `advance()`), as
# well as managing the `stack` thereof.
#
# @this {Execution} The `Execution` to advance. (This function must be `apply()`ied.)
#---
# XXX: At some point, I want to refactor this function (originally, a method of Execution, that I
#      decided was more in-kind with the rest of `reactor` instead of with anything implemented
#      within the rest of the data-types, and so moved here) to be A) simpler, and B) integrated
#      tighter with the rest of the reactor. For now, however, it's a direct port from `µpaws.js`.
# TODO: REPEAT. REFACTOR THIS SHIT.
advance = (response)->
   return if @complete()
   
   if this instanceof Alien
      @pristine = no
      return _.bind @bits.shift(), this
      # FIXME: Test whether _.bind works identically-enough to ES5's Function#bind() for this
   
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

# FIXME: I dislike exposing this, but I'm bad at TDD, and don't know how else to access it in my
#        tests. I need to move some of these sorts of things to a `testables` object, conditionally
#        exported.
reactor._advance = advance

reactor.schedule = 
   reactor.awaitingTicks++
   
   

reactor.awaitingTicks = 0
