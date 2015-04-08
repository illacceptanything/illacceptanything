/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
YUI().use('mojito-util', 'mojito-test-extra', 'test', 'array-extras', function(Y) {

    var suite = new Y.Test.Suite('util tests'),
        cases = {},
        A = Y.Assert,
        AA = Y.ArrayAssert,
        OA = Y.ObjectAssert;

    cases = {
        name: 'functional',

        'blend should work on arrays': function() {
            var base = [0, 1, 2, 3],
                over = ['a', 'b'];

            AA.itemsAreEqual([0, 1, 2, 3, 'a', 'b'],
                Y.mojito.util.blend(base, over),
                     'Array values should properly blend.');
        },
        
        'blend should unique arrays': function() {
            var base = [0, 1, 2, 3],
                over = ['a', 'b', 1];

            AA.itemsAreEqual([0, 1, 2, 3, 'a', 'b'],
                Y.mojito.util.blend(base, over),
                     'Array values should blend uniquely.');
        },
        
        'blend should work on objects': function() {
            var base = {
                    a: 1,
                    b: 2
                },
                over = {
                    c: 3,
                    d: 4
                },
                blended = {
                    a: 1,
                    b: 2,
                    c: 3,
                    d: 4
                };

            OA.areEqual(blended, Y.mojito.util.blend(base, over));
        },

        'blend should replace object values': function() {
            var base = {
                    a: 1,
                    b: 2
                },
                over = {
                    c: 3,
                    a: 4
                },
                blended = {
                    a: 4,
                    b: 2,
                    c: 3
                };

            OA.areEqual(blended, Y.mojito.util.blend(base, over));
        },
        
        'blend should handle nested merges': function() {
            var base = {
                    a: 1,
                    b: 2,
                    c: {
                        foo: 1
                    }
                },
                over = {
                    c: {
                        bar: 2
                    }
                },
                blended = {
                    a: 1,
                    b: 2,
                    c: {
                        foo: 1,
                        bar: 2
                    }
                };

            OA.areEqual(blended.c, Y.mojito.util.blend(base, over).c);
        },

        'blend should handle nested merges with replacements': function() {
            var base = {
                    a: 1,
                    b: 2,
                    c: {
                        foo: 1,
                        baz: 3
                    }
                },
                over = {
                    a: 4,
                    c: {
                        foo: 3,
                        bar: 2
                    }
                },
                blended = {
                    a: 4,
                    b: 2,
                    c: {
                        foo: 3,
                        bar: 2,
                        baz: 3
                    }
                };

            OA.areEqual(blended.c, Y.mojito.util.blend(base, over).c);
        },

        'blend value type matrix': function() {
            // positions:  base, overlay
            // s = scalar
            // o = object
            // a = array
            // n = null
            // u = undefined
            // m = missing (not given)
            var base = {
                'ss': 'base-ss',
                'so': 'base-so',
                'sa': 'base-sa',
                'sn': 'base-sn',
                'su': 'base-su',
                'sm': 'base-sm',
                'os': { 'base': 'os' },
                'oo': { 'base': 'oo' },
                'oa': { 'base': 'oa' },
                'on': { 'base': 'on' },
                'ou': { 'base': 'ou' },
                'om': { 'base': 'om' },
                'as': [ 'base-as' ],
                'ao': [ 'base-ao' ],
                'aa': [ 'base-aa' ],
                'an': [ 'base-an' ],
                'au': [ 'base-au' ],
                'am': [ 'base-am' ],
                'ns': null,
                'no': null,
                'na': null,
                'nn': null,
                'nu': null,
                'nm': null,
                'us': undefined,
                'uo': undefined,
                'ua': undefined,
                'un': undefined,
                'uu': undefined,
                'um': undefined,
            };
            var overlay = {
                'ss': 'overlay-ss',
                'so': { 'overlay': 'so' },
                'sa': [ 'overlay-sa' ],
                'sn': null,
                'su': undefined,
                'os': 'overlay-os',
                'oo': { 'overlay': 'oo' },
                'oa': [ 'overlay-oa' ],
                'on': null,
                'ou': undefined,
                'as': 'overlay-as',
                'ao': { 'overlay': 'ao' },
                'aa': [ 'overlay-aa' ],
                'an': null,
                'au': undefined,
                'ns': 'overlay-ns',
                'no': { 'overlay': 'no' },
                'na': [ 'overlay-na' ],
                'nn': null,
                'nu': undefined,
                'us': 'overlay-us',
                'uo': { 'overlay': 'uo' },
                'ua': [ 'overlay-ua' ],
                'un': null,
                'uu': undefined,
                'ms': 'overlay-ms',
                'mo': { 'overlay': 'mo' },
                'ma': [ 'overlay-ma' ],
                'mn': null,
                'mu': undefined,
            };
            var have = Y.mojito.util.blend(base, overlay);
            var want = {
                'ss': 'overlay-ss',
                'so': { 'overlay': 'so' },
                'sa': [ 'overlay-sa' ],
                'sn': null,
                'su': undefined,
                'os': 'overlay-os',
                'oo': { 'overlay': 'oo', 'base': 'oo' },
                'oa': [ 'overlay-oa' ],
                'on': null,
                'ou': undefined,
                'as': 'overlay-as',
                'ao': { 'overlay': 'ao' },
                'aa': [ 'base-aa', 'overlay-aa' ],
                'an': null,
                'au': undefined,
                'ns': 'overlay-ns',
                'no': { 'overlay': 'no' },
                'na': [ 'overlay-na' ],
                'nn': null,
                'nu': undefined,
                'us': 'overlay-us',
                'uo': { 'overlay': 'uo' },
                'ua': [ 'overlay-ua' ],
                'un': null,
                'uu': undefined,
                'ms': 'overlay-ms',
                'mo': { 'overlay': 'mo' },
                'ma': [ 'overlay-ma' ],
                'mn': null,
                'mu': undefined,
                'sm': 'base-sm',
                'om': { 'base': 'om' },
                'am': [ 'base-am' ],
                'nm': null,
                'um': undefined
            };
            Y.TEST_CMP(want, have);
        },

        'blend3 should work on arrays': function() {
            var low = [0, 1, 2, 3];
            var med = ['a', 'b'];
            var hig = ['mic', 'mac'];
            var have = Y.mojito.util.blend3(low, med, hig);
            var want = [0, 1, 2, 3, 'a', 'b', 'mic', 'mac'];
            AA.itemsAreEqual(want, have, 'Array values should blend3 concat');
        },

        'blend3 should unique arrays': function() {
            var low = [0, 1, 2, 3];
            var med = ['a', 'b', 1];
            var hig = ['mic', 'mac', 'a', 2];
            var have = Y.mojito.util.blend3(low, med, hig);
            var want = [0, 1, 2, 3, 'a', 'b', 'mic', 'mac'];
            AA.itemsAreEqual(want, have, 'Array values should blend3 uniquely');
        },

        'blend3 should work on object': function() {
            var low = {a: 1, b: 2};
            var med = {c: 3, d: 4};
            var hig = {e: 5, f: 6};
            var have = Y.mojito.util.blend3(low, med, hig);
            var want = {a: 1, b: 2, c: 3, d: 4, e: 5, f: 6};
            OA.areEqual(want, have, 'object values should blend3');
        },

        'blend3 should replace object values': function() {
            var low = {a: 1, b: 2};
            var med = {c: 3, d: 4, a: 'med'};
            var hig = {e: 5, f: 6, b: 'hig', c: 'hig'};
            var have = Y.mojito.util.blend3(low, med, hig);
            var want = {a: 'med', b: 'hig', c: 'hig', d: 4, e: 5, f: 6};
            OA.areEqual(want, have, 'object values should blend3 with replacement');
        },

        'blend3 objects with null should replace': function() {
            var low = {a: 1, b: 2};
            var med = {c: 3, d: 4, a: null};
            var hig = {e: 5, f: 6, b: null, c: null};
            var have = Y.mojito.util.blend3(low, med, hig);
            var want = {a: null, b: null, c: null, d: 4, e: 5, f: 6};
            OA.areEqual(want, have, 'object values should blend3 with null replacement');
        },

        'blend3 value type matrix': function() {
            // positions:  lowest, medium, highest
            // s = scalar
            // o = object
            // a = array
            // n = null
            // u = undefined
            // m = missing (not given)
            var low = {
                'sss': 'low-sss',
                'sso': 'low-sso',
                'ssa': 'low-ssa',
                'ssn': 'low-ssn',
                'ssu': 'low-ssu',
                'ssm': 'low-ssm',
                'sos': 'low-sos',
                'soo': 'low-soo',
                'soa': 'low-soa',
                'son': 'low-son',
                'sou': 'low-sou',
                'som': 'low-som',
                'sas': 'low-sas',
                'sao': 'low-sao',
                'saa': 'low-saa',
                'san': 'low-san',
                'sau': 'low-sau',
                'sam': 'low-sam',
                'sns': 'low-sns',
                'sno': 'low-sno',
                'sna': 'low-sna',
                'snn': 'low-snn',
                'snu': 'low-snu',
                'snm': 'low-snm',
                'sus': 'low-sus',
                'suo': 'low-suo',
                'sua': 'low-sua',
                'sun': 'low-sun',
                'suu': 'low-suu',
                'sum': 'low-sum',
                'sms': 'low-sms',
                'smo': 'low-smo',
                'sma': 'low-sma',
                'smn': 'low-smn',
                'smu': 'low-smu',
                'smm': 'low-smm',
                'oss': { 'low': 'oss' },
                'oso': { 'low': 'oso' },
                'osa': { 'low': 'osa' },
                'osn': { 'low': 'osn' },
                'osu': { 'low': 'osu' },
                'osm': { 'low': 'osm' },
                'oos': { 'low': 'oos' },
                'ooo': { 'low': 'ooo' },
                'ooa': { 'low': 'ooa' },
                'oon': { 'low': 'oon' },
                'oou': { 'low': 'oou' },
                'oom': { 'low': 'oom' },
                'oas': { 'low': 'oas' },
                'oao': { 'low': 'oao' },
                'oaa': { 'low': 'oaa' },
                'oan': { 'low': 'oan' },
                'oau': { 'low': 'oau' },
                'oam': { 'low': 'oam' },
                'ons': { 'low': 'ons' },
                'ono': { 'low': 'ono' },
                'ona': { 'low': 'ona' },
                'onn': { 'low': 'onn' },
                'onu': { 'low': 'onu' },
                'onm': { 'low': 'onm' },
                'ous': { 'low': 'ous' },
                'ouo': { 'low': 'ouo' },
                'oua': { 'low': 'oua' },
                'oun': { 'low': 'oun' },
                'ouu': { 'low': 'ouu' },
                'oum': { 'low': 'oum' },
                'oms': { 'low': 'oms' },
                'omo': { 'low': 'omo' },
                'oma': { 'low': 'oma' },
                'omn': { 'low': 'omn' },
                'omu': { 'low': 'omu' },
                'omm': { 'low': 'omm' },
                'ass': [ 'low-ass' ],
                'aso': [ 'low-aso' ],
                'asa': [ 'low-asa' ],
                'asn': [ 'low-asn' ],
                'asu': [ 'low-asu' ],
                'asm': [ 'low-asm' ],
                'aos': [ 'low-aos' ],
                'aoo': [ 'low-aoo' ],
                'aoa': [ 'low-aoa' ],
                'aon': [ 'low-aon' ],
                'aou': [ 'low-aou' ],
                'aom': [ 'low-aom' ],
                'aas': [ 'low-aas' ],
                'aao': [ 'low-aao' ],
                'aaa': [ 'low-aaa' ],
                'aan': [ 'low-aan' ],
                'aau': [ 'low-aau' ],
                'aam': [ 'low-aam' ],
                'ans': [ 'low-ans' ],
                'ano': [ 'low-ano' ],
                'ana': [ 'low-ana' ],
                'ann': [ 'low-ann' ],
                'anu': [ 'low-anu' ],
                'anm': [ 'low-anm' ],
                'aus': [ 'low-aus' ],
                'auo': [ 'low-auo' ],
                'aua': [ 'low-aua' ],
                'aun': [ 'low-aun' ],
                'auu': [ 'low-auu' ],
                'aum': [ 'low-aum' ],
                'ams': [ 'low-ams' ],
                'amo': [ 'low-amo' ],
                'ama': [ 'low-ama' ],
                'amn': [ 'low-amn' ],
                'amu': [ 'low-amu' ],
                'amm': [ 'low-amm' ],
                'nss': null,
                'nso': null,
                'nsa': null,
                'nsn': null,
                'nsu': null,
                'nsm': null,
                'nos': null,
                'noo': null,
                'noa': null,
                'non': null,
                'nou': null,
                'nom': null,
                'nas': null,
                'nao': null,
                'naa': null,
                'nan': null,
                'nau': null,
                'nam': null,
                'nns': null,
                'nno': null,
                'nna': null,
                'nnn': null,
                'nnu': null,
                'nnm': null,
                'nus': null,
                'nuo': null,
                'nua': null,
                'nun': null,
                'nuu': null,
                'num': null,
                'nms': null,
                'nmo': null,
                'nma': null,
                'nmn': null,
                'nmu': null,
                'nmm': null,
                'uss': undefined,
                'uso': undefined,
                'usa': undefined,
                'usn': undefined,
                'usu': undefined,
                'usm': undefined,
                'uos': undefined,
                'uoo': undefined,
                'uoa': undefined,
                'uon': undefined,
                'uou': undefined,
                'uom': undefined,
                'uas': undefined,
                'uao': undefined,
                'uaa': undefined,
                'uan': undefined,
                'uau': undefined,
                'uam': undefined,
                'uns': undefined,
                'uno': undefined,
                'una': undefined,
                'unn': undefined,
                'unu': undefined,
                'unm': undefined,
                'uus': undefined,
                'uuo': undefined,
                'uua': undefined,
                'uun': undefined,
                'uuu': undefined,
                'uum': undefined,
                'ums': undefined,
                'umo': undefined,
                'uma': undefined,
                'umn': undefined,
                'umu': undefined,
                'umm': undefined,
            };
            var med = {
                'sss': 'med-sss',
                'sso': 'med-sso',
                'ssa': 'med-ssa',
                'ssn': 'med-ssn',
                'ssu': 'med-ssu',
                'ssm': 'med-ssm',
                'sos': { 'med': 'sos' },
                'soo': { 'med': 'soo' },
                'soa': { 'med': 'soa' },
                'son': { 'med': 'son' },
                'sou': { 'med': 'sou' },
                'som': { 'med': 'som' },
                'sas': [ 'med-sas' ],
                'sao': [ 'med-sao' ],
                'saa': [ 'med-saa' ],
                'san': [ 'med-san' ],
                'sau': [ 'med-sau' ],
                'sam': [ 'med-sam' ],
                'sns': null,
                'sno': null,
                'sna': null,
                'snn': null,
                'snu': null,
                'snm': null,
                'sus': undefined,
                'suo': undefined,
                'sua': undefined,
                'sun': undefined,
                'suu': undefined,
                'sum': undefined,
                'oss': 'med-oss',
                'oso': 'med-oso',
                'osa': 'med-osa',
                'osn': 'med-osn',
                'osu': 'med-osu',
                'osm': 'med-osm',
                'oos': { 'med': 'oos' },
                'ooo': { 'med': 'ooo' },
                'ooa': { 'med': 'ooa' },
                'oon': { 'med': 'oon' },
                'oou': { 'med': 'oou' },
                'oom': { 'med': 'oom' },
                'oas': [ 'med-oas' ],
                'oao': [ 'med-oao' ],
                'oaa': [ 'med-oaa' ],
                'oan': [ 'med-oan' ],
                'oau': [ 'med-oau' ],
                'oam': [ 'med-oam' ],
                'ons': null,
                'ono': null,
                'ona': null,
                'onn': null,
                'onu': null,
                'onm': null,
                'ous': undefined,
                'ouo': undefined,
                'oua': undefined,
                'oun': undefined,
                'ouu': undefined,
                'oum': undefined,
                'ass': 'med-ass',
                'aso': 'med-aso',
                'asa': 'med-asa',
                'asn': 'med-asn',
                'asu': 'med-asu',
                'asm': 'med-asm',
                'aos': { 'med': 'aos' },
                'aoo': { 'med': 'aoo' },
                'aoa': { 'med': 'aoa' },
                'aon': { 'med': 'aon' },
                'aou': { 'med': 'aou' },
                'aom': { 'med': 'aom' },
                'aas': [ 'med-aas' ],
                'aao': [ 'med-aao' ],
                'aaa': [ 'med-aaa' ],
                'aan': [ 'med-aan' ],
                'aau': [ 'med-aau' ],
                'aam': [ 'med-aam' ],
                'ans': null,
                'ano': null,
                'ana': null,
                'ann': null,
                'anu': null,
                'anm': null,
                'aus': undefined,
                'auo': undefined,
                'aua': undefined,
                'aun': undefined,
                'auu': undefined,
                'aum': undefined,
                'nss': 'med-nss',
                'nso': 'med-nso',
                'nsa': 'med-nsa',
                'nsn': 'med-nsn',
                'nsu': 'med-nsu',
                'nsm': 'med-nsm',
                'nos': { 'med': 'nos' },
                'noo': { 'med': 'noo' },
                'noa': { 'med': 'noa' },
                'non': { 'med': 'non' },
                'nou': { 'med': 'nou' },
                'nom': { 'med': 'nom' },
                'nas': [ 'med-nas' ],
                'nao': [ 'med-nao' ],
                'naa': [ 'med-naa' ],
                'nan': [ 'med-nan' ],
                'nau': [ 'med-nau' ],
                'nam': [ 'med-nam' ],
                'nns': null,
                'nno': null,
                'nna': null,
                'nnn': null,
                'nnu': null,
                'nnm': null,
                'nus': undefined,
                'nuo': undefined,
                'nua': undefined,
                'nun': undefined,
                'nuu': undefined,
                'num': undefined,
                'uss': 'med-uss',
                'uso': 'med-uso',
                'usa': 'med-usa',
                'usn': 'med-usn',
                'usu': 'med-usu',
                'usm': 'med-usm',
                'uos': { 'med': 'uos' },
                'uoo': { 'med': 'uoo' },
                'uoa': { 'med': 'uoa' },
                'uon': { 'med': 'uon' },
                'uou': { 'med': 'uou' },
                'uom': { 'med': 'uom' },
                'uas': [ 'med-uas' ],
                'uao': [ 'med-uao' ],
                'uaa': [ 'med-uaa' ],
                'uan': [ 'med-uan' ],
                'uau': [ 'med-uau' ],
                'uam': [ 'med-uam' ],
                'uns': null,
                'uno': null,
                'una': null,
                'unn': null,
                'unu': null,
                'unm': null,
                'uus': undefined,
                'uuo': undefined,
                'uua': undefined,
                'uun': undefined,
                'uuu': undefined,
                'uum': undefined,
                'mss': 'med-mss',
                'mso': 'med-mso',
                'msa': 'med-msa',
                'msn': 'med-msn',
                'msu': 'med-msu',
                'msm': 'med-msm',
                'mos': { 'med': 'mos' },
                'moo': { 'med': 'moo' },
                'moa': { 'med': 'moa' },
                'mon': { 'med': 'mon' },
                'mou': { 'med': 'mou' },
                'mom': { 'med': 'mom' },
                'mas': [ 'med-mas' ],
                'mao': [ 'med-mao' ],
                'maa': [ 'med-maa' ],
                'man': [ 'med-man' ],
                'mau': [ 'med-mau' ],
                'mam': [ 'med-mam' ],
                'mns': null,
                'mno': null,
                'mna': null,
                'mnn': null,
                'mnu': null,
                'mnm': null,
                'mus': undefined,
                'muo': undefined,
                'mua': undefined,
                'mun': undefined,
                'muu': undefined,
                'mum': undefined,
            };
            var hig = {
                'sss': 'hig-sss',
                'sso': { 'hig': 'sso' },
                'ssa': [ 'hig-ssa' ],
                'ssn': null,
                'ssu': undefined,
                'sos': 'hig-sos',
                'soo': { 'hig': 'soo' },
                'soa': [ 'hig-soa' ],
                'son': null,
                'sou': undefined,
                'sas': 'hig-sas',
                'sao': { 'hig': 'sao' },
                'saa': [ 'hig-saa' ],
                'san': null,
                'sau': undefined,
                'sns': 'hig-sns',
                'sno': { 'hig': 'sno' },
                'sna': [ 'hig-sna' ],
                'snn': null,
                'snu': undefined,
                'sus': 'hig-sus',
                'suo': { 'hig': 'suo' },
                'sua': [ 'hig-sua' ],
                'sun': null,
                'suu': undefined,
                'sms': 'hig-sms',
                'smo': { 'hig': 'smo' },
                'sma': [ 'hig-sma' ],
                'smn': null,
                'smu': undefined,
                'oss': 'hig-oss',
                'oso': { 'hig': 'oso' },
                'osa': [ 'hig-osa' ],
                'osn': null,
                'osu': undefined,
                'oos': 'hig-oos',
                'ooo': { 'hig': 'ooo' },
                'ooa': [ 'hig-ooa' ],
                'oon': null,
                'oou': undefined,
                'oas': 'hig-oas',
                'oao': { 'hig': 'oao' },
                'oaa': [ 'hig-oaa' ],
                'oan': null,
                'oau': undefined,
                'ons': 'hig-ons',
                'ono': { 'hig': 'ono' },
                'ona': [ 'hig-ona' ],
                'onn': null,
                'onu': undefined,
                'ous': 'hig-ous',
                'ouo': { 'hig': 'ouo' },
                'oua': [ 'hig-oua' ],
                'oun': null,
                'ouu': undefined,
                'oms': 'hig-oms',
                'omo': { 'hig': 'omo' },
                'oma': [ 'hig-oma' ],
                'omn': null,
                'omu': undefined,
                'ass': 'hig-ass',
                'aso': { 'hig': 'aso' },
                'asa': [ 'hig-asa' ],
                'asn': null,
                'asu': undefined,
                'aos': 'hig-aos',
                'aoo': { 'hig': 'aoo' },
                'aoa': [ 'hig-aoa' ],
                'aon': null,
                'aou': undefined,
                'aas': 'hig-aas',
                'aao': { 'hig': 'aao' },
                'aaa': [ 'hig-aaa' ],
                'aan': null,
                'aau': undefined,
                'ans': 'hig-ans',
                'ano': { 'hig': 'ano' },
                'ana': [ 'hig-ana' ],
                'ann': null,
                'anu': undefined,
                'aus': 'hig-aus',
                'auo': { 'hig': 'auo' },
                'aua': [ 'hig-aua' ],
                'aun': null,
                'auu': undefined,
                'ams': 'hig-ams',
                'amo': { 'hig': 'amo' },
                'ama': [ 'hig-ama' ],
                'amn': null,
                'amu': undefined,
                'nss': 'hig-nss',
                'nso': { 'hig': 'nso' },
                'nsa': [ 'hig-nsa' ],
                'nsn': null,
                'nsu': undefined,
                'nos': 'hig-nos',
                'noo': { 'hig': 'noo' },
                'noa': [ 'hig-noa' ],
                'non': null,
                'nou': undefined,
                'nas': 'hig-nas',
                'nao': { 'hig': 'nao' },
                'naa': [ 'hig-naa' ],
                'nan': null,
                'nau': undefined,
                'nns': 'hig-nns',
                'nno': { 'hig': 'nno' },
                'nna': [ 'hig-nna' ],
                'nnn': null,
                'nnu': undefined,
                'nus': 'hig-nus',
                'nuo': { 'hig': 'nuo' },
                'nua': [ 'hig-nua' ],
                'nun': null,
                'nuu': undefined,
                'nms': 'hig-nms',
                'nmo': { 'hig': 'nmo' },
                'nma': [ 'hig-nma' ],
                'nmn': null,
                'nmu': undefined,
                'uss': 'hig-uss',
                'uso': { 'hig': 'uso' },
                'usa': [ 'hig-usa' ],
                'usn': null,
                'usu': undefined,
                'uos': 'hig-uos',
                'uoo': { 'hig': 'uoo' },
                'uoa': [ 'hig-uoa' ],
                'uon': null,
                'uou': undefined,
                'uas': 'hig-uas',
                'uao': { 'hig': 'uao' },
                'uaa': [ 'hig-uaa' ],
                'uan': null,
                'uau': undefined,
                'uns': 'hig-uns',
                'uno': { 'hig': 'uno' },
                'una': [ 'hig-una' ],
                'unn': null,
                'unu': undefined,
                'uus': 'hig-uus',
                'uuo': { 'hig': 'uuo' },
                'uua': [ 'hig-uua' ],
                'uun': null,
                'uuu': undefined,
                'ums': 'hig-ums',
                'umo': { 'hig': 'umo' },
                'uma': [ 'hig-uma' ],
                'umn': null,
                'umu': undefined,
                'mss': 'hig-mss',
                'mso': { 'hig': 'mso' },
                'msa': [ 'hig-msa' ],
                'msn': null,
                'msu': undefined,
                'mos': 'hig-mos',
                'moo': { 'hig': 'moo' },
                'moa': [ 'hig-moa' ],
                'mon': null,
                'mou': undefined,
                'mas': 'hig-mas',
                'mao': { 'hig': 'mao' },
                'maa': [ 'hig-maa' ],
                'man': null,
                'mau': undefined,
                'mns': 'hig-mns',
                'mno': { 'hig': 'mno' },
                'mna': [ 'hig-mna' ],
                'mnn': null,
                'mnu': undefined,
                'mus': 'hig-mus',
                'muo': { 'hig': 'muo' },
                'mua': [ 'hig-mua' ],
                'mun': null,
                'muu': undefined,
                'mms': 'hig-mms',
                'mmo': { 'hig': 'mmo' },
                'mma': [ 'hig-mma' ],
                'mmn': null,
                'mmu': undefined,
            };
            var have = Y.mojito.util.blend3(low, med, hig);
            var want = {
                sss: 'hig-sss',
                sso: { hig: 'sso' },
                ssa: [ 'hig-ssa' ],
                ssn: null,
                ssu: undefined,
                sos: 'hig-sos',
                soo: { hig: 'soo', med: 'soo' },
                soa: [ 'hig-soa' ],
                son: null,
                sou: undefined,
                sas: 'hig-sas',
                sao: { hig: 'sao' },
                saa: [ 'med-saa', 'hig-saa' ],
                san: null,
                sau: undefined,
                sns: 'hig-sns',
                sno: { hig: 'sno' },
                sna: [ 'hig-sna' ],
                snn: null,
                snu: undefined,
                sus: 'hig-sus',
                suo: { hig: 'suo' },
                sua: [ 'hig-sua' ],
                sun: null,
                suu: undefined,
                sms: 'hig-sms',
                smo: { hig: 'smo' },
                sma: [ 'hig-sma' ],
                smn: null,
                smu: undefined,
                oss: 'hig-oss',
                oso: { hig: 'oso' },
                osa: [ 'hig-osa' ],
                osn: null,
                osu: undefined,
                oos: 'hig-oos',
                ooo: { hig: 'ooo', med: 'ooo', low: 'ooo' },
                ooa: [ 'hig-ooa' ],
                oon: null,
                oou: undefined,
                oas: 'hig-oas',
                oao: { hig: 'oao' },
                oaa: [ 'med-oaa', 'hig-oaa' ],
                oan: null,
                oau: undefined,
                ons: 'hig-ons',
                ono: { hig: 'ono' },
                ona: [ 'hig-ona' ],
                onn: null,
                onu: undefined,
                ous: 'hig-ous',
                ouo: { hig: 'ouo' },
                oua: [ 'hig-oua' ],
                oun: null,
                ouu: undefined,
                oms: 'hig-oms',
                omo: { hig: 'omo', low: 'omo' },
                oma: [ 'hig-oma' ],
                omn: null,
                omu: undefined,
                ass: 'hig-ass',
                aso: { hig: 'aso' },
                asa: [ 'hig-asa' ],
                asn: null,
                asu: undefined,
                aos: 'hig-aos',
                aoo: { hig: 'aoo', med: 'aoo' },
                aoa: [ 'hig-aoa' ],
                aon: null,
                aou: undefined,
                aas: 'hig-aas',
                aao: { hig: 'aao' },
                aaa: [ 'low-aaa', 'med-aaa', 'hig-aaa' ],
                aan: null,
                aau: undefined,
                ans: 'hig-ans',
                ano: { hig: 'ano' },
                ana: [ 'hig-ana' ],
                ann: null,
                anu: undefined,
                aus: 'hig-aus',
                auo: { hig: 'auo' },
                aua: [ 'hig-aua' ],
                aun: null,
                auu: undefined,
                ams: 'hig-ams',
                amo: { hig: 'amo' },
                ama: [ 'low-ama', 'hig-ama' ],
                amn: null,
                amu: undefined,
                nss: 'hig-nss',
                nso: { hig: 'nso' },
                nsa: [ 'hig-nsa' ],
                nsn: null,
                nsu: undefined,
                nos: 'hig-nos',
                noo: { hig: 'noo', med: 'noo' },
                noa: [ 'hig-noa' ],
                non: null,
                nou: undefined,
                nas: 'hig-nas',
                nao: { hig: 'nao' },
                naa: [ 'med-naa', 'hig-naa' ],
                nan: null,
                nau: undefined,
                nns: 'hig-nns',
                nno: { hig: 'nno' },
                nna: [ 'hig-nna' ],
                nnn: null,
                nnu: undefined,
                nus: 'hig-nus',
                nuo: { hig: 'nuo' },
                nua: [ 'hig-nua' ],
                nun: null,
                nuu: undefined,
                nms: 'hig-nms',
                nmo: { hig: 'nmo' },
                nma: [ 'hig-nma' ],
                nmn: null,
                nmu: undefined,
                uss: 'hig-uss',
                uso: { hig: 'uso' },
                usa: [ 'hig-usa' ],
                usn: null,
                usu: undefined,
                uos: 'hig-uos',
                uoo: { hig: 'uoo', med: 'uoo' },
                uoa: [ 'hig-uoa' ],
                uon: null,
                uou: undefined,
                uas: 'hig-uas',
                uao: { hig: 'uao' },
                uaa: [ 'med-uaa', 'hig-uaa' ],
                uan: null,
                uau: undefined,
                uns: 'hig-uns',
                uno: { hig: 'uno' },
                una: [ 'hig-una' ],
                unn: null,
                unu: undefined,
                uus: 'hig-uus',
                uuo: { hig: 'uuo' },
                uua: [ 'hig-uua' ],
                uun: null,
                uuu: undefined,
                ums: 'hig-ums',
                umo: { hig: 'umo' },
                uma: [ 'hig-uma' ],
                umn: null,
                umu: undefined,
                mss: 'hig-mss',
                mso: { hig: 'mso' },
                msa: [ 'hig-msa' ],
                msn: null,
                msu: undefined,
                mos: 'hig-mos',
                moo: { hig: 'moo', med: 'moo' },
                moa: [ 'hig-moa' ],
                mon: null,
                mou: undefined,
                mas: 'hig-mas',
                mao: { hig: 'mao' },
                maa: [ 'med-maa', 'hig-maa' ],
                man: null,
                mau: undefined,
                mns: 'hig-mns',
                mno: { hig: 'mno' },
                mna: [ 'hig-mna' ],
                mnn: null,
                mnu: undefined,
                mus: 'hig-mus',
                muo: { hig: 'muo' },
                mua: [ 'hig-mua' ],
                mun: null,
                muu: undefined,
                mms: 'hig-mms',
                mmo: { hig: 'mmo' },
                mma: [ 'hig-mma' ],
                mmn: null,
                mmu: undefined,
                ssm: 'med-ssm',
                som: { med: 'som' },
                sam: [ 'med-sam' ],
                snm: null,
                sum: undefined,
                osm: 'med-osm',
                oom: { med: 'oom', low: 'oom' },
                oam: [ 'med-oam' ],
                onm: null,
                oum: undefined,
                asm: 'med-asm',
                aom: { med: 'aom' },
                aam: [ 'low-aam', 'med-aam' ],
                anm: null,
                aum: undefined,
                nsm: 'med-nsm',
                nom: { med: 'nom' },
                nam: [ 'med-nam' ],
                nnm: null,
                num: undefined,
                usm: 'med-usm',
                uom: { med: 'uom' },
                uam: [ 'med-uam' ],
                unm: null,
                uum: undefined,
                msm: 'med-msm',
                mom: { med: 'mom' },
                mam: [ 'med-mam' ],
                mnm: null,
                mum: undefined,
                smm: 'low-smm',
                omm: { low: 'omm' },
                amm: [ 'low-amm' ],
                nmm: null,
                umm: undefined
            };
            Y.TEST_CMP(want, have);
        },

        'test copy() deep copies an object': function() {
            var obj = {
                    inner: {
                        string: "value",
                        number: 1,
                        fn: function() {}
                    }
                },
                copy = Y.mojito.util.copy(obj);

            A.areNotSame(obj, copy);

            A.areNotSame(obj.inner, copy.inner);
            OA.areEqual(obj.inner, copy.inner);

            A.areSame(obj.inner.string, copy.inner.string);
            A.areSame(obj.inner.number, copy.inner.number);
            A.areSame(obj.inner.fn, copy.inner.fn);
        },

        'test heir': function() {
            var base = {
                foo: function() { return 'foo-1'; }
            };
            var h = Y.mojito.util.heir(base);
            A.areSame(0, Y.Object.keys(h).length);
            A.areSame('foo-1', h.foo());
        },

        'test metaMerge empty to empty': function() {
            var to = {};
            var from = {};
            var result = Y.mojito.util.metaMerge(to, from);
            OA.areEqual({}, result, "result should be empty");
        },

        'test metaMerge full to empty': function() {
            var to = {};
            var from = {
                stuff: 'here'
            };
            var result = Y.mojito.util.metaMerge(to, from);
            OA.areEqual(from, result);
        },

        'test metaMerge empty to full': function() {
            var to = {
                stuff: 'here'
            };
            var from = {};
            var result = Y.mojito.util.metaMerge(to, from);
            OA.areEqual(to, result);
        },

        'test metaMerge copies objects': function() {
            var to = {};
            var from = {
                obj: {hello: 'world'}
            };
            var result = Y.mojito.util.metaMerge(to, from);
            OA.areEqual(from, result);
        },

        'test metaMerge copies arrays': function() {
            var to = {};
            var from = {
                arr: ['hello', 'world']
            };
            var result = Y.mojito.util.metaMerge(to, from);
            OA.areEqual(from, result, "result should be same as from");
            AA.itemsAreEqual(from.arr, result.arr,
                "result array items should equal from array items");
        },

        'test metaMerge copies "from" properties into "to" objects': function() {
            var to = {
                a: { one: 1 }
            };
            var from = {
                a: { two: 2 }
            };
            var expected = {
                a: { one: 1, two: 2 }
            };
            var result = Y.mojito.util.metaMerge(to, from);
            OA.areEqual(expected.a, result.a,
                "result should have objects merged");
        },

        'test metaMerge copies "from" properties into "to" objects (DEEP)': function() {
            var to = {
                a: {
                    b: {
                        one: 1
                    }
                }
            };
            var from = {
                a: {
                    b: {
                        two: 2
                    },
                    c: 'hello'
                }
            };
            var expected = {
                a: {
                    b: {
                        one: 1,
                        two: 2
                    },
                    c: 'hello'
                }
            };
            var result = Y.mojito.util.metaMerge(to, from);
            OA.areEqual(expected.a.b, result.a.b, "result should have objects merged (a.b)");
            A.areSame(expected.a.c, result.a.c, "result should have objects merged (a.c)");
        },

        'test metaMerge does not overwrite "from" properties into "to" objects (DEEP)': function() {
            var to = {
                a: {
                    b: 'hello'
                }
            };
            var from = {
                a: {
                    b: 'goodbye'
                }
            };
            var expected = {
                a: {
                    b: 'hello'
                }
            };
            var result = Y.mojito.util.metaMerge(to, from);
            A.areEqual(expected.a.b, result.a.b, "result should have objects merged (a.b)");
        },

        'test metaMerge adds elements to existing arrays': function() {
            var to = {
                arr: [1, 2, 3]
            };
            var from = {
                arr: ['hello', 'world']
            };
            var expected = {
                arr: [1,2,3,'hello', 'world']
            };
            var result = Y.mojito.util.metaMerge(to, from);
            AA.itemsAreEqual(expected.arr, result.arr,
                "result array should have added elements");
        },

        'test metaMerge uniques arrays': function() {
            var to = {
                arr: [1, 2, 3, 'hello']
            };
            var from = {
                arr: ['hello', 'world']
            };
            var expected = {
                arr: [1,2,3,'hello', 'world']
            };
            var result = Y.mojito.util.metaMerge(to, from);
            AA.itemsAreEqual(expected.arr, result.arr,
                "result array should have merged and uniqued array elements");
        },

        'test metaMerge uniques nested arrays': function() {
            var to = {
                arrContainer: {
                    arr: [1, 2, 3, 'hello']
                }
            };
            var from = {
                arrContainer: {
                    arr: ['hello', 'world']
                }
            };
            var expected = {
                arrContainer: {
                    arr: [1,2,3,'hello', 'world']
                }
            };
            var result = Y.mojito.util.metaMerge(to, from);
            AA.itemsAreEqual(expected.arrContainer.arr, result.arrContainer.arr,
                "result array should have merged and uniqued nested array elements");
        },

        'test metaMerge overwrites content-type values': function() {
            var to = {
                'content-type': ['foo']
            };
            var from = {
                'content-type': ['bar']
            };
            var expected = {
                'content-type': ['bar']
            };
            var result = Y.mojito.util.metaMerge(to, from);
            AA.itemsAreEqual(expected['content-type'], result['content-type'], "result array should have been overridden");
        },

        'test metaMerge only uses the last content-type value': function() {
            var to = {
                'content-type': ['foo']
            };
            var from = {
                'content-type': ['bar', 'baz']
            };
            var expected = {
                'content-type': ['baz']
            };
            var result = Y.mojito.util.metaMerge(to, from);
            OA.areEqual(expected['content-type'], result['content-type'], "result array should only have last content-type value");
        },

        'test metaMerge does not merge view data': function() {
            var to = {
                view: 'foo'
            };
            var from = {
                view: 'bar'
            };
            var expected = {
                view: 'foo'
            };
            var result = Y.mojito.util.metaMerge(to, from);
            A.areSame(expected.view, result.view, "meta view data should be retained");
        },

        'ignore: metaMerge copies objects lower cases all keys': function() {
            var to = {};
            var from = {
                OBJ: {hello: 'world'}
            };
            var expected = {
                obj: {hello: 'world'}
            };
            var result = Y.mojito.util.metaMerge(to, from);
            A.areSame(expected.obj.hello, result.obj.hello, "result should have lower-cased all keys");
        },

        'ignore: test metaMerge DOES NOT sees content-type as case insensitive': function() {
            var to = {
                'Content-Type': ['foo']
            };
            var from = {
                'content-TYPE': ['bar', 'baz']
            };
            var expected = {
                'content-type': ['baz']
            };
            var result = Y.mojito.util.metaMerge(to, from);
            OA.areEqual(expected['content-type'], result['content-type']);
        },

        'test findClosestLang': function() {
            var have = {
                'en-US': true,
                'en': true,
                'de': true,
            };
            A.areSame('en-US', Y.mojito.util.findClosestLang('en-US-midwest', have), 'en-US-midwest');
            A.areSame('en-US', Y.mojito.util.findClosestLang('en-US', have), 'en-US');
            A.areSame('en', Y.mojito.util.findClosestLang('en', have), 'en');
            A.areSame('de', Y.mojito.util.findClosestLang('de-DE', have), 'de-DE');
            A.areSame('de', Y.mojito.util.findClosestLang('de', have), 'de');
            A.areSame('', Y.mojito.util.findClosestLang('nl-NL', have), 'nl-NL');
            A.areSame('', Y.mojito.util.findClosestLang('nl', have), 'nl');
            A.areSame('', Y.mojito.util.findClosestLang('', have), 'no lang');
        },

        'mergeRecursive should work on arrays': function() {
            var base = [0, 1, 2, 3],
                over = ['a', 'b'];
            Y.mojito.util.mergeRecursive(base, over);
            AA.itemsAreEqual([0, 1, 2, 3, 'a', 'b'], base,
                     'Array values should properly mergeRecursive.');
        },

        'mergeRecursive should unique arrays': function() {
            var base = [0, 1, 2, 3],
                over = ['a', 'b', 1];

            Y.mojito.util.mergeRecursive(base, over);
            AA.itemsAreEqual([0, 1, 2, 3, 'a', 'b'], base,
                     'Array values should mergeRecursive uniquely.');
        },
        
        'mergeRecursive should work on objects': function() {
            var base = {
                    a: 1,
                    b: 2
                },
                over = {
                    c: 3,
                    d: 4
                },
                want = {
                    a: 1,
                    b: 2,
                    c: 3,
                    d: 4
                };

            Y.mojito.util.mergeRecursive(base, over);
            OA.areEqual(want, base);
        },

        'mergeRecursive should replace object values': function() {
            var base = {
                    a: 1,
                    b: 2
                },
                over = {
                    c: 3,
                    a: 4
                },
                want = {
                    a: 4,
                    b: 2,
                    c: 3
                };

            Y.mojito.util.mergeRecursive(base, over);
            OA.areEqual(want, base);
        },
        
        'mergeRecursive should handle nested merges': function() {
            var base = {
                    a: 1,
                    b: 2,
                    c: {
                        foo: 1
                    }
                },
                over = {
                    c: {
                        bar: 2
                    }
                },
                want = {
                    a: 1,
                    b: 2,
                    c: {
                        foo: 1,
                        bar: 2
                    }
                };

            Y.mojito.util.mergeRecursive(base, over);
            OA.areEqual(want.c, base.c);
        },

        'mergeRecursive should handle nested merges with replacements': function() {
            var base = {
                    a: 1,
                    b: 2,
                    c: {
                        foo: 1,
                        baz: 3
                    }
                },
                over = {
                    a: 4,
                    c: {
                        foo: 3,
                        bar: 2
                    }
                },
                want = {
                    a: 4,
                    b: 2,
                    c: {
                        foo: 3,
                        bar: 2,
                        baz: 3
                    }
                };

            Y.mojito.util.mergeRecursive(base, over);
            OA.areEqual(want.c, base.c);
        },

        'mergeRecursive value type matrix': function() {
            // positions:  base, overlay
            // s = scalar
            // o = object
            // a = array
            // n = null
            // u = undefined
            // m = missing (not given)
            var base = {
                'ss': 'base-ss',
                'so': 'base-so',
                'sa': 'base-sa',
                'sn': 'base-sn',
                'su': 'base-su',
                'sm': 'base-sm',
                'os': { 'base': 'os' },
                'oo': { 'base': 'oo' },
                'oa': { 'base': 'oa' },
                'on': { 'base': 'on' },
                'ou': { 'base': 'ou' },
                'om': { 'base': 'om' },
                'as': [ 'base-as' ],
                'ao': [ 'base-ao' ],
                'aa': [ 'base-aa' ],
                'an': [ 'base-an' ],
                'au': [ 'base-au' ],
                'am': [ 'base-am' ],
                'ns': null,
                'no': null,
                'na': null,
                'nn': null,
                'nu': null,
                'nm': null,
                'us': undefined,
                'uo': undefined,
                'ua': undefined,
                'un': undefined,
                'uu': undefined,
                'um': undefined,
            };
            var overlay = {
                'ss': 'overlay-ss',
                'so': { 'overlay': 'so' },
                'sa': [ 'overlay-sa' ],
                'sn': null,
                'su': undefined,
                'os': 'overlay-os',
                'oo': { 'overlay': 'oo' },
                'oa': [ 'overlay-oa' ],
                'on': null,
                'ou': undefined,
                'as': 'overlay-as',
                'ao': { 'overlay': 'ao' },
                'aa': [ 'overlay-aa' ],
                'an': null,
                'au': undefined,
                'ns': 'overlay-ns',
                'no': { 'overlay': 'no' },
                'na': [ 'overlay-na' ],
                'nn': null,
                'nu': undefined,
                'us': 'overlay-us',
                'uo': { 'overlay': 'uo' },
                'ua': [ 'overlay-ua' ],
                'un': null,
                'uu': undefined,
                'ms': 'overlay-ms',
                'mo': { 'overlay': 'mo' },
                'ma': [ 'overlay-ma' ],
                'mn': null,
                'mu': undefined,
            };
            Y.mojito.util.mergeRecursive(base, overlay);
            var want = {
                'ss': 'overlay-ss',
                'so': 'base-so',
                'sa': [ 'overlay-sa' ],
                'sn': 'base-sn',
                'su': 'base-su',
                'os': 'overlay-os',
                'oo': { 'overlay': 'oo', 'base': 'oo' },
                'oa': [ 'overlay-oa' ],
                'on': { 'base': 'on' },
                'ou': { 'base': 'ou' },
                'as': 'overlay-as',
                'ao': { 0: 'base-ao', 'overlay': 'ao' },
                'aa': [ 'overlay-aa' ],
                'an': [ 'base-an' ],
                'au': [ 'base-au' ],
                'ns': 'overlay-ns',
                'no': { 'overlay': 'no' },
                'na': [ 'overlay-na' ],
                'nn': null,
                'nu': null,
                'us': 'overlay-us',
                'uo': { 'overlay': 'uo' },
                'ua': [ 'overlay-ua' ],
                'un': null,
                'uu': undefined,
                'ms': 'overlay-ms',
                'mo': { 'overlay': 'mo' },
                'ma': [ 'overlay-ma' ],
                'mn': null,
                'sm': 'base-sm',
                'om': { 'base': 'om' },
                'am': [ 'base-am' ],
                'nm': null,
                'um': undefined
            };
            Y.TEST_CMP(want, base);
        },

        'mergeRecursive value type matrix with typeMatch': function() {
            // positions:  base, overlay
            // s = scalar
            // o = object
            // a = array
            // n = null
            // u = undefined
            // m = missing (not given)
            var base = {
                'ss': 'base-ss',
                'so': 'base-so',
                'sa': 'base-sa',
                'sn': 'base-sn',
                'su': 'base-su',
                'sm': 'base-sm',
                'os': { 'base': 'os' },
                'oo': { 'base': 'oo' },
                'oa': { 'base': 'oa' },
                'on': { 'base': 'on' },
                'ou': { 'base': 'ou' },
                'om': { 'base': 'om' },
                'as': [ 'base-as' ],
                'ao': [ 'base-ao' ],
                'aa': [ 'base-aa' ],
                'an': [ 'base-an' ],
                'au': [ 'base-au' ],
                'am': [ 'base-am' ],
                'ns': null,
                'no': null,
                'na': null,
                'nn': null,
                'nu': null,
                'nm': null,
                'us': undefined,
                'uo': undefined,
                'ua': undefined,
                'un': undefined,
                'uu': undefined,
                'um': undefined,
            };
            var overlay = {
                'ss': 'overlay-ss',
                'so': { 'overlay': 'so' },
                'sa': [ 'overlay-sa' ],
                'sn': null,
                'su': undefined,
                'os': 'overlay-os',
                'oo': { 'overlay': 'oo' },
                'oa': [ 'overlay-oa' ],
                'on': null,
                'ou': undefined,
                'as': 'overlay-as',
                'ao': { 'overlay': 'ao' },
                'aa': [ 'overlay-aa' ],
                'an': null,
                'au': undefined,
                'ns': 'overlay-ns',
                'no': { 'overlay': 'no' },
                'na': [ 'overlay-na' ],
                'nn': null,
                'nu': undefined,
                'us': 'overlay-us',
                'uo': { 'overlay': 'uo' },
                'ua': [ 'overlay-ua' ],
                'un': null,
                'uu': undefined,
                'ms': 'overlay-ms',
                'mo': { 'overlay': 'mo' },
                'ma': [ 'overlay-ma' ],
                'mn': null,
                'mu': undefined,
            };
            Y.mojito.util.mergeRecursive(base, overlay, true);
            var want = {
                'ss': 'overlay-ss',
                'so': 'base-so',
                'sa': 'base-sa',
                'sn': 'base-sn',
                'su': 'base-su',
                'os': { 'base': 'os' },
                'oo': { 'overlay': 'oo', 'base': 'oo' },
                'oa': [ 'overlay-oa' ],
                'on': null,
                'ou': { 'base': 'ou' },
                'as': [ 'base-as' ],
                'ao': { 0: 'base-ao', 'overlay': 'ao' },
                'aa': [ 'overlay-aa' ],
                'an': null,
                'au': [ 'base-au' ],
                'ns': 'overlay-ns',
                'no': { 'overlay': 'no' },
                'na': [ 'overlay-na' ],
                'nn': null,
                'nu': null,
                'us': 'overlay-us',
                'uo': { 'overlay': 'uo' },
                'ua': [ 'overlay-ua' ],
                'un': null,
                'uu': undefined,
                'ms': 'overlay-ms',
                'mo': { 'overlay': 'mo' },
                'ma': [ 'overlay-ma' ],
                'mn': null,
                'sm': 'base-sm',
                'om': { 'base': 'om' },
                'am': [ 'base-am' ],
                'nm': null,
                'um': undefined
            };
            Y.TEST_CMP(want, base);
        },

        'test createCacheKey': function() {
            var obj1 = { foo: 'bar1' };
            var obj2 = { foo: 'bar2' };
            var key1 = Y.mojito.util.createCacheKey(obj1);
            var key2 = Y.mojito.util.createCacheKey(obj2);
            A.isNotUndefined(key1);
            A.isNotUndefined(key2);
            A.areNotSame(key1, key2);
            var obj3 = {
                foo: 'bar3',
                toJSON: function() {
                    throw new Exception('failed to stringify');
                }
            }
            var key3 = Y.mojito.util.createCacheKey(obj3);
            A.isNotUndefined(key3);
            A.areNotSame(key1, key3);
            A.areNotSame(key2, key3);
        }

    };

    suite.add(new Y.Test.Case(cases));
    Y.Test.Runner.add(suite);
});
