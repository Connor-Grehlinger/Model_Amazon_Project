from django.db import models
from django.contrib.auth.models import User
from django.db.models.signals import post_save
from django.dispatch import receiver
from django.core.validators import MaxValueValidator, MinValueValidator
from django.core.validators import RegexValidator

import calendar

class Profile(models.Model):
    user = models.OneToOneField(User, on_delete=models.CASCADE)
    def __str__(self):
        return self.user.username
    
@receiver(post_save, sender=User)
def create_user_profile(sender, instance, created, **kwargs):
    if created:
        Profile.objects.create(user=instance)

@receiver(post_save, sender=User)
def save_user_profile(sender, instance, **kwargs):
    instance.profile.save()

class Address(models.Model):
    user_id = models.ForeignKey(Profile, on_delete=models.CASCADE)
    address = models.CharField(max_length=2000, default="User address!")
    def __str__(self):
        return self.user_id.user.username + ' ' + address[:10]

class CCInfo(models.Model):
    user_id = models.ForeignKey(Profile, on_delete=models.CASCADE)
    name_on_card = models.CharField(max_length=50, default="YOUR NAME")
    cc_no = models.CharField(max_length=16, validators=[RegexValidator(regex='^.{16}$', message='Length has to be 16', code='nomatch')])
    month = models.CharField(max_length=16, choices=[(str(i), calendar.month_name[i]) for i in range(1,13)], default='1')
    year = models.IntegerField(
        default=2019,
        validators=[MaxValueValidator(2026), MinValueValidator(2018)]
    )
