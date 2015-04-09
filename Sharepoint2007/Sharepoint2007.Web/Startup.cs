using Microsoft.Owin;
using Owin;

[assembly: OwinStartupAttribute(typeof(Sharepoint2007.Web.Startup))]
namespace Sharepoint2007.Web
{
    public partial class Startup
    {
        public void Configuration(IAppBuilder app)
        {
            ConfigureAuth(app);
        }
    }
}
