var _ = require('lodash');
var DepGraph = require('dependency-graph').DepGraph;

/**
 * @name  sortByDependency
 * @description                         Sort a collection of items, such that the items come before
 *                                      or after the dependencies defined on the items.
 * @param  {Array|Object}  items        The collection of items to sort.
 * @param  {string} [afterProp]         The name of the property that will hold an array of names of
 *                                      other items that the item must come after. If it is not
 *                                      defined then this property is ignored.
 * @param  {string} [beforeProp]        The name of the property that will hold an array of names of
 *                                      other items that the item must come before. If it is not
 *                                      defined then this property is ignored.
 * @param  {string} [nameProp='name']   The name of the property on the object that holds its name,
 *                                      defaults to 'name'.
 * @return {Array}                      A new array containing the sorted collection of items.
 */
module.exports = function sortByDependency(items, afterProp, beforeProp, nameProp) {

  nameProp = nameProp || 'name';

  var map = {};
  var depGraph = new DepGraph();

  var addDependencies = function(item, dependencyProp, addBefore) {
    if ( dependencyProp && item[dependencyProp]) {
      if ( !Array.isArray(item[dependencyProp]) ) {
        throw new Error('Error in item "' + item[nameProp] + '" - ' + dependencyProp + ' must be an array');
      }
      item[dependencyProp].forEach(function(dependency) {
        if ( !map[dependency] ) {
          throw new Error('Missing dependency: "' + dependency + '"  on "' + item[nameProp] + '"');
        }
        if ( addBefore ) {
          depGraph.addDependency(dependency, item[nameProp]);
        } else {
          depGraph.addDependency(item[nameProp], dependency);
        }
      });
    }
  };

  _.forEach(items, function(item, index) {
    if ( !item[nameProp] ) {
      throw new Error('Missing ' + nameProp + ' property on item #' + (index+1));
    }
    map[item[nameProp]] = item;
    depGraph.addNode(item[nameProp]);
  });


  _.forEach(items, function(item) {
    addDependencies(item, afterProp);
    addDependencies(item, beforeProp, true);
  });

  return depGraph.overallOrder().map(function(itemName) {
    return map[itemName];
  });
};
