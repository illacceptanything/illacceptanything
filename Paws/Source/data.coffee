require('./utilities.coffee').infect global

uuid = require 'node-uuid'
util = require 'util'
{EventEmitter} = require 'events'


module.exports =
   Paws = new Object

Paws.debugging = require('./additional.coffee').debugging
Paws.debugging.inject Paws


# Core data-types
# ---------------

#---
# XXX: I'm not sure of the consequences of making all Things into EventEmitters ...
Paws.Thing = Thing = parameterizable class Thing extends EventEmitter
   constructor: constructify(return:@) (elements...)->
      @id = uuid.v4()
      @metadata = new Array
      @push elements... if elements.length
      
      @metadata.unshift undefined if @_?.noughtify != no
   
   rename: (name)-> @name = name ; return this
   
   # Construct a generic ‘key/value’ style `Thing` from a JavaScript `Object`-representation
   # thereof. These representations will have JavaScript strings as the keys (which will be
   # converted into the `Label` of a pair), and a Paws `Object`-type as the values.
   # 
   # For instance, given `{foo: thing_A, bar: thing_B}` will be constructed into the following:
   #    
   #    [, [, ‘foo’, thing_B], [, ‘bar’, thing_B]]
   # 
   # The ‘pair-ish’ values are always owned by the generated structure; as are, by default, the
   # objects passed in. The latter is overridable with `.with(own: no)`.
   # 
   # @option own: Whether to construct the structure's `Relation`s as `own`ing the objects passed in
   #---
   # TODO: Support functions, so this can replace µPaws' applyGlobals.
   @construct: (representation)->
      members = for key, value of representation
         value = Native.synchronous value if _.isFunction value
         value = @construct value unless value instanceof Thing
         value.rename key if @_?.names
         relation = Relation(value, @_?.own ? yes)
         Thing.pair( key, relation ).owned()
      
      return Thing members...
   
   # XXX: Defined later, in `reactor.coffee`. These definitions have to be deferred, because
   #      `Execution` isn't defined yet.
   receiver: undefined
   
   at: (idx)->       @metadata[idx]?.to
   set: (idx, to)->  @metadata[idx] = Relation.from to
   
   inject: (things...)->
      @push ( _.flatten _.map things, (thing)-> thing.toArray() )...
   
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
      # TODO: include-noughtie optional
      noughty = @metadata.shift()
      @metadata.unshift other
      @metadata.unshift noughty
   
   compare: (to)-> to == this
   
   # Creates a copy of the `Thing` it is called on. Alternatively, can be given an extant `Thing`
   # copy this `Thing` *to*, over-writing that `Thing`'s metadata. In the process, the
   # `Relation`s within this relation are themselves cloned, so that changes to the new clone's
   # ownership don't affect the original.
   clone: (to)->
      to ?= new Thing.with(noughtify: no)()
      to.metadata = @metadata.map (rel)-> rel?.clone()
      
      to.name = @name unless to.name?
      
      return to
   
   # This implements the core algorithm of the default jux-receiver; this algorithm is very
   # crucial to Paws' object system:
   # 
   # Working through the metadata in reverse, select those items whose *first* (not the noughty; but
   # subscript-one) item `compare()`s truthfully to the searched-for key. Return them in the order
   # found (thus, “in reverse”), such that the latter-most item in the metadata that was found to
   # match is returned as the first match. For libside purposes, only this (the very latter-most
   # matching item) is used.
   #
   # Of note, in this implementation, we additionally test *if the matching item is a pair*. For
   # most *intended* purposes, this should work fine; but it departs slightly from the spec.
   # We'll see if we keep it that way.
   #---
   # TODO: `pair` option, can be disabled to return the 'valueish' things, instead of the pairs
   # TODO: `raw` option, to return the `Relation`s, instead of the wrapped `Thing`s
   find: (key)->
      key = new Label(key) unless key instanceof Thing
      results = @metadata.filter (rel)->
         rel?.to?.isPair?() and key.compare rel.to.at 1
      _.pluck(results.reverse(), 'to')
   
   # TODO: Option to include the noughty
   toArray: (cb)-> @metadata.slice(1).map (rel)-> (cb ? identity) rel?.to
   
   @pair: (key, value)->
      new Thing(Label(key), value)
   isPair:   -> @metadata[1] and @metadata[2]
   keyish:   -> @at 1
   valueish: -> @at 2
   
   owned:    -> new Relation this, yes
   disowned: -> new Relation this, no

