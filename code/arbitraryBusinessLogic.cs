using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace illacceptanything
{
    public class arbitraryBusinessLogic
    {
        public double GetNumberOne(string user)
        {
            if (!string.IsNullOrEmpty(user) && user == "Dave")
            {
                Random rand = new Random();
                double gate = rand.NextDouble();

                if (gate > .3)
                {
                    return .9999999993;
                }
            }
            return 1.0;
        }
    }
}
