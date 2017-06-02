/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true, newcap:true*/
/*global YUI*/


YUI.add('mojito-util', function(Y, NAME) {

    var META_AUTOCLOBBER = ['content-type'],
        META_EXCLUDE = ['view'],
        META_ATOMIC = ['content-type'];


    function arrayContainsLowerCase(a, obj) {
        var i = a.length,
            selector = obj.toLowerCase();

        i -= 1;
        while (i >= 0) {
            if (a[i].toLowerCase() === selector) {
                return true;
            }
            i -= 1;
        }
        return false;
    }


    function shouldAutoClobber(k) {
        return arrayContainsLowerCase(META_AUTOCLOBBER, k);
    }


    function isExcluded(k) {
        return arrayContainsLowerCase(META_EXCLUDE, k);
    }


    function isAtomic(k) {
        return arrayContainsLowerCase(META_ATOMIC, k);
    }


    Y.namespace('mojito').util = {
        copy: function(oldObj) {
            var newObj,
                key,
                len,
                type,
                copy = Y.mojito.util.copy;

            if (!oldObj || typeof oldObj !== 'object') {
                return oldObj;
            }

            type = Y.Lang.type(oldObj);

            if (type === 'regexp' || type === 'function') {
                return oldObj;
            }

            if (type === "array") {
                newObj = [];
                len = oldObj.length;
                for (key = 0; key < len; key += 1) {
                    newObj[key] = copy(oldObj[key]);
                }
                return newObj;
            }

            newObj = {};
            for (key in oldObj) {
                if (oldObj.hasOwnProperty(key)) {
                    newObj[key] = copy(oldObj[key]);
                }
            }
            return newObj;
        },

        extend: function (r, s, px, sx) {
            var o;
            if (r && typeof r === 'object') {
                o = r;
                r = function () {};
                r.prototype = o;
            }
            if (s && typeof s === 'object') {
                o = s;
                s = function () {};
                s.prototype = o;
            }

            return Y.extend(r, s, px, sx);
        },


        heir: function(o) {
            if (typeof o === 'function') {
                return new o();
            }
            function F() {}
            F.prototype = o;
            return new F();
        },


        /**
         * Blends a source base object and an overlay object to create a new
         * object containing the recursively merged values of the two. This is
         * similar to performing a copy and then a mergeRecursive operation but
         * is intended to be more performant by avoiding duplicate work.
         *
         * @method blend
         * @param {Object} base The base object. Properties in this object may
         * be overwritten by those in overlay.
         * @param {Object} extension The "overlay" object. Properties in this
         * object are always found in the result.
         * @return {Object} A new object containing the blended values.
         */
        blend: function(base, overlay) {

            var dest,
                i,
                slot,
                copy = Y.mojito.util.copy,
                blend = Y.mojito.util.blend;

            // Protect from bad input data.
            if (base === null || base === undefined) {
                return copy(overlay);
            }
            if (overlay === null || overlay === undefined) {
                return copy(base);
            }

            if (Y.Lang.isArray(overlay)) {
                if (Y.Lang.isArray(base)) {
                    return Y.Array.unique(base.concat(overlay));
                }
                Y.log('Type mismatch for array merge. Dropping value from base: ' + JSON.stringify(base), 'warn', NAME);
                return overlay.slice(0);
            }

            // Otherwise treat everything as an object.
            dest = {};
            for (i in overlay) {
                if (overlay.hasOwnProperty(i)) {
                    if (base.hasOwnProperty(i)) {
                        // "Conflicting" property. A recursive merge of the
                        // value is required if it's a complex object.
                        slot = overlay[i];
                        if (Y.Lang.isObject(slot)) {
                            // Complex, not null.
                            dest[i] = blend(base[i], slot);
                        } else {
                            // Simple value or null. Overlay value wins.
                            dest[i] = slot;
                        }
                    } else {
                        // Unique to the overlay object, no merge needed.
                        dest[i] = copy(overlay[i]);
                    }
                }
            }

            // Verify we didn't miss properties in the base. Anything
            // missing from the destination object should be copied.
            // Anything already there would have been merged in the previous
            // loop.
            if (Y.Lang.isObject(base) && !Y.Lang.isArray(base)) {
                for (i in base) {
                    if (base.hasOwnProperty(i)) {
                        if (!dest.hasOwnProperty(i)) {
                            dest[i] = copy(base[i]);
                        }
                    }
                }
            }

            return dest;
        },


        /**
         * Blends three objects to create a new object containing the
         * recursively merged values of the them all. This is similar to
         * performing copy()s and then mergeRecursive()s but is intended
         * to be more performant by avoiding duplicate work.
         *
         * The type of the "highest" parameter always dictates type of results.
         *
         * @method blend3
         * @param {Object} lowest The lowest priority object.
         * Properties in this object may be overwritten by those in medium and highest.
         * @param {Object} medium The medium priority object.
         * Properties in this object may be overwritten by those in highest.
         * @param {Object} highest The highest priority object.
         * Properties in this object are always found in the results.
         * @return {Object} A new object containing the blended values.
         */
        blend3: function(lowest, medium, highest) {
            var dest,
                key,
                val,
                useMedium,
                useLowest,
                copy = Y.mojito.util.copy,
                blend2 = Y.mojito.util.blend,
                blend3 = Y.mojito.util.blend3;

            // Protect from bad input data.
            if (highest === null || highest === undefined) {
                return blend2(lowest, medium);
            }

            if (Y.Lang.isArray(highest)) {
                if (Y.Lang.isArray(medium)) {
                    if (Y.Lang.isArray(lowest)) {
                        dest = lowest.concat(medium, highest);
                    } else {
                        dest = medium.concat(highest);
                    }
                } else {
                    dest = highest.slice(0);
                }
                return Y.Array.unique(dest);
            }

            // Otherwise treat everything as an object.
            dest = {};
            useMedium = true;
            useLowest = true;
            if ((!Y.Lang.isObject(medium)) || Y.Lang.isArray(medium)) {
                useMedium = false;
                useLowest = false;
            }
            if (useLowest && ((!Y.Lang.isObject(lowest)) || Y.Lang.isArray(lowest))) {
                useLowest = false;
            }
            for (key in highest) {
                if (highest.hasOwnProperty(key)) {
                    val = highest[key];
                    if (Y.Lang.isObject(val)) {
                        if (useMedium && medium.hasOwnProperty(key)) {
                            if (useLowest && lowest.hasOwnProperty(key)) {
                                dest[key] = blend3(lowest[key], medium[key], val);
                            } else {
                                dest[key] = blend2(medium[key], val);
                            }
                        } else {
                            if (useLowest && lowest.hasOwnProperty(key)) {
                                dest[key] = blend2(lowest[key], val);
                            } else {
                                dest[key] = copy(val);
                            }
                        }
                    } else {
                        dest[key] = copy(val);
                    }
                }
            }

            // Process keys that are in medium but not in highest.
            if (useMedium) {
                for (key in medium) {
                    if (medium.hasOwnProperty(key)) {
                        if (!dest.hasOwnProperty(key)) {
                            val = medium[key];
                            if (Y.Lang.isObject(val)) {
                                if (useLowest && lowest.hasOwnProperty(key)) {
                                    dest[key] = blend2(lowest[key], val);
                                } else {
                                    dest[key] = copy(val);
                                }
                            } else {
                                // scalar
                                dest[key] = val;
                            }
                        }
                    }
                }
            }

            // Process keys that are in lowest but not highest nor in medium.
            if (useLowest) {
                for (key in lowest) {
                    if (lowest.hasOwnProperty(key)) {
                        if (!dest.hasOwnProperty(key)) {
                            dest[key] = copy(lowest[key]);
                        }
                    }
                }
            }

            return dest;
        },


        /**
         * Recursively merge properties of two objects
         * @method mergeRecursive
         * @param {object} dest The destination object.
         * @param {object} src The source object.
         * @param {boolean} typeMatch Only replace if src and dest types are
         *     the same type if true.
         */
        mergeRecursive: function(dest, src, typeMatch) {
            var p,
                arr;

            if (Y.Lang.isArray(src)) {
                if (!Y.Lang.isArray(dest)) {
                    throw new Error('Type mismatch for object merge.');
                }

                // Not particularly performant, but we need to avoid duplicates
                // as much as possible. Unfortunately, the YUI unique calls
                // don't have an option to work in-place so we have even more
                // overhead here :(.

                // copy destination array.
                arr = dest.slice(0);

                // unroll src elements into our copy via apply.
                arr.push.apply(arr, src);

                // unique the src and destination items.
                arr = Y.Array.unique(arr);

                // truncate destination and unroll uniqued elements into it.
                dest.length = 0;
                dest.push.apply(dest, arr);
            } else {
                for (p in src) {
                    if (src.hasOwnProperty(p)) {
                        // Property in destination object set; update its value.
                        // TODO: lousy test. Constructor matches don't always work.
                        if (src[p] && src[p].constructor === Object) {
                            if (!dest[p]) {
                                dest[p] = {};
                            }
                            dest[p] = this.mergeRecursive(dest[p], src[p]);
                        } else {
                            if (dest[p] && typeMatch) {
                                if (typeof dest[p] === typeof src[p]) {
                                    dest[p] = src[p];
                                }
                            } else if (typeof src[p] !== 'undefined') {
                                // only copy values that are not undefined, null and
                                // falsey values should be copied
                                // for null sources, we only want to copy over
                                // values that are undefined
                                if (src[p] === null) {
                                    if (typeof dest[p] === 'undefined') {
                                        dest[p] = src[p];
                                    }
                                } else {
                                    dest[p] = src[p];
                                }
                            }
                        }
                    }
                }
            }
            return dest;
        },


        /**
         * Used to merge meta objects into each other. Special consideration for
         * certain headers values like 'content-type'.
         * @method metaMerge
         * @private
         * @param {object} to The target object.
         * @param {object} from The source object.
         * @param {boolean} clobber True to overwrite existing properties.
         */
        metaMerge: function(to, from, clobber, __internal) {
            var k,
                tv,
                fv,
                internal = __internal;

            for (k in from) {
                if (from.hasOwnProperty(k)) {
                    if (internal || !isExcluded(k)) {
                        fv = from[k];
                        tv = to[k];
                        if (!tv) {
                            // Y.log('adding ' + k);
                            to[k] = fv;
                        } else if (Y.Lang.isArray(fv)) {
                            // Y.log('from array ' + k);
                            if (!Y.Lang.isArray(tv)) {
                                throw new Error('Meta merge error.' +
                                    ' Type mismatch between mojit metas.');
                            }
                            // Largely used for content-type, but could be other
                            // key values in the future.
                            if (shouldAutoClobber(k)) {
                                if (isAtomic(k)) {
                                    // Note the choice to use the last item of
                                    // the inbound array, not the first.
                                    // Y.log('atomizing ' + k);
                                    to[k] = [fv[fv.length - 1]];
                                } else {
                                    // Not "atomic" but clobbering means we'll
                                    // completely replace any existing array
                                    // value in the slot.
                                    // Y.log('clobbering ' + k);
                                    to[k] = fv;
                                }
                            } else {
                                // A simple push() here would work, but it
                                // doesn't unique the values so it may cause an
                                // array to grow without bounds over time even
                                // when no truly new content is added.
                                Y.mojito.util.mergeRecursive(tv, fv);
                            }
                        } else if (Y.Lang.isObject(fv)) {
                            // Y.log('from object ' + k);
                            if (Y.Lang.isObject(tv)) {
                                // Y.log('merging ' + k);
                                to[k] = Y.mojito.util.metaMerge(tv, fv, clobber,
                                    true);
                            } else if (Y.Lang.isNull(tv) ||
                                    Y.Lang.isUndefined(tv)) {
                                to[k] = fv;
                            } else {
                                throw new Error('Meta merge error.' +
                                    ' Type mismatch between mojit metas.');
                            }
                        } else if (clobber) {
                            // Y.log('clobbering ' + k);
                            to[k] = fv;
                        }
                    }
                }
            }
            return to;
        },


        /*
         * TODO: [Issue 79] I'm sure we can do this better.
         *
         * This function trys to make the given URL relative to the
         * folder the iOS UIWebView is running in.
         */
        iOSUrl: function(url) {

            // If we are not in a DOM, return
            if (typeof window === 'undefined') {
                return url;
            }

            // Now we do some bad stuff for iOS
            // Basically if we are in a UIWebView and its location is a
            // file:// on the device we have to make our URL relative to the
            // file that was opened
            if (window.location.href.indexOf('file://') === 0 &&
                    window.location.href.indexOf('/Applications/') > 0 &&
                    window.location.href.indexOf('.app/') > 0) {
                if (url.charAt(0) === '/') {
                    url = url.slice(1);
                }
            }
            return url;
        },

        /**
         * Given an object, generate a key that can be used for cache lookup
         *
         * @method createKey
         * @param obj {Object}
         * @return {String}
         */
        createCacheKey: function(obj) {
            var key;
            // TODO: should check if obj is of type Object vs try/catch
            try {
                key = Y.JSON.stringify(obj);
            } catch (err) {
                key = Y.guid();
            }
            return key;
        },

        /**
         * @method findClosestLang
         * @param {string} want the desired language code
         * @param {object} have an object whose keys are available language codes and whose values are true (for all keys that exist)
         * @return {string} closest matching language code, or an empty string if none match
         */
        findClosestLang: function(want, have) {
            var p,
                parts,
                test;

            if (want) {
                parts = want.split('-');
                for (p = want.length; p > 0; p -= 1) {
                    test = parts.slice(0, p).join('-');
                    if (have[test]) {
                        return test;
                    }
                }
            }
            return '';
        },

        /**
         * Abstract the logic on creating a route name based on
         * the `verb` and `call` used by the Mojito router.
         *
         * @method createRouteName
         * @public
         * @param {String} verb
         * @param {String} call
         * @return {String} key
         */
        encodeRouteName: function(verb, call) {
            var args = [].slice.call(arguments);
            return args.join('#');
        }
    };

}, '0.1.0', {requires: [
    'oop',
    'array-extras',
    'json-stringify',
    'mojito'
]});