Paws.Relation = Relation = parameterizable delegated('to', Thing) class Relation
   # Given a `Thing` (or `Array`s thereof), this will return a `Relation` to that thing.
   # 
   # @option own: Whether to create new `Relation`s as `owns: yes`
   @from: (it)->
      if it instanceof Relation
         it.owned @_?.own ? it.owns
         return it
            
      if it instanceof Thing
         return new Relation(it, @_?.own ? no)
      if _.isArray(it)
         return it.map (el) => @from el
   
   constructor: constructify (@to, @owns = false)->
      @to.clone this if @to instanceof Relation
   
   clone: -> new Relation @to, @owns
   
   owned:    chain (val)-> @owns = val ? yes
   disowned: chain      -> @owns = no


Paws.Label = Label = class Label extends Thing
   constructor: constructify(return:@) (@alien)->
   
   clone: (to)->
      super (to ?= new Label)
      to.alien = @alien
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


# A `Combination` represents a single operation in the Paws semantic. An instance of this class
# contains the information necessary to process a pending combo (as returned by
# `Execution#next`).
Paws.Combination = Combination = class Combination
   constructor: constructify (@subject, @message)->

# A `Position` records the last element of a given expression from which a `Combination` was
# generated. It also contains the information necessary to find the *next* element of that
# expression's sequence (i.e. the indices of both the element within the expression, and the
# expression within the sequence.)
#
# This class is considered immutable.
Paws.Position = Position = class Position
   constructor: constructify (@_sequence, @expression_index = 0, @index = 0)->
      # FIXME: argument validation.
      #unless _.isArray(@_sequence) and _.isArray(@_sequence[0])
   
   sequence:   -> @_sequence
   expression: -> @_sequence?.at @expression_index
   valueOf:    -> @_sequence?.at(@expression_index)?.at @index
   
   clone: ->
      new Position @_sequence, @expression_index, @index
   
   # Returns a new `Position`, representing the next element of the parent sequence needing
   # combination. (If the current element is the last word of the last expression in the sequence,
   # returns `undefined`.)
   next: ->
      if @expression().at(@index + 1)?
         return new Position @_sequence, @expression_index, @index + 1
      if @sequence().at(@expression_index + 1)?
         return new Position @_sequence, @expression_index + 1

