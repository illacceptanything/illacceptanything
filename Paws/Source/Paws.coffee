`require = require('./cov_require.js')(require)`

require('./additional.coffee') module.exports =
   paws = new Object

uuid = require 'uuid'

paws.utilities       = require('./utilities.coffee').infect global
paws.Unit   = Unit   = require './Unit.coffee'
paws.Script = Script = require './Script.coffee'



# Core data-types
# ---------------
paws.Thing = Thing = parameterizable class Thing
   constructor: constructify(return:@) (elements...) ->
      @id = uuid.v4()
      @metadata = new Array
      @push elements... if elements.length
      
      @metadata.unshift undefined if @_?.noughtify != no
   
   # Creates a copy of the `Thing` it is called on. Alternatively, can be given an extant `Thing`
   # copy this `Thing` *to*, over-writing that `Thing`'s metadata. In the process, the
   # `Relation`s within this relation are themselves cloned, so that changes to the new clone's
   # responsibility don't affect the original.
   clone: (to) ->
      to ?= new Thing.with(noughtify: no)()
      to.metadata = @metadata.map (rel) -> rel?.clone()
      return to
   
   compare: (to) -> to == this
   
   at: (idx) -> @metadata[idx]?.to
   push: (elements...) ->
      @metadata = @metadata.concat Relation.from elements
   
   toArray: (cb) -> @metadata.map (rel) -> (cb ? identity) rel?.to
   
   # This implements the core algorithm of the default jux-receiver; this algorithm is very
   # crucial to Paws' object system:
   # 
   # Working through the metadata in reverse, select those items whose *first* (not the naughty;
   # subscript-1) item `compare()`s truthfully to the searched-for key. Return them in the order
   # found (thus, “in reverse”), such that the latter-most item in the metadata that was found to
   # match is returned as the first match. For libside purposes, only this (the very latter-most
   # matching item) is used.
   #
   # Of note, in this implementation, we additionally test *if the matching item is a pair*. For
   # most *intended* purposes, this should work fine; but it departs slightly from the spec.
   # We'll see if we keep it that way.
   find: (key) ->
      # TODO: Sanity-check `key`
      results = @metadata.filter (rel) ->
         rel?.to?.isPair?() and key.compare rel.to.at 1
      _.pluck(results.reverse(), 'to')
   
   # TODO: Figure out whether pairs should be responsible for their children
   @pair: (key, value) ->
      new Thing(Label(key), value)
   isPair:   -> @metadata[1] and @metadata[2]
   keyish:   -> @at 1
   valueish: -> @at 2
   
   responsible:   -> new Relation this, yes
   irresponsible: -> new Relation this, no

paws.Relation = Relation = parameterizable delegated('to', Thing) class Relation
   # Given a `Thing` (or `Array`s thereof), this will return a `Relation` to that thing.
   # 
   # @option responsible: Whether to create new relations as `responsible`
   @from: (it) ->
      if it instanceof Relation
         it.responsible @_?.responsible ? it.isResponsible
         return it
            
      if it instanceof Thing
         return new Relation(it, @_?.responsible ? false)
      if _.isArray(it)
         return it.map (el) => @from el
   
   constructor: constructify (@to, @isResponsible = false) ->
   
   clone: -> new Relation @to, @isResponsible
   
   responsible:   chain (val) -> @isResponsible = val ? true
   irresponsible: chain       -> @isResponsible = false


paws.Label = Label = class Label extends Thing
   constructor: constructify(return:@) (@alien) ->
      @alien = new String @alien
      @alien.native = this
   
   clone: (to) ->
      to ?= new Label
      super to
      to.alien = @alien
      to.alien.native
      return to
   compare: (to) ->
      to instanceof Label and
      to.alien.valueOf() == @alien.valueOf()


paws.Execution = Execution = class Execution extends Thing
   constructor: constructify (first) ->
      unless this instanceof Alien or this instanceof Native
         return (if typeof first == 'function' then Alien else Native).apply this, arguments
      
      @pristine = yes
      @locals = new Thing # TODO: `name` this “locals”
      @locals.push Thing.pair 'locals', @locals.irresponsible()
      @      .push Thing.pair 'locals', @locals.responsible()

paws.Alien = Alien = class Alien extends Execution
   constructor: constructify(return:@) (@bits...) -> @alien = yes

paws.Native = Native = class Native extends Execution
   constructor: constructify(return:@) (@position) -> @stack = new Array
