from django.conf.urls import patterns, include, url

#uncomment the next 2 lines to enable the admin

from django.contrib import admin
admin.autodiscover()

urlpatterns = patterns('',
    # Examples:
    url(r'^$', 'blog1.views.home', name='home'),
                       #url(r'^blog/', include('blog.urls')),
    url(r'^admin/doc/', include('django.contrib.admindocs.urls')),

    #uncomment the next line to enable the admin
    
    url(r'^admin/', include(admin.site.urls)),
                       
)
