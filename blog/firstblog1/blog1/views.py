from django.shortcuts import render_to_response

# Create your views here.

from blog1.models import posts

def home(request):
    entries = posts.objects.all()[:10]
    return render_to_response('index.html', {'posts' : entries})



