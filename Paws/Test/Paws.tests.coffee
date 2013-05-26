`require = require('../Source/cov_require.js')(require)`
expect = require 'expect.js'

# TODO: Replace all the 'should' language with more direct 'will' language
#       (i.e. “it should return ...” becomes “it returns ...”
describe 'The Paws API:', ->
   Paws = require "../Source/Paws.coffee"
   it 'should be defined', ->
      expect(Paws).to.be.ok()
   
   Thing = Paws.Thing
   Label = Paws.Label
   
   describe 'Thing', ->
      
      Relation = Paws.Relation
      describe 'Relation', ->
         describe '##from', ->
            it 'should return the passed value if it is already a Relation', ->
               rel = new Relation(new Thing)
               expect(Relation.from rel).to.be rel
            it 'should return a relation *to* the passed value if not', ->
               thing = new Thing
               expect(Relation.from thing).to.be.a Relation
               expect(Relation.from(thing).to).to.be thing
            
            it 'should ensure all elements of a passed array are Relations', ->
               thing1 = new Thing; thing2 = new Thing; thing3 = new Thing
               array = [thing1, new Relation(thing2), thing3]
               expect(Relation.from array).to.be.an 'array'
               expect(Relation.from(array).every (el) -> el instanceof Relation).to.be.ok()
            
            it 'should be able to change the responsibility of relations', ->
               thing1 = new Thing; thing2 = new Thing; thing3 = new Thing
               array = [thing1, new Relation(thing2, false), new Relation(thing3, true)]
               expect( # TODO: This could be cleaner.
                  Relation.with(responsible: yes).from(array).every (el) -> el.isResponsible
               ).to.be.ok()
               
         it 'should default to being irresponsible', ->
            expect((new Relation).isResponsible).to.be false
         
         describe '#clone', ->
            it 'should return a new Relation', ->
               rel = new Relation(new Thing, true)
               expect(rel.clone()).to.not.be rel
               expect(rel.clone().to).to.be rel.to
               expect(rel.clone().isResponsible).to.be rel.isResponsible
         
      describe '##pair', ->
      
      uuid_regex = /[a-f0-9]{8}-[a-f0-9]{4}-4[a-f0-9]{3}-[89aAbB][a-f0-9]{3}-[a-f0-9]{12}/
      it 'should have a UUID', ->
         expect((new Thing).id).to.match uuid_regex
      
      it 'should noughtify the metadata by default', ->
         thing = new Thing
         expect(thing.metadata).to.have.length 1
         expect(thing.metadata[0]).to.be undefined
         
         bare_thing = new Thing.with(noughtify: no)()
         expect(bare_thing.metadata).to.have.length 0
      
      it 'should store metadata relations', ->
         child1 = new Thing; child2 = new Thing
         thing = new Thing child1, child2
         expect(thing).to.have.property 'metadata'
         expect(thing.metadata).to.be.an 'array'
         expect(thing.metadata[1]).to.be.a Relation
         expect(thing.metadata[1].to).to.be child1
         expect(thing.metadata[2]).to.be.a Relation
         expect(thing.metadata[2].to).to.be child2
      
      describe '#clone', ->
         it 'should return a new Thing', ->
            thing = new Thing
            expect(thing.clone()).to.not.be thing
         it 'should have identical metadata', ->
            thing = new Thing new Thing, new Thing, new Thing
            clone = thing.clone()
            expect(clone.metadata).to.have.length 4
            thing.metadata.forEach (rel, i) -> if rel
               expect(clone.at i).to.be.ok()
               expect(rel).not.to.be clone.metadata[i]
               expect(rel.to).to.be clone.metadata[i].to
         it 'should apply new metadata to any passed Thing', ->
            thing1 = new Thing new Thing, new Thing, new Thing
            thing2 = new Thing new Thing
            old_metadata = thing2.metadata
            
            result = thing1.clone(thing2)
            expect(result).to.be thing2
            expect(thing2.metadata).to.not.be old_metadata
      
      describe '#find', ->
         first = new Thing; second = new Thing; third = new Thing
         foo_bar_foo = new Thing Thing.pair('foo', first),
                                 Thing.pair('bar', second),
                                 Thing.pair('foo', third)
         
         it 'should return an array ...', ->
            expect(foo_bar_foo.find Label 'foo').to.be.an 'array'
         it '... of result-Things ...', ->
            expect(foo_bar_foo.find(Label 'foo').length).to.be.greaterThan 0
            foo_bar_foo.find(Label 'foo').forEach (result) ->
               expect(result).to.be.a Thing
         it '... that only contains matching pairs ...', ->
            expect(foo_bar_foo.find Label 'foo').to.have.length 2
         it '... in reverse order', ->
            expect(foo_bar_foo.find(Label 'foo')[0].valueish()).to.be third
            expect(foo_bar_foo.find(Label 'foo')[1].valueish()).to.be first
         
         it 'should handle non-pair Things gracefully', ->
            thing = new Thing Thing.pair('foo', first),
                              new Thing,
                              Thing.pair('bar', second),
                              Thing.pair('foo', third)
            expect(thing.find Label 'foo').to.have.length 2
   
   
   describe 'Label', ->
      it 'should contain a String', ->
         foo = new Label 'foo'
         expect(foo).to.be.a Thing
         expect(foo.alien).to.be.a String
         expect(foo.alien.valueOf()).to.be 'foo' # temporary hack. see: http://git.io/expect.js-57
      
      it 'should compare as equal, when containing the same String', ->
         foo1 = new Label 'foo'
         foo2 = new Label 'foo'
         expect(foo1.compare foo2).to.be true
