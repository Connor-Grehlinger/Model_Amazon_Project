from django import forms
from django.contrib.auth.models import User
from django.forms.formsets import BaseFormSet
from django.contrib.admin import widgets
from django.utils import timezone
from django.contrib.auth import authenticate

class ProductForm(forms.Form):
    def __init__(self, *args, **kwargs):
        self.num_choices = kwargs.pop('num_choices')
        super(ProductForm,self).__init__(*args,**kwargs)
        self.fields['quantity'] = forms.ChoiceField(
            required = False,
            label = 'Number of products',
            choices = [(i, i) for i in range(1, self.num_choices+1)],
        )

class AddressForm(forms.Form):
    def __init__(self, *args, **kwargs):
        super(AddressForm,self).__init__(*args,**kwargs)
        self.fields['x'] = forms.IntegerField(
            required = True,
            label = 'X coordinate',
        )
        self.fields['y'] = forms.IntegerField(
            required = True,
            label = 'Y coordinate',
        )
        self.fields['ups_account'] = forms.CharField(
            max_length = 100,
            required = False,
            label = 'UPS Account',
        )
        self.fields['is_prime'] = forms.IntegerField(
            required = False,
            label = 'Prime Account',
        )

class ChangeAddrForm(forms.Form):
    def __init__(self, *args, **kwargs):
        super(ChangeAddrForm,self).__init__(*args,**kwargs)
        self.fields['x'] = forms.IntegerField(
            required = False,
            label = 'X coordinate',
        )
        self.fields['y'] = forms.IntegerField(
            required = False,
            label = 'Y coordinate',
        )
        
class SearchForm(forms.Form):
    def __init__(self, *args, **kwargs):
        super(SearchForm,self).__init__(*args,**kwargs)
        self.fields['query'] = forms.CharField(
            required = False,
            max_length = 500,
            label = 'Search',
        )

class CouponForm(forms.Form):
    def __init__(self, *args, **kwargs):
        super(CouponForm,self).__init__(*args,**kwargs)
        self.fields['coupons'] = forms.CharField(
            required = False,
            max_length = 50,
            label = 'Coupon',
        )

