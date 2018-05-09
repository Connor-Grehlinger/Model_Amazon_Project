from django.shortcuts import render, redirect, get_object_or_404, get_list_or_404
from django.contrib.auth.models import User
from django.contrib.auth.decorators import login_required
from django.contrib.auth import authenticate, login
from django.contrib import messages
from django.http import HttpResponseRedirect
from django import forms
from django.db import transaction, IntegrityError
from .models import *
from .forms import *
from django.forms.formsets import formset_factory
from django.template import RequestContext
from django.core.mail import EmailMessage
from product.views import list_products
# Create your views here.

def view_404(request):
        """ 
        This function will be called whenever the program returns a 404 or 500 error
        """
        messages.warning(request, "That page either doesn't exist, or you don't have permission.")
        return redirect('/')

def home(request):
        """ 
        This function will check if the user is authenticated, and send event
        information about the user if user belongs to any event. When users want
        the home page, they can click on the DukRSVP button on the upper left side.
        """ 
        return list_products(request)

def login_view(request):
        """
        This function will be called when the user clicks on Login button.
        It will check whether the username/password combination matches to 
        any user in the database using django built in authentication.
        """
        form = LoginForm(request.POST or None)
        if request.POST:
                if 'back' in request.POST:
                        return redirect('/')
                if form.is_valid():
                        user = form.login(request)
                        if user:
                                login(request, user)
                                messages.success(request, 'Login succesful. Welcome, ' + user.username)
                                return redirect('/')
                        else:
                                messages.warning(request, 'Login failed. Wrong username and/or password')
        return render(request, 'registration/login_user.html', {'login_form': form })

def register(request):
        """
        This function will be called when the user clicks on Register button to register
        a new user. It will check whether the username and password is filled out, and 
        whether the email is valid or not (ex: 123@123.123)
        """
        if request.method == 'POST':
                if 'back' in request.POST:
                        return redirect('/')
                form = UserRegistrationForm(request.POST)
                if form.is_valid():
                        userObj = form.cleaned_data
                        username = userObj['username']
                        email =  userObj['email']
                        password =  userObj['password']
                        if not (User.objects.filter(username=username).exists() or User.objects.filter(email=email).exists()):
                                User.objects.create_user(username, email, password)
                                user = authenticate(username = username, password = password)
                                login(request, user)
                                send_email(email,username)
                                return redirect('/')
                        else:
                                messages.warning(request,'Looks like a username with that email or password already exists')
                else:
                        messages.warning(request, 'Email address is invalid!')
        else:
                form = UserRegistrationForm()    
        return render(request, 'registration/register.html', {'form' : form})

@login_required
@transaction.atomic
def profile(request):
        """
        This function will be called when the user clicks on <username>'s Profile button.
        It allows user to modify the information about their profile, as well as new info
        that is first name and last name.
        """
        if request.method == 'POST':
                if 'back' in request.POST:
                        return redirect('/')
                user_form = UserForm(request.POST, instance=request.user)
                if user_form.is_valid():
                        user_form.save()
                        messages.success(request, 'Your profile was successfully updated!')
                        return redirect('/')
                else:
                        messages.warning(request, 'Please correct the error below.')
        else:
                user_form = UserForm(instance=request.user)
        return render(request, 'home/profile.html', {
                'user_form': user_form,
        })

def send_email(email, uname):
        #send email
        try:
                body = "Hello, " + uname + ",your account has been succesfully created!"
                emails = EmailMessage("Ardazon account creation", body, to=[email])
                emails.send()
        except:
                print("Sending email failed!")
