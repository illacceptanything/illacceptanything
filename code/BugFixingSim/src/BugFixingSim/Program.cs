using System;
using System.Collections.Generic;

namespace BugFixingSim
{
    public class Program
    {
        public void Main(string[] args)
        {
            Console.WriteLine("Bugfixing sim 0.0.1-alpha, fix bugs yourself!");
            Console.WriteLine();
            int task = 1;
            int bugfixes = 0;
            var memleak = new List<byte[]>();
            while (true)
            {
                WHO_NEEDS_WHILE_ANYWAY:
                Console.WriteLine("Bug Task #{0}, enter commit message", task);
                string line;
                memleak.Add(new byte[31337]);
                do
                {
                    line = Console.ReadLine();
                    if (line == "exit")
                        goto OH_YEAH_I_did_use_a_goto_in_a_csharp_program;
                    if (line == "porn")
                        throw new NotSupportedException("porn is not allowed!");
                } while (!CheckBugfix(line));
                bugfixes++;
                Console.WriteLine("Task #{0} is closed.", task++);
                goto WHO_NEEDS_WHILE_ANYWAY;
            }
            OH_YEAH_I_did_use_a_goto_in_a_csharp_program:
            if (bugfixes <= 0)
            {
                Console.WriteLine("Lazy bastart!!! eat this!");
                StackOverflow();
                throw new InvalidOperationException("this shouldn't happen!");
            }
            Console.WriteLine("meh, already tired?, at least you fixed {0} bugs", bugfixes);
        }

        private void StackOverflow()
        {
            try
            {
                //because i'm too lazy to throw the StackOverflowException myself
                StackOverflow();
            }
#if ASPNET50
            catch (StackOverflowException soe)
#else
            catch (Exception e)
#endif
            {
                // what better way is there to handle this great exception than to catch it? :P
                // at least I also added the coreCLR fallback.
            }
        }

        private bool CheckBugfix(string line)
        {
            if (string.IsNullOrWhiteSpace(line))
                return false;
            var words = line.Split(new[] { ' ', ',', '.' }, StringSplitOptions.RemoveEmptyEntries);
            foreach (var word in words)
            {
                switch (word.ToLowerInvariant())
                {
                    case "fixed":
                    case "done":
                    case "workaround":
                    case "intended":
                    case "fix":
                    case "solved":
                        return true;
                    //legacy support for old version and ppl like me who can't type correctly!!!11111
                    case "intented":
                        return true;
                }
            }
            return false;
        }
    }
}
