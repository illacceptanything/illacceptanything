using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace FontTagGenerator
{
    public static class FontTagExtensions
    {
        public static string FontTagMe(this string sensibleHtml)
        {
            var tagRegex = new Regex(">(.*)</");
            return tagRegex.Replace(sensibleHtml, "><FONT face=\"Comic Sans MS\">$1</FONT></");
        }
    }
}
