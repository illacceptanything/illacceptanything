`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`
(Paws = require './Paws.coffee').utilities.infect global

module.exports =
reactor = new Object

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

# The Unitary design (i.e. distribution) isn't complete, at all. At the moment, a `Unit` is just a
# place to store the action-queue and access-table.
#
# Theoretically, this should be enough to, at least, run two Units *at once*, even if there's
# currently no design for the ways I want to allow them to interact.
# More on that later.
reactor.Unit = Unit = class Unit
   constructor: constructify(return:@) ->
      @queue = new Array
      @table = new AccessTable

# This acts as a `Unit`'s store of access knowledge: `Executions` are matched to the `Mask`s they've
# successfully requested a form of access to.
#
# I'd *really* like to see a better data-structure; but my knowledge of such things is insufficient
# to apply a truly appropriate one. For now, just a simple mapping of roots (via `Mask`s) to
# accessors (`Executions`).
class OwnershipTable
   constructor: ->

reactor.schedule = 
   reactor.awaitingTicks++
   
   

reactor.awaitingTicks = 0
