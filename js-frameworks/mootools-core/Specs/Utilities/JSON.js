/*
---
name: JSON
requires: ~
provides: ~
...
*/

describe('JSON', function(){

	it('should encode and decode an object', function(){
		var object = {
			a: [0, 1, 2],
			s: "It's-me-Valerio!",
			u: '\x01',
			n: 1,
			f: 3.14,
			b: false,
			nil: null,
			o: {
				a: 1,
				b: [1, 2],
				c: {
					a: 2,
					b: 3
				}
			}
		};

		expect(JSON.decode(JSON.encode(object))).toEqual(object);
	});

});
describe('JSON', function(){

    var goodString = '{"name":"Jim Cowart","location":{"city":{"name":"Chattanooga","population":167674}}}';
    var badString = 'alert("I\'m a bad string!")';

    it('should parse a valid JSON string by default', function(){
        expect(typeOf(JSON.decode(goodString))).toEqual("object");
    });

    it('should parse a valid JSON string when secure is set to false', function(){
        expect(typeOf(JSON.decode(goodString, false))).toEqual("object");
    });

    it('should parse a hazarous string when secure is set to false', function(){
        var _old_alert = window.alert;
        window.alert = function (string) {
            if (string == "I'm a bad string!") return true;
            return false;
        };
        expect(JSON.decode(badString, false)).toEqual(true);
        window.alert = _old_alert;
    }); 
    it('should parse a hazarous string when JSON.secure is set to false and secure is not defined', function(){
        var _old_alert = window.alert;
        window.alert = function (string) {
            if (string == "I'm a bad string!") return true;
            return false;
        };
        JSON.secure = false;
        expect(JSON.decode(badString)).toEqual(true);
        window.alert = _old_alert;
        JSON.secure = true;
    });     
    it('should NOT parse a hazarous string by default', function(){
        var err;
        try {
            JSON.decode(badString);
        } catch (e){
            err = !!e;
        };
        expect(err).toEqual(true);
    });  

});
