# Dependency Graph

Simple dependency graph

## Overview

This is a simple dependency graph useful for determining the order to do a list of things that depend on certain items being done before they are.

To use, `npm install dependency-graph` and then `require('dependency-graph').DepGraph`

## API

### DepGraph

Nodes in the graph are just simple strings.

 - `addNode(name)` - add a node in the graph
 - `removeNode(name)` - remove a node from the graph
 - `hasNode(name)` - check if a node exists in the graph
 - `addDependency(from, to)` - add a dependency between two nodes (will throw an Error if one of the nodes does not exist)
 - `removeDependency(from, to)` - remove a dependency between two nodes
 - `dependenciesOf(name, leavesOnly)` - get an array containing the nodes that the specified node depends on (transitively). If `leavesOnly` is true, only nodes that do not depend on any other nodes will be returned in the array.
 - `dependantsOf(name, leavesOnly)` - get an array containing the nodes that depend on the specified node (transitively). If `leavesOnly` is true, only nodes that do not have any dependants will be returned in the array.
 - `overallOrder(leavesOnly)` - construct the overall processing order for the dependency graph. If `leavesOnly` is true, only nodes that do not depend on any other nodes will be returned.

## Examples

    var DepGraph = require('dependency-graph').DepGraph;

    var graph = new DepGraph();
    graph.addNode('a');
    graph.addNode('b');
    graph.addNode('c');

    graph.addDependency('a', 'b');
    graph.addDependency('b', 'c');

    graph.dependenciesOf('a'); // ['c', 'b']
    graph.dependenciesOf('b'); // ['c']
    graph.dependantsOf('c'); // ['a', 'b']

    graph.overallOrder(); // ['c', 'b', 'a']
    graph.overallOrder(true); // ['c']