`                                                                                                                 /*|*/ require = require('../Library/cov_require.js')(require)`
require('./utilities.coffee').infect global

uuid = require 'uuid'
util = require 'util'


module.exports =
   Paws = new Object

{debugging} = require('./additional.coffee')
debugging.inject Paws

# Core data-types
# ---------------
Paws.Thing = Thing = parameterizable class Thing
   constructor: constructify(return:@) (elements...)->
      @id = uuid.v4()
      @metadata = new Array
      @push elements... if elements.length
      
      @metadata.unshift undefined if @_?.noughtify != no
   
   rename: (name)-> @name = name ; return this
   
   # Construct a generic ‘key/value’ style `Thing` from a JavaScript `Object`-representation thereof.
   # These representations will have JavaScript strings as the keys (which will be converted into the
   # `Label` of a pair), and a Paws `Object`-type as the values.
   # 
   # For instance, given `{foo: thing_A, bar: thing_B}` will be constructed into the following:
   #    
   #    (, (, ‘foo’, thing_B), (, ‘bar’, thing_B))
   # 
   # The ‘pair-ish’ values are always owned by the generated structure; as are, by default, the objects
   # passed in. The latter is overridable with `.with(responsible: no)`.
   # 
   # @option responsible: Whether to mark the structure as `responsible` for the objects passed in.
   #---
   # TODO: Support functions, so this can replace µPaws' applyGlobals.
   @construct: (representation)->
      members = for key, value of representation
         value = Alien.synchronous value if _.isFunction value
         value = @construct value unless value instanceof Thing
         value.rename key if @_?.names
         relation = Relation(value, @_?.responsible ? yes)
         Thing.pair( key, relation ).responsible()
      
      return Thing members...
   
   # XXX: Defined later, in `reactor.coffee`. These definitions have to be deferred, because
   #      `Execution` isn't defined yet.
   receiver: undefined
   
   at: (idx)->       @metadata[idx]?.to
   set: (idx, to)->  @metadata[idx] = Relation.from to
   
   push: (elements...)->
      @metadata = @metadata.concat Relation.from elements
   pop: ->
      @metadata.pop()
   shift: ->
      noughty = @metadata.shift()
      result = @metadata.shift()
      @metadata.unshift noughty
      result
   unshift: (other)->
      # TODO: include-noughtie option
      noughty = @metadata.shift()
      @metadata.unshift other
      @metadata.unshift noughty
   
   compare: (to)-> to == this
   
   # Creates a copy of the `Thing` it is called on. Alternatively, can be given an extant `Thing`
   # copy this `Thing` *to*, over-writing that `Thing`'s metadata. In the process, the
   # `Relation`s within this relation are themselves cloned, so that changes to the new clone's
   # responsibility don't affect the original.
   clone: (to)->
      to ?= new Thing.with(noughtify: no)()
      to.metadata = @metadata.map (rel)-> rel?.clone()
      
      to.name = @name unless to.name?
      
      return to
   
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
   #---
   # TODO: `raw` option, to return the `Relation`s, instead of the wrapped `Thing`s
   find: (key)->
      key = new Label(key) unless key instanceof Thing
      results = @metadata.filter (rel)->
         rel?.to?.isPair?() and key.compare rel.to.at 1
      _.pluck(results.reverse(), 'to')
   
   toArray: (cb)-> @metadata.map (rel)-> (cb ? identity) rel?.to
   
   # TODO: Figure out whether pairs should be responsible for their children
   @pair: (key, value)->
      new Thing(Label(key), value)
   isPair:   -> @metadata[1] and @metadata[2]
   keyish:   -> @at 1
   valueish: -> @at 2
   
   responsible:   -> new Relation this, yes
   irresponsible: -> new Relation this, no

Paws.Relation = Relation = parameterizable delegated('to', Thing) class Relation
   # Given a `Thing` (or `Array`s thereof), this will return a `Relation` to that thing.
   # 
   # @option responsible: Whether to create new relations as `responsible`
   @from: (it)->
      if it instanceof Relation
         it.responsible @_?.responsible ? it.isResponsible
         return it
            
      if it instanceof Thing
         return new Relation(it, @_?.responsible ? false)
      if _.isArray(it)
         return it.map (el) => @from el
   
   constructor: constructify (@to, @isResponsible = false)->
   
   clone: -> new Relation @to, @isResponsible
   
   responsible:   chain (val)-> @isResponsible = val ? true
   irresponsible: chain      -> @isResponsible = false


Paws.Label = Label = class Label extends Thing
   constructor: constructify(return:@) (@alien)->
   
   clone: (to)->
      super (to ?= new Label)
      to.alien = @alien
      to.alien.native
      return to
   compare: (to)->
      to instanceof Label and
      to.alien == @alien
   
   # FIXME: I need to double-check the Unicode properties of this. I'd really like to explode by
   #        codepoint, and I'm not sure how JS handles `split()` and Unicode.
   explode: ->
      it = new Thing
      it.push.apply it, _.map @alien.split(''), (char)-> new Label char
      it


