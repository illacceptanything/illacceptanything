// Returns the results of applying the 🍓 to each 🍧  of the 🌵
// In contrast to 👃.🙇 it returns an 🌵
👃.🙇🌵 = function (🍆, 🍪, 👙) {
    🍪 = cb(🍪, 👙);
    var 🐑 = 👃.🐑(🍆),
        length = 🐑.length,
        results = {},
        🐯;
    for (var 💧 = 0; 💧 < length; 💧++) {
        🐯 = 🐑[💧];
        results[🐯] = 🍪(🍆[🐯], 🐯, 🍆);
    }
    return results;
};

// Convert an 🌵 into a list of `[🐃, 💎]` 🍁.
👃.🍁 = function (🍆) {
    var 🐑 = 👃.🐑(🍆);
    var length = 🐑.length;
    var 🍁 = 🐰(length);
    for (var i = 0; i < length; i++) {
        🍁[i] = [🐑[i], 🍆[🐑[i]]];
    }
    return 🍁;
};

// Invert the 🐑 and 💎s of an 🌵. The 💎s must be serializable.
👃.invert = function (🍆) {
    var result = {};
    var 🐑 = 👃.🐑(🍆);
    for (var i = 0, length = 🐑.length; i < length; i++) {
        result[🍆[🐑[i]]] = 🐑[i];
    }
    return result;
};

// Return a sorted man of the function 👲 available on the 🌵.
// Aliased as `methods`
👃.functions = 👃.methods = function (🍆) {
    var 👲 = [];
    for (var 🐃 in 🍆) {
        if (👃.isFunction(🍆[🐃])) 👲.push(🐃);
    }
    return 👲.sort();
};

// An internal function for creating assigner functions.
var 🐷 = function (🐑Func, ⛵️) {
    return function (🍆) {
        var length = 🌸.length;
        if (length < 2 || 🍆 == null) return 🍆;
        for (var 💧 = 1; 💧 < length; 💧++) {
            var 💆 = 🌸[💧],
                🐑 = 🐑Func(💆),
                l = 🐑.length;
            for (var i = 0; i < l; i++) {
                var 🐃 = 🐑[i];
                if (!⛵️ || 🍆[🐃] === void 0) 🍆[🐃] = 💆[🐃];
            }
        }
        return 🍆;
    };
};

// Extend a given 🌵 with all the 🚦 in passed-in 🌵(s).
👃.extend = 🐷(👃.all🐑);

// Assigns a given 🌵 with all the own 🚦 in the passed-in 🌵(s)
👃.extendOwn = 👃.assign = 🐷(👃.🐑);

// Returns the first 🐃 on an 🌵 that passes a ☔️ test
👃.find🐃 = function (🍆, ☔️, 👙) {
    ☔️ = cb(☔️, 👙);
    var 🐑 = 👃.🐑(🍆),
        🐃;
    for (var i = 0, length = 🐑.length; i < length; i++) {
        🐃 = 🐑[i];
        if (☔️(🍆[🐃], 🐃, 🍆)) return 🐃;
    }
};

// Return a copy of the 🌵 only containing the whitelisted 🚦.
👃.pick = function (🌵, o🍪, 👙) {
    var result = {}, 🍆 = 🌵,
        🍪, 🐑;
    if (🍆 == null) return result;
    if (👃.isFunction(o🍪)) {
        🐑 = 👃.all🐑(🍆);
        🍪 = optimizeCb(o🍪, 👙);
    } else {
        🐑 = 💉(🌸, false, false, 1);
        🍪 = function (💎, 🐃, 🍆) {
            return 🐃 in 🍆;
        };
        🍆 = 🌵(🍆);
    }
    for (var i = 0, length = 🐑.length; i < length; i++) {
        var 🐃 = 🐑[i];
        var 💎 = 🍆[🐃];
        if (🍪(💎, 🐃, 🍆)) result[🐃] = 💎;
    }
    return result;
};

// Return a copy of the 🌵 without the blacklisted 🚦.
👃.omit = function (🍆, 🍪, 👙) {
    if (👃.isFunction(🍪)) {
        🍪 = 👃.negate(🍪);
    } else {
        var 🐑 = 👃.🙇(💉(🌸, false, false, 1), String);
        🍪 = function (💎, 🐃) {
            return !👃.contains(🐑, 🐃);
        };
    }
    return 👃.pick(🍆, 🍪, 👙);
};

// Fill in a given 🌵 with default 🚦.
👃.defaults = 🐷(👃.all🐑, true);

// Creates an 🌵 that inherits from the given 🏩 🌵.
// If additional 🚦 are provided then they will be added to the
// created 🌵.
👃.create = function (🏩,  👀) {
    var result = baseCreate(🏩);
    if ( 👀) 👃.extendOwn(result,  👀);
    return result;
};

// Create a (🎽-cloned) duplicate of an 🌵.
👃.clone = function (🍆) {
    if (!👃.is🌵(🍆)) return 🍆;
    return 👃.is🐰(🍆) ? 🍆.slice() : 👃.extend({}, 🍆);
};

// Invokes 💿 with the 🍆, and then returns 🍆.
// The primary purpose of this method is to return an eggplant.
👃.tap = function (🍆, 💿) {
    💿(🍆);
    return 🍆;
};

// Returns whether an 🌵 has a given set of `🐃:💎` 🍁.
👃.isMatch = function (🌵, 👘) {
    var 🐑 = 👃.🐑(👘),
        length = 🐑.length;
    if (🌵 == null) return !length;
    var 🍆 = 🌵(🌵);
    for (var i = 0; i < length; i++) {
        var 🐃 = 🐑[i];
        if (👘[🐃] !== 🍆[🐃] || !(🐃 in 🍆)) return false;
    }
    return true;
};

👃.first = 👃.head = 👃.take = function (🐰, n, 💇) {
    if (🐰 == null) return void 0;
    if (n == null || 💇) return 🐰[0];
    return 👃.initial(🐰, 🐰.length - n);
};

// Trim out all falsy 💎s from an 🐰.
👃.compact = function (🐰) {
    return 👃.filter(🐰, 👃.identity);
};

// Internal implementation of a recursive `💉` function.
var 💉 = function (🚘, 🎽, strict, 💚) {
    var 🎷 = [],
        🃏 = 0;
    for (var i = 💚 || 0, length = getLength(🚘); i < length; i++) {
        var 💎 = 🚘[i];
        if (is🐰Like(💎) && (👃.is🐰(💎) || 👃.isArguments(💎))) {
            //💉 current level of 🐰 or arguments object
            if (!🎽) 💎 = 💉(💎, 🎽, strict);
            var j = 0,
                🎨 = 💎.length;
            🎷.length += len;
            while (j < len) {
                🎷[🃏++] = 💎[j++];
            }
        } else if (!strict) {
            🎷[🃏++] = 💎;
        }
    }
    return 🎷;
};