# `Execution`s store the context necessary to ‘hold’ a Paws script, partially-executed, until the
# next time it is advanced to produce a `Combination` for the reactor.
#
# Each time an `Execution` is advanced, that ‘current location’ at which it will be held is moved
# forward; either along the chain of words in the current expression, or diving into (or later back
# out of) sub-expressions within the current expression. That ‘current location’ is encoded into a
# stack of `Position`s stored in `instructions`; each time a new sub-expression is entered, a new
# `Position` for that expression is shifted onto the `instructions` stack. (Correspondingly, when a
# sequence of expressions is completed, that element is discarded from the stack.) *NB: This means
# the `instructions` stack lists *completed* nodes; ones for which a `Combination` has already been
# generated.)*
#
# In addition, the `results` of previous combinations, for each layer of the `instructions` stack,
# are stored in a second stack. These are used as the left-hand side of combinations (as well as the
# right-hand side when dropping out of a sub-expression.)
#
# Finally, an `Execution` contains the objective evaluation context of the instructions it contains;
# this is in the form of a `locals` object, against which the initial word in any expression will be
# combined.
Paws.Execution = Execution = class Execution extends Thing
   constructor: constructify (@begin)->
      if typeof @begin == 'function' then return Native.apply this, arguments
      if @begin instanceof Paws.parse.Expression then @begin = new Paws.parse.Sequence(@begin)
      if @begin instanceof Paws.parse.Sequence   then @begin = new Position(@begin)
      
      @instructions = [ @begin ]
      @results      = [ null   ]
      
      @pristine = yes
      @locals = new Thing().rename 'locals'
      @locals.push Thing.pair 'locals', @locals.disowned()
      this   .push Thing.pair 'locals', @locals.owned()
      
      return this
   
   # XXX: Defined later, in `reactor.coffee`. These definitions have to be deferred, because
   #      `Execution` isn't defined yet.
   receiver: undefined
   
   complete:-> !this.instructions.length

   # Responds with the *current* `Position`; i.e. the top element of the `instructions` stack.
   current:-> @instructions[0]
   
   # This method of the `Execution` types will copy all data relevant to advancement of the
   # execution to a `Execution` instance. This includes the pristine-ness (boolean), the `results`
   # and `instructions` stacks (or for a `Native`, any `bits`.) A clone made thus can be advanced
   # just as the original would have been, without affecting the original's position.
   # 
   # Of note: along with all the other data copied from the old instance, the new clone will inherit
   # the original `locals`. This is intentional.
   #---
   # TODO: nuke-API equivalent of lib-API's `branch()()`
   clone: (to)->
      super (to ?= new Execution)
      to.pristine    = @pristine
      
      to.locals      = @locals.clone().rename('locals')
      to.push Thing.pair 'locals', to.locals.owned()
      
      to.resumptions = @resumptions if @resumptions?
      
      if @instructions? and @results?
         to.instructions = @instructions.slice 0
         to.results      = @results.slice 0
      
      return to
   
   # This informs an `Execution` of the ‘result’ of the last `Combination` returned from `next`.
   # This value is stored in the `results` stack, and is later used as one of the values in furhter
   # `Combination`s.
   respond: (response)-> @results[0] = response
   
   # Returns the next `Combination` that needs to be preformed for the advancement of this
   # `Execution`. This is a mutating call, and each time it is called, it will produce a new
   # (subsequent) `Combination` for this `Execution`.
   #
   # It usually only makes sense for this to be called after a response to the *previous*
   # combination has been signaled by `respond` (obviously unless this is the first time it's being
   # advanced.) This also accepts an optional argument, the passing of which is identical to calling
   # `respond` with that value before-hand.
   # 
   # For combinations involving the start of a new expression, `null` will be returned as one part
   # of the `Combination`; this indicates no meaningful data from the stack for that node. (The
   # reactor will interpret this as an instruction to insert this `Execution`'s `locals` into that
   # combo, instead.)
   advance: (response)->
      return undefined if @complete()
      
      if this instanceof Native
         @pristine = no
         return @bits.shift()
      
      @respond response if response?
      
      # If we're continuing to advance a partially-completed `Execution`, ...
      completed = @instructions[0]
      previous_response = @results[0]
      if not @pristine
         
         # Gets the next instruction from the current sequence (via `Position#next`)
         @instructions[0] =
         upcoming = completed.next()
         
         # If we've completed the current sub-expression (a sequence.), then we're going to step out
         # (pop the stack.) and preform the indirected combination.
         if not upcoming?
            outer_current_value = @results[1]
            @instructions.shift(); @results.shift()
            return new Combination outer_current_value, previous_response
         
         # Discards the last response at the current stack-level, if this is the beginning of a new
         # semicolon-delimited expression.
         if upcoming.index is 0
            @results[0] =
            previous_response = null
         
         it = upcoming.valueOf()
        
         # If the current node is a `Thing`, we combine it against the top of the `results`-stack
         # (either another Thing, or `null` if this is the start of an expression.)
         if it instanceof Thing
            upcoming_value = it
            return new Combination previous_response, upcoming_value
         
         # If it's not a `Thing`, it must be a sub-expression (that is, a `parse.Sequence`).
         else
            upcoming = new Position it
            
            # If we've got what appears to be an empty sub-expression, then we're dealing with the
            # special-case syntax for referencing the Paws equivalent of `this`. We treat this like
            # a simple embedded-`Thing` combination, except with the current `Execution` as the
            # `Thing` in question:
            if not upcoming.valueOf()?
               return new Combination previous_response, this
            
            # Else, the ‘upcoming’ node is a real sub-expression, and we're going to ‘dive’ into it
            # (push it onto the stack.)
            @instructions.unshift upcoming; @results.unshift null
      
      # At this point, we're left at the *beginning* of a new expression. Either we'll be looking at
      # a `Thing`, or a nested series of ‘immediate’ (i.e. `[[[foo ...`) sub-expressions, eventually
      # ending in a `Thing` (since truly empty sub-expressions are impossible.)
      @pristine = no
      
      # If we *are* looking at another (or an arbitrary number of further) immediate
      # sub-expression(s), we need to push it (all of them) onto the stack.
      while (it = @instructions[0].valueOf())? and it not instanceof Thing
         @instructions.unshift new Position it; @results.unshift null
      
      upcoming = @instructions[0]
      
      # (another opportunity for an empty sub-expression / self-reference)
      if not upcoming.valueOf()?
         return new Combination previous_response, this
      
      # At this point, through one of several paths above, we've definitely descended into a (or
      # several) sub-expression(s), and are definitely looking at the first `Thing` in a new
      # expression.
      upcoming_value = upcoming.valueOf()
      return new Combination null, upcoming_value


