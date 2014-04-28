expect  = require 'expect.js'
Paws = require '../Source/Paws.coffee'

Assertion = expect.Assertion
i = expect.stringify

Assertion::responsible = ->
   expect(@obj).to.be.a(Paws.Relation);
   
   this.assert @obj.isResponsible,
      (-> 'expected ' + i(@obj) + ' to be responsible' ),
      (-> 'expected ' + i(@obj) + ' to be irresponsible' )