/*
 * Copyright (c) 2012, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint
    anon:true, sloppy:true, regexp:true, continue:true, nomen:true, node:true, stupid:true
*/


var libpath = require('path'),
    libfs = require('fs'),
    existsSync = libfs.existsSync || libpath.existsSync;

function copy(x) {
    if (!x) {
        return x;
    }
    return JSON.parse(JSON.stringify(x));
}


/*
 * walks a tree of NPM packages breadth first
 *
 * @constructor
 * @param root {string} directory to walk
 */
function BreadthFirstPackageWalker(root) {
    this.root = root;
}


/*
 * do the actual walking
 *
 * The "cb" argument takes two parameters.  The first is an error if something
 * when wrong.  (Otherwise it is null.)
 *
 * The second argument is information about the package.  It is an object
 * containing the following:
 *      dir {string} directory of package
 *      pkg {object} contents of the package's package.json
 *      depth {interger} how deeply nested the package is
 *      parents {array} list of ancestor package names
 *
 * @param cb {function(error, info)} callback called for each package
 * @return {nothing} results returned via callback
 */
BreadthFirstPackageWalker.prototype.walk = function(cb) {
    var work;
    this.todo = [];
    if ('node_modules' === libpath.basename(this.root)) {
        this._walkModules({
            depth: 0,
            parents: [],
            dir: libpath.dirname(this.root),
            pkg: {
                name: 'node_modules'
            }
        });
    } else {
        this.todo.push({
            depth: 0,
            parents: [],
            dir: this.root
        });
    }
    // jslint is overly particular about simple things
    work = this.todo.shift();
    while (work) {
        this._walkPackage(work, cb);
        work = this.todo.shift();
    }
};


/*
 * walk a package
 * @param work {object} see "info" argument to callback of walk()
 * @param cb {function(error, info)} see walk()
 * @return {nothing} results returned via callback
 */
BreadthFirstPackageWalker.prototype._walkPackage = function(work, cb) {
    //console.log('--------------------------------------- WALK PACKAGE');
    var packagePath, pkg;
    packagePath = libpath.join(work.dir, 'package.json');
    if (existsSync(packagePath)) {
        try {
            pkg = libfs.readFileSync(packagePath, 'utf-8');
            pkg = JSON.parse(pkg);
            work.pkg = pkg;
        } catch (e) {
            cb(e);
            return;
        }
    } else {
        pkg = {
            name: libpath.basename(work.dir),
            version: '999.999.999'
        };
        work.pkg = pkg;
    }
    if (cb(null, work) !== false) {
        this._walkModules(work);
    }
};


/*
 * walk the node_modules/ of a directory
 * @param work {object} see "info" argument to callback of walk()
 * @param cb {function(error, info)} see walk()
 * @return {nothing} results returned via callback
 */
BreadthFirstPackageWalker.prototype._walkModules = function(work) {
    var modulesDir, i, subdir, subdirs, parents;
    modulesDir = libpath.join(work.dir, 'node_modules');
    //console.log('--------------------------------------- WALK MODULES -- ' + modulesDir);
    //console.log(work);
    parents = work.parents.slice(0);
    parents.unshift(work.pkg.name);
    try {
        subdirs = libfs.readdirSync(modulesDir);
    } catch (e) {
        return;
    }
    subdirs.sort();
    for (i = 0; i < subdirs.length; i += 1) {
        subdir = subdirs[i];
        if ('.' === subdir.charAt(0)) {
            continue;
        }
        this.todo.push({
            depth: work.depth + 1,
            parents: parents,
            dir: libpath.join(modulesDir, subdir)
        });
    }
};


module.exports = {
    BreadthFirst: BreadthFirstPackageWalker
};