Paws.Execution = Execution = class Execution extends Thing
   constructor: constructify (first)->
      unless this instanceof Alien or this instanceof Native
         return (if typeof first == 'function' then Alien else Native).apply this, arguments
      
      @pristine = yes
      @locals = new Thing().rename 'locals'
      @locals.push Thing.pair 'locals', @locals.irresponsible()
      this   .push Thing.pair 'locals', @locals.responsible()
   
   # XXX: Defined later, in `reactor.coffee`. These definitions have to be deferred, because
   #      `Execution` isn't defined yet.
   receiver: undefined
   
   # This method of the `Execution` types will copy all data relevant to advancement of the
   # execution to a `Execution` instance. This includes the pristine-state, any `Alien`'s `bits`, or
   # a `Native`'s `stack` and `position`. A clone made thus can be advanced just as the original
   # would have been, without affecting the original's advancement-state.
   # 
   # Of note: along with all the other data copied from the old instance, the new clone will inherit
   # the original `locals`. This is intentional.
   # 
   #---
   # NOTE: This will never be called directly, as the Execution constructor ensures that actual
   #       instances of raw Execution are impossible, and both Alien and Native wrap this.
   # FIXME: ‘Cloning’ locals ... *isn't*, here. I need to figure out what I want to do with this.
   # TODO: nuke-API equivalent of lib-API's `branch()()`
   clone: (to)->
      super to
      to.pristine    = @pristine
      to.locals      = @locals
      to.resumptions = @resumptions if @resumptions?

