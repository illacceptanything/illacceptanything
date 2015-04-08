<?php

namespace spec\PhpSpec\Formatter\Presenter\Differ;

use PhpSpec\ObjectBehavior;

class StringEngineSpec extends ObjectBehavior
{
    function it_is_a_diff_engine()
    {
        $this->shouldBeAnInstanceOf('PhpSpec\Formatter\Presenter\Differ\EngineInterface');
    }

    function it_supports_string_values()
    {
        $this->supports('string1', 'string2')->shouldReturn(true);
    }

    function it_calculates_strings_diff()
    {
        $expected = <<<DIFF
<code>
@@ -1,1 +1,1 @@
<diff-del>-string1</diff-del>
<diff-add>+string2</diff-add>
</code>
DIFF;

        $this->compare('string1', 'string2')->shouldReturn(str_replace("\n", PHP_EOL, $expected));
    }
}
