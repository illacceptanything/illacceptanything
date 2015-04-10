// Much of this is cribbed from the JavaScript grammar.
// This fact makes me a horrible person.  ~ ELLIOTTCABLE

/* This parser outputs arrays-of-arrays-of-elements. The outer Array, a 'sequence', represents a set
 * of semicolon-delimited expressions; each element of that, representing such an 'expression', is
 * again an Array, this time of individual 'words'. Each word may be either an Object with a `type`
 * property, representing some form of literal embedded into the code, or yet another
 * Array-of-Arrays as described above, representing an indirecting subexpression. */

{
   function label(string)        { return { type: 'label',     string: string                   } }
   function execution(sequence)  { return { type: 'execution', sequence: sequence               } }
   
   function R(begin, contents) { return function(it){
      // I'm still fucking unsure whether end is off-by-one.
      it.source = {begin:begin, contents:contents, end:begin + contents.length - 1}; return it } }
}

__ENTER = Sequence

// ### Grammar

Sequence = seq:Expression*
   { seq.type = 'sequence';    return R(offset(), text())(seq) }

Expression = _ expr:Partial _ END _
   { expr.type = 'expression'; return R(offset(), text())(expr) }

Partial
 = PartialWithoutDelimiter
 / PartialWithDelimiter

PartialWithDelimiter
 = it:DelimitedWord _ rest:PartialWithoutDelimiter { rest.unshift(it); return rest }
 / it:DelimitedWord _ rest:PartialWithDelimiter    { rest.unshift(it); return rest }
 / it:DelimitedWord                                { return [it] }

PartialWithoutDelimiter
 = it:Word __ rest:PartialWithoutDelimiter         { rest.unshift(it); return rest }
 / it:Word _  rest:PartialWithDelimiter            { rest.unshift(it); return rest }
 / it:Word                                         { return [it] }


DelimitedWord
 = DelimitedLabel
 / IndirectionSubExpression
 / ExecutionLiteral

IndirectionSubExpression "indirection"
 = '[' it:Sequence ']'                             { return it }


Word "label" = them:LabelCharacter+
   { var it = label(them.join('')); return R(offset(), text())(it) }

LabelCharacter
 = !(WhiteSpace / LineTerminatorSequence / ReservedCharacter) it:.   { return it }

DelimitedLabel "delimited label"
 = BalancedDelimitedLabel
 / StraightDelimitedLabel

BalancedDelimitedLabel
 = '“' them:([^”]*) '”'
   { var it = label(them.join('')); return R(offset(), text())(it) }

StraightDelimitedLabel
 = '"' them:([^"]*) '"'
   { var it = label(them.join('')); return R(offset(), text())(it) }


ExecutionLiteral "execution"
 = '{' it:Sequence '}'
   { return R(offset(), text())( execution(it) ) }

// ### Miscellaneous

_  "optional whitespace"   = WhiteSpace*           { return '??? whitespace' }
__ "seperating whitespace" = WhiteSpace+           { return '??? whitespace' }

END "end of expression"
 = (_ ';'
 / _ &']'
 / _ &'}'
 / _ EOF)                                          { return '??? end' }

WhiteSpace "whitespace character"
 = " "
 / "\t"
 / "\v"
 / "\f"
 / "\u00A0"    // NBSP
 / "\uFEFF"    // ZWNBS
 / Zs
 / LineTerminatorSequence

LineTerminatorSequence "end of line"
 = "\n"        // UNIX
 / "\r\n"      // Windows
 / "\r"        // Mac OS ;)
 / "\u2028"
 / "\u2029"

ReservedCharacter = ["“”{}\[\];]

EOF = !.                                           { return '??? EOF' }
 
// Excerpt from: https://github.com/dmajda/pegjs/blob/master/examples/javascript.pegjs
/*
 * Unicode Character Categories
 *
 * Extracted from the following Unicode Character Database file:
 *
 *   http://www.unicode.org/Public/7.0.0/ucd/extracted/DerivedGeneralCategory.txt
 *   Date: 2014-02-07, 18:42:12 GMT [MD]
 *
 * Unix magic used:
 *
 *   grep "; $CATEGORY" DerivedGeneralCategory.txt |   # Filter characters
 *     cut -f1 -d " " |                                # Extract code points
 *     grep -v '[0-9a-fA-F]\{5\}' |                    # Exclude non-BMP characters
 *     sed -e 's/\.\./-/' |                            # Adjust formatting
 *     sed -e 's/\([0-9a-fA-F]\{4\}\)/\\u\1/g' |       # Adjust formatting
 *     tr -d '\n'                                      # Join lines
 *
 * Non-BMP characters are completely ignored to avoid surrogate pair handling
 * (detecting surrogate pairs isn't possible with a simple character class and
 * other methods would degrade performance).
 */

Zs = [\u0020\u00A0\u1680\u2000-\u200A\u202F\u205F\u3000]
