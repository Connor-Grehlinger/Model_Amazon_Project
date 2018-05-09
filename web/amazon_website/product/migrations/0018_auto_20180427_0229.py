# Generated by Django 2.0.1 on 2018-04-27 02:29

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('product', '0017_auto_20180427_0212'),
    ]

    operations = [
        migrations.RemoveField(
            model_name='price',
            name='order',
        ),
        migrations.AddField(
            model_name='order',
            name='price',
            field=models.IntegerField(default=0),
        ),
        migrations.AddField(
            model_name='order',
            name='time',
            field=models.DateTimeField(blank=True, null=True, verbose_name='time of the event'),
        ),
        migrations.DeleteModel(
            name='Price',
        ),
    ]