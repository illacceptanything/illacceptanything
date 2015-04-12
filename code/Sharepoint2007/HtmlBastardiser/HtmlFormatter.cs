using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using FontTagGenerator;

namespace HtmlBastardiser
{
    public class HtmlFormatter
    {
        public static string Bastardise(string html)
        {
            return html
                .Replace("\r", String.Empty)
                .Replace("\n", String.Empty)
                .Replace("\t", String.Empty)
                .FontTagMe()
                .Reverse()
                .ToString();
        }
    }
}
