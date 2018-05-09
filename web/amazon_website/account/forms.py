from django import forms
from django.contrib.auth.models import User
from django.forms.formsets import BaseFormSet
from django.contrib.admin import widgets
from django.utils import timezone
from django.contrib.auth import authenticate
class UserRegistrationForm(forms.Form):
    username = forms.CharField(
        required = True,
        label = 'Username',
        max_length = 32,
    )
    email = forms.EmailField(
        required = True,
        label = 'Email',
        max_length = 64,
    )
    password = forms.CharField(
        required = True,
        widget=forms.PasswordInput,
        label = 'Password',
        max_length = 32,
    )                
                
class UserForm(forms.ModelForm):
    class Meta:
        model = User
        fields = ( 'username', 'email', 'first_name', 'last_name')

class LoginForm(forms.Form):
    username = forms.CharField(max_length=32, required=True)
    password = forms.CharField(widget=forms.PasswordInput, required=True)
    
    def clean(self):
        username = self.cleaned_data.get('username')
        password = self.cleaned_data.get('password')
        user = authenticate(username=username, password=password)
        if not user or not user.is_active:
            raise forms.ValidationError("Sorry, that login was invalid. Please try again.")
        return self.cleaned_data
    
    def login(self, request):
        username = self.cleaned_data.get('username')
        password = self.cleaned_data.get('password')
        user = authenticate(username=username, password=password)
        return user