Paws.Native = Native = class Native extends Execution
   
   # An `Native` is an `Execution` that's implemented with JavaScript code, instead of as a series
   # of Paws combinations. These are the primitive building-blocks with which Paws programs are
   # built.
   # 
   # Most `Native`s are exposed to Paws code through ‘bags’ of `Native`s stored under useful names
   # on the `locals` of the first (root-level) `Execution` in a Paws program; notably, the
   # primitives described by the Paws specification are exposed in such a bag, named
   # “infrastructure.”
   # 
   # `Native`s in this implementation consist of a series of ‘bits’, each of which is a JavaScript
   # `Function`. Each ‘bit’ of the `Native` implements a (clearly synchronous, as they are written
   # in JavaScript) set of operations to be preformed upon the next resumption of the `Execution`
   # represented by this `Native`. (It can be easier to conceptualize an `Native` as an explicit
   # coroutine, with the `return` statement of one bit and the arguments of the following bit acting
   # somewhat like a traditional `yield` statement.)
   # 
   # The first `Function`-bit will be invoked upon resumption of the `Native`, and then discarded
   # (thus causing the following `Function` to receive the resumption thereafter.) Upon resumption,
   # the `Function` will be invoked with the following arguments and environment:
   # 
   #  1. `result`, whatever resumption-value was queued to cause this resumption
   #  2. `unit` (optional), the `Unit` in which this resumption is relevant (thus providing access
   #     to the current `Unit`'s staging-`queue` and responsibility-`table`.)
   #  
   # In addition, the `this` at invocation will be the `Native` itself, giving the bit-body
   # convenient access to a place to store incrementally-constructed results. (Note that the actual
   # `Native` object referred to during the invocation of subsequent bits may *not* be the same, as
   # the `Native` may have been branched!)
   constructor: constructify(return:@) (@bits...)->
      delete @begin
      delete @instructions
      delete @results
      
      @resumptions = @bits.length
   
   complete: -> !this.bits.length
   
   clone: (to)->
      super (to ?= new Native)
      _.map Object.getOwnPropertyNames(this), (key)=> to[key] = this[key]
      to.bits = @bits.slice 0
      return to

   # This alternative constructor will automatically generate a series of ‘bits’ that will curry the
   # appropriate number of arguments into a single, final function.
   # 
   # Instead of having to write individual function-bits for your `Native` that collect the
   # appropriate set of resumption-values into a series of “arguments” that you need for your task,
   # you can use this convenience constructor for the common situation that you're treating an
   # `Execution` as equivalent to a synchronous JavaScript function.
   # 
   # ----
   # 
   # This takes a single function, and checks the number of arguments it requires before generating
   # the corresponding bits to acquire those arguments.
   # 
   # Then, once the resultant `Native` has been resumed the appropriate number of times (plus one
   # extra initial resumption with a `caller` as the resumption-value, as is standard coproductive
   # practice in Paws), the synchronous JavaScript passed in as the argument here will be invoked.
   # 
   # That invocation will provide the arguments recorded in the function's implementation, as well
   # as a context-object containing the following information as `this`:
   # 
   # caller
   #  : The first resumption-value provided to the generated `Native`. Usually, itself, an
   #    `Execution`, in the coproductive pattern.
   # this
   #  : The original `this`. That is, the generated `Native` that's currently being run.
   # unit
   #  : The current `Unit` at the time of realization, as provided by the reactor.
   # 
   # After your function executes, if it provides a non-null JavaScript return value, then the
   # `caller` provided as the first resumption-value Paws-side will be resumed one final time with
   # that as the resumption-value. (Hence the name of this method: it provides a ‘synchronous’ (ish)
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
         #  - Immediately (at generate-time), the locals we'll need within the body: the `Paws` API,
         #    and the `func` we were passed. This is necessary, because we're building the body in a
         #    new closure environment, via the `eval`-y `Function` constructor.
         #  - Second (later-on, at stage-time), the `caller` curried in by the first bit
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
      body.apply new Native


# Debugging output
# ----------------
T = Paws.debugging.tput

# Convenience to call whatever string-making methods are available on the passed object.
Paws.inspect = (object)->
   object?.inspect?() or
   object instanceof Thing && Thing::inspect.apply(object) or
   util.inspect object


Thing.inspectID = (it)-> it.id.slice(-8)

Thing::toString = ->
   output = Thing.inspectID(this) + (if @name? then ': '+T.bold @name else '')
   if @_?.tag == no then output else '<'+(@constructor.__name__ or @constructor.name)+' '+output+'>'

Thing::inspect = ->
   @toString()

Label::toString = ->
   output = '“'+@alien+'”' + (if @name? then ': '+T.bold @name else '')
   if @_?.tag == no then output else '<'+(@constructor.__name__ or @constructor.name)+' '+output+'>'

Execution::toString = ->
   output = Thing.inspectID(this) +
      (if @name? then ': '+T.bold @name else '') +
      (if @resumptions? then new Array(@resumptions - @bits.length).join('[]') else '')
   if @_?.tag == no then output else '<'+(@constructor.__name__ or @constructor.name)+' '+output+'>'