Paws.Alien = Alien = class Alien extends Execution
   
   # An `Alien` is an `Execution` that's implemented with JavaScript code, instead of as a series of
   # Paws combinations. These are the primitive building-blocks with which Paws programs are built.
   # 
   # Most `Alien`s are exposed to Paws code through 'bags' of `Alien`s stored under useful names on
   # the `locals` of the first (root-level) `Execution` in a Paws program; notably, the primitives
   # described by the Paws specification are exposed in such a bag, named "infrastructure."
   # 
   # `Alien`s in this implementation consist of a series of 'bits', each of which is a JavaScript
   # `Function`. Each 'bit' of the `Alien` implements a (clearly synchronous, as they are written in
   # JavaScript) set of operations to be preformed upon the next resumption of the `Execution`
   # represented by this `Alien`. (It can be easier to conceptualize an `Alien` as an explicit
   # coroutine, with the `return` statement of one bit and the arguments of the following bit acting
   # somewhat like a traditional `yield` statement.)
   # 
   # The first `Function`-bit will be invoked upon resumption of the `Alien`, and then discarded
   # (thus causing the following `Function` to receive the resumption thereafter.) Upon resumption,
   # the `Function` will be invoked with the following arguments and environment:
   # 
   #  1. `result`, whatever resumption-value was queued to cause this resumption
   #  2. `unit` (optional), the `Unit` in which this resumption is relevant (thus providing access
   #     to the current `Unit`'s staging-`queue` and responsibility-`table`.)
   #  
   # In addition, the `this` at invocation will be the `Alien` itself, giving the bit-body
   # convenient access to a place to store incrementally-constructed results. (Note that the actual
   # `Alien` object referred to during the invocation of subsequent bits may *not* be the same, as
   # the `Alien` may have been branched!)
   constructor: constructify(return:@) (@bits...)->
      @resumptions = @bits.length
   
   complete: -> !this.bits.length
   
   clone: (to)->
      super (to ?= new Alien)
      _.map Object.getOwnPropertyNames(this), (key)=> to[key] = this[key]
      to.bits = @bits.slice 0
      return to

   # This alternative constructor will automatically generate a series of ‘bits’ that will curry the
   # appropriate number of arguments into a single, final function.
   # 
   # Instead of having to write individual function-bits for your Alien that collect the appropriate
   # set of resumption-values into a series of “arguments” that you need for your task, you can use
   # this convenience constructor for the common situation that you're treating an Execution as
   # equivalent to a synchronous JavaScript function.
   # 
   # ----
   # 
   # This takes a single function, and checks the number of arguments it requires before generating
   # the corresponding bits to acquire those arguments.
   # 
   # Then, once it's been resumed the appropriate number of times (plus one extra initial resumption
   # with a `caller` as the resumption-value, as is standard coproductive practice in Paws), the
   # synchronous JavaScript passed in as the argument here will be invoked.
   # 
   # That invocation will provide the arguments recorded in the function's implementation, as well
   # as a context-object containing the following information as `this`:
   # 
   # caller
   #  : The first resumption-value provided to the generated `Execution`. Usually, itself, an
   #    `Execution`, in the coproductive pattern.
   # this
   #  : The original `this`. That is, the generated `Execution` that's currently being run.
   # unit
   #  : The current `Unit` at the time of execution, as provided by the reactor.
   # 
   # After your function executes, if it provides a non-null JavaScript return value, then the
   # `caller` provided as the first resumption-value Paws-side will be resumed one final time with
   # that as the resumption-value. (Hence the name of this method: it provides a ‘synchronous’
   # result after all arguments have been acquired.)
   # 
   # @param { function(... [Thing]
   #                , this:{caller: Execution, this, unit: Unit}): ?Thing }
   #    func   The synchronous function we'll generate an Execution to match
   #---
   # FIXME: Replace the holdover ES5 methods in this with IE6-compat LoDash functions
   @synchronous: (func) ->
      body = ->
         arity = func.length
         @resumptions = arity + 1
         
         # First, we construct the *middle* bits of the coproductive pattern (that is, the ones that
         # handle all but the *last* actual argument the passed function requires.) These are pretty
         # generic: they simply partially-apply their RV to the *last* bit (which will be defined
         # below.) Thus, they participate in currying their argument into the final invocation of
         # the synchronous function.
         @bits = new Array(arity).join().split(',').map ->
            return (caller, rv, here)->
               # FIXME: Pretty this up with prototype extensions. (#last, anybody?)
               @bits[@bits.length - 1] = _.partial @bits[@bits.length - 1], rv
               here.stage caller, this
         
         # Next, we construct the *first* bit, which is assumed to be responsible for receiving the
         # `caller` (as is usually the case in the coproductive pattern.) It takes its
         # resumption-value, and curries it into *every* following bit. (Notice that both the
         # middle-bits, above, and the concluding bit, below, save a spot for a `caller` argument.)
         @bits[0] = (caller, here)->
            @bits = @bits.map (bit)=> _.partial bit, caller
            here.stage caller, this
         
         # Now, the complex part. The *final* bit has quite a few arguments curried into it:
         # 
         #  - First, we *immediately* (at generate-time) contribute the locals we'll need within the
         #    body: the `Paws` API, and the `func` we were passed
         #  - Second, the `caller` curried in by the first bit
         #  - Third, any *actual arguments* curried in by intermediate bits
         # 
         # In addition to these, it's got one final argument (the actual resumption-value with which
         # this final bit is invoked, **after** all the other bits have been exhausted), and the
         # Unit passed in by the reactor.
         #
         # These values are curred into a function we construct within the body-string below, that
         # proceeds to provide the *actual* arguments to the synchronous `func`, as well as
         # constructing a context-object to act as the `this` described above.
         #---
         # FIXME: Remove the `Paws` pass, if it's unnecessary
         @bits[arity] = Function.apply(null, ['Paws', 'func', 'caller'].concat(
            Array(arity + 1).join('_').split(''), 'here', """
               var rv = func.apply({ caller: caller, this: this
                                   , unit: arguments[arguments.length - 1] }
                                 , [].slice.call(arguments, 3) )
               if (typeof rv !== 'undefined' && rv !== null) {
                  here.stage(caller, rv) }
            """))
         @bits[arity] = _.partial @bits[arity], Paws, func
         
         return this
      body.apply new Execution(->)

Paws.Native = Native = class Native extends Execution
   constructor: constructify(return:@) (@position)-> @stack = new Array
   
   complete:-> not this.position? and !this.stack.length
   
   clone: (to)->
      super (to ?= new Native)
      to.position = @position
      to.stack = @stack.slice 0
      return to


# Debugging output
# ----------------
T = debugging.tput

# Convenience to call whatever string-making methods are available on the passed object.
Paws.inspect = (object)->
   object?.inspect?() or
   object instanceof Thing && Thing::inspect.apply(object) or
   util.inspect object


Thing.inspectID = (it)-> it.id.slice(-8)

Thing::toString = ->
   output = Thing.inspectID(this) + (if @name? then ': '+T.bold @name else '')
   if @_?.tag == no then output else '['+(@constructor.__name__ or @constructor.name)+' '+output+']'

Thing::inspect = ->
   @toString()

Label::toString = ->
   output = '“'+@alien+'”' + (if @name? then ': '+T.bold @name else '')
   if @_?.tag == no then output else '['+(@constructor.__name__ or @constructor.name)+' '+output+']'

Execution::toString = ->
   output = Thing.inspectID(this) +
      (if @name? then ': '+T.bold @name else '') +
      (if @resumptions? then new Array(@resumptions - @bits.length).join('()') else '')
   if @_?.tag == no then output else '['+(@constructor.__name__ or @constructor.name)+' '+output+']'
