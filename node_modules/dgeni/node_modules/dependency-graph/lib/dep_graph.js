/**
 * A simple dependency graph
 */
var _ = require('underscore');

/**
 * Helper for creating a Depth-First-Search on
 * a set of edges.
 *
 * Detects cycles and throws an Error if one is detected
 *
 * @param edges The set of edges to DFS through
 * @param leavesOnly Whether to only return "leaf" nodes (ones who have no edges)
 * @param result An array in which the results will be populated
 */
function createDFS(edges, leavesOnly, result) {
  var chain = {};
  var visited = {};
  return function DFS(name) {
    visited[name] = true;
    chain[name] = true;
    edges[name].forEach(function (edgeName) {
      if (!visited[edgeName]) {
        DFS(edgeName);
      } else if (chain[edgeName]) {
        throw new Error('Dependency Cycle Found: ' + edgeName);
      }
    });
    chain[name] = false;
    if ((!leavesOnly || edges[name].length === 0) && result.indexOf(name) === -1) {
      result.push(name);
    }
  };
}

/**
 * Simple Dependency Graph
 */
var DepGraph = exports.DepGraph = function DepGraph() {
  this.nodes = {};
  this.outgoingEdges = {}; // Node name -> [Dependency Node name]
  this.incomingEdges = {}; // Node name -> [Dependant Node name]
};
DepGraph.prototype = {
  addNode:function (name) {
    this.nodes[name] = name;
    this.outgoingEdges[name] = [];
    this.incomingEdges[name] = [];
  },
  removeNode:function (name) {
    delete this.nodes[name];
    delete this.outgoingEdges[name];
    delete this.incomingEdges[name];
    _.each(this.incomingEdges, function (edges) {
      var idx = edges.indexOf(name);
      if (idx >= 0) {
        edges.splice(idx, 1);
      }
    });
  },
  hasNode:function (name) {
    return !!this.nodes[name];
  },
  addDependency:function (from, to) {
    if (this.hasNode(from) && this.hasNode(to)) {
      if (this.outgoingEdges[from].indexOf(to) === -1) {
        this.outgoingEdges[from].push(to);
      }
      if (this.incomingEdges[to].indexOf(from) === -1) {
        this.incomingEdges[to].push(from);
      }
      return true;
    } else {
      throw new Error('One of the nodes does not exist: ' + from + ', ' + to);
    }
  },
  removeDependency:function (from, to) {
    var idx = this.outgoingEdges[from].indexOf(to);
    if (idx >= 0) {
      this.outgoingEdges[from].splice(idx, 1);
    }
    idx = this.incomingEdges[to].indexOf(from);
    if (idx >= 0) {
      this.incomingEdges[to].splice(idx, 1);
    }
  },
  dependenciesOf:function (name, leavesOnly) {
    if (this.nodes[name]) {
      var result = [];
      var DFS = createDFS(this.outgoingEdges, leavesOnly, result);
      DFS(name);
      var idx = result.indexOf(name);
      if (idx >= 0) {
        result.splice(idx, 1);
      }
      return result;
    }
    else {
      throw new Error('Node does not exist: ' + name);
    }
  },
  dependantsOf:function (name, leavesOnly) {
    if (this.nodes[name]) {
      var result = [];
      var DFS = createDFS(this.incomingEdges, leavesOnly, result);
      DFS(name);
      var idx = result.indexOf(name);
      if (idx >= 0) {
        result.splice(idx, 1);
      }
      return result;
    } else {
      throw new Error('Node does not exist: ' + name);
    }
  },
  overallOrder:function (leavesOnly) {
    var self = this;
    var result = [];
    var DFS = createDFS(this.outgoingEdges, leavesOnly, result);
    _.each(_.filter(_.keys(this.nodes), function (node) {
      return self.incomingEdges[node].length === 0;
    }), function (n) {
      DFS(n);
    });
    if (_.size(this.nodes) > 0 && result.length === 0) {
      // Special case when there are no nodes with no dependants
      throw new Error('Dependency Cycle Found');
    }
    return result;
  }
};