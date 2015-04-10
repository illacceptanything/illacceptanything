#!/usr/bin/env sh
':' //; exec "$(command -v nodejs || command -v node)" "$0" "$@"
~function(){

// This script searches recent git commits' messages for [git labels][], and prints a list of labels
// to standard output, ordered by number of occurrences.
// 
// Author: twitter.com/@ELLIOTTCABLE
// 
// Usage:
//  $ node <this script>.js [decorate] [age cuttoff] [max commits] [separator char]
//    # for example,
//  $ node rank-gitlabels.js yes 6.months
//  $ node rank-gitlabels.js yes 2.years 10000
// 
// Parameters: (note, defaults for these are set in the corresponding shell-script to this file!)
// 
// 1. decorate:         whether or not to include the actual count for each tag in the output
// 2. age cutoff:       the age of the oldest commit that will be examined (as taken by `git log`)
// 3. max commits:      the maximum number of commits to examine
// 4. separator char:   a character that will never appear in the commit-messages being examined
//                         (defaults to U+001E ‘Record Separator’)
// 
// [git labels]: <https://github.com/ELLIOTTCABLE/.gitlabels>
//    "gitlabels, a system for ‘tagging’ git commits"


// == Modules ==
var child_process = require('child_process')

// == Command-line parameters ==
  , decorate       = !process.argv[2] || !(process.argv[2][0] === 'n' || process.argv[2][0] === 'f')
  , max_commit_age = process.argv[3]                  || '1 year'
  , max_commits    = parseInt(process.argv[4], 10)    || 1000
  , separator      = process.argv[5] && process.argv[4].length > 0 ?
                        process.argv[4]                : "\x1E"

// If piped to `head` or similar, we don't want to spit up on a closing stdout.
process.stdout.on('error', function(err){
   if (err.code === 'epipe') process.exit(0) })


// First, I generate a log of recent commit messages (delimited by `separator` codepoints).
var format_code_body       = '%B'
  , format_code_separator  = '%x'+separator.charCodeAt(0).toString(16)

var git_log = 'git --no-pager log'
    git_log += " --all"
    git_log += " --max-count='"+max_commits+"'"
    git_log += " --since='"+max_commit_age+"'"
    git_log += " --no-decorate"
    git_log += " --no-notes"
    git_log += ' --format="' + 'format:'+format_code_body+format_code_separator + '"'

child_process.exec(git_log, function(err, log){ var extract_labels
    , cases = new Object
    , case_labels, labels
    , case_occurrences = new Object, occurrences = new Object
   
   if (err) throw err
   
   // This will iterate record-wise over each commit-message selected above, extracting gitlabels
   // (both prefix-style `(foo bar)`, and slashtag-style `... hello there! /foo bar`) from each
   // message. Duplicates within each message will only be counted once, although there will be
   // multiple occurrences of each label across *all* of them.
   extract_labels = function(labels, message){ var these_labels = new Array
      if (message.length === 0) return labels
      
      // This is fragile, and will fail when there are slashes *inside* the slashtag, somehow (a
      // key-value pair whose value includes a slash, perhaps?). Then again, maybe I should just
      // explicitly disallow that.
      var bits = message.trim().split('/')
        , first = bits[0]
        , last = bits[bits.length - 1]
      
      // This, too, is fragile; this will break on double-quoted label data containing closing-
      // parens.
      if (first && first[0] == '(') { var
         label_string = first.split(')')[0]
         label_string = label_string.substr(1)
         
         // ... remove label ‘parameter’ values
         label_string = label_string.replace(/:("[^"]+"|[^"\s]+)/gi, '')
         
         these_labels = these_labels.concat(label_string.split(' '))
      }
      
      //if (bits.length > 1 && /^\s/.test(last)) {
      //   // NYI
      //}
      
      // ... remove duplicate labels
      these_labels = these_labels.filter(function(e, idx, labels){ return labels.indexOf(e) === idx })
      
      return labels.concat(these_labels)
   }
   
   case_labels = log.split(separator).reduce(extract_labels, new Array)
   
   case_labels.forEach(function(original){ var lowercase = original.toLowerCase()
      cases[lowercase] = cases[lowercase] || new Array
      
      if (cases[lowercase].indexOf(original) === -1)
          cases[lowercase].push(original)
      
      // Now we're going to count the occurrences of each label; both case-sensitively (to decide
      // which case of a given label is the most common), and case-insensitively (to acutally
      // display the counts).
           occurrences[lowercase] =      occurrences[lowercase] + 1 || 1
      case_occurrences[original]  = case_occurrences[original]  + 1 || 1 })
   
   labels = Object.keys(cases)
   labels.forEach(function(lowercase){
      cases[lowercase].sort(function(a,b){ return case_occurrences[b] - case_occurrences[a] }) })
   
   labels = labels.sort(function(a,b){ return occurrences[b] - occurrences[a] })          // sort
   
   labels.forEach(function(label){
      if (decorate)
         console.log('('+cases[label][0]+'): ' + occurrences[label])
      else
         console.log(label) })
}) }()
