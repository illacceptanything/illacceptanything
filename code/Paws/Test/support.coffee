expect  = require 'expect.js'
Paws = require '../Source/Paws.coffee'

Assertion = expect.Assertion
i = expect.stringify

Assertion::owned = ->
   expect(@obj).to.be.a(Paws.Relation);
   
   this.assert @obj.owns,
      (-> 'expected ' + i(@obj) + ' to be owning' ),
      (-> 'expected ' + i(@obj) + ' to not be owning' )
