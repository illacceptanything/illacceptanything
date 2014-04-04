`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`
(Paws = require './Paws.coffee').utilities.infect global

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

# At some point, I want to refactor this function (originally, a method of Execution, that I decided
# was more in-kind with the rest of `reactor` instead of with anything implemented within the rest
# of the data-types, and so moved here) to be A) simpler, and B) integrated tighter with the rest of
# the reactor. For now, however, it's a direct port from `µpaws.js`.
advance = (exe)->
   return if @complete()
   
   if this instanceof Alien
      @pristine = no
      return _.bind @bits.shift(), this
   
   # NYI

reactor.schedule = 
   reactor.awaitingTicks++
   
   

reactor.awaitingTicks = 0
